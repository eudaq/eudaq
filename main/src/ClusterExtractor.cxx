#include "eudaq/FileSerializer.hh"
#include "eudaq/DetectorEvent.hh"
#include "eudaq/EUDRBEvent.hh"
#include "eudaq/OptionParser.hh"
//#include "eudaq/Logger.hh"
#include "eudaq/Utils.hh"

#include <iostream>
#include <algorithm>

using eudaq::from_string;
using eudaq::to_string;
using eudaq::split;

static const short BADPIX = -32767;

struct Seed {
  Seed(short x, short y, short a) : m_x(x), m_y(y), m_a(a) {}
  static bool compare(const Seed & lhs, const Seed & rhs) { return lhs.m_a > rhs.m_a; }
  short m_x, m_y, m_a;
};

short delmarker(short x) {
  short submat = x / 66;
  short subpix = x - 66 * submat;
  if (subpix < 2) return -1;
  return submat * 64 + subpix - 2;
}

int main(int /*argc*/, char ** argv) {
  eudaq::OptionParser op("EUDAQ Cluster Extractor", "1.0",
                         "A command-line tool for extracting cluster information from native raw files",
                         1);
  //eudaq::OptionFlag tracksonly(op, "t", "tracks-only", "Extract only clusters which are part of a track");
  eudaq::Option<double> noise(op, "n", "noise", 5.0, "val", "Noise level, in adc units");
  eudaq::Option<double> thresh_seed(op, "s", "seed-thresh", 5.0, "thresh", "Threshold for seed pixels, in units of sigma");
  eudaq::Option<double> thresh_clus(op, "c", "cluster-thresh", 10.0, "thresh", "Threshold for 3x3 clusters, in units of sigma");
  typedef counted_ptr<std::ofstream> fileptr_t;
  typedef std::map<int, fileptr_t> filemap_t;
  filemap_t files;
  try {
    op.Parse(argv);
    //EUDAQ_LOG_LEVEL("INFO");
    for (size_t i = 0; i < op.NumArgs(); ++i) {
      std::string datafile = op.GetArg(i);
      if (datafile.find_first_not_of("0123456789") == std::string::npos) {
        datafile = "data/run" + to_string(from_string(datafile, 0), 6) + ".raw";
      }
      std::cout << "Reading: " << datafile << std::endl;
      eudaq::FileDeserializer des(datafile);
      counted_ptr<eudaq::EUDRBDecoder> decoder;
      unsigned runnum = 0;
      while (des.HasData()) {
        counted_ptr<eudaq::Event> ev(eudaq::EventFactory::Create(des));
        eudaq::DetectorEvent * dev = dynamic_cast<eudaq::DetectorEvent*>(ev.get());
        if (ev->IsBORE()) {
          runnum = ev->GetRunNumber();
          std::cout << "Found BORE, run number = " << runnum << std::endl;
          decoder = new eudaq::EUDRBDecoder(*dev);
          //unsigned totalboards = 0;
          for (size_t i = 0; i < dev->NumEvents(); ++i) {
            eudaq::EUDRBEvent * drbev = dynamic_cast<eudaq::EUDRBEvent *>(dev->GetEvent(i));
            if (drbev) {
              unsigned numboards = from_string(drbev->GetTag("BOARDS"), 0);
              for (unsigned i = 0; i < numboards; ++i) {
                int id = from_string(drbev->GetTag("ID" + to_string(i)), -1);
                filemap_t::const_iterator it = files.find(id);
                if (it != files.end()) EUDAQ_THROW("ID is repeated: " + to_string(id));
                std::string fname = "run" + to_string(runnum) + "_eutel_" + to_string(id) + ".txt";
                std::cout << "Opening output file: " << fname << std::endl;
                files[id] = fileptr_t(new std::ofstream(fname.c_str()));
              }
            }
          }
        } else if (ev->IsEORE()) {
          std::cout << "Found EORE" << std::endl;
        } else {
          try {
            unsigned boardnum = 0;
            if (dev->GetEventNumber() % 100 == 0) {
              std::cout << "Event " << dev->GetEventNumber() << std::endl;
            }
            for (size_t i = 0; i < dev->NumEvents(); ++i) {
              eudaq::Event * subev = dev->GetEvent(i);
              eudaq::EUDRBEvent * eudev = dynamic_cast<eudaq::EUDRBEvent*>(subev);
              if (eudev) {
                if (!decoder.get()) EUDAQ_THROW("Missing EUDRB BORE, cannot decode events");
                for (size_t j = 0; j < eudev->NumBoards(); ++j) {
                  eudaq::EUDRBBoard & brd = eudev->GetBoard(j);
                  //std::cout << "boardnum " << boardnum << ", id " << brd.GetID() << std::endl;
                  eudaq::EUDRBDecoder::arrays_t<short, short> a = decoder->GetArrays<short, short>(brd);
                  const size_t npixels = 256*256;
                  std::vector<short> cds(npixels);
                  if (decoder->NumFrames(brd) > 1) {
                    for (size_t i = 0; i < cds.size(); ++i) {
                      short x = delmarker(a.m_x[i]);
                      if (x < 0) continue;
                      size_t idx = x + 256*a.m_y[i];
                      cds[idx] = a.m_adc[0][i] * (a.m_pivot[i])
                        + a.m_adc[1][i] * (1-2*a.m_pivot[i])
                        + a.m_adc[2][i] * (a.m_pivot[i]-1);
                    }
                  } else {
                    for (size_t i = 0; i < a.m_adc.size(); ++i) {
                      short x = delmarker(a.m_x[i]);
                      if (x < 0) continue;
                      size_t idx = x + 256*a.m_y[i];
                      cds[idx] = a.m_adc[0][i];
                    }
                  }
                  std::vector<Seed> seeds;
                  size_t i = 0;
                  for (short y = 0; y < 256; ++y) {
                    for (short x = 0; x < 256; ++x) {
                      if (cds[i] > noise.Value() * thresh_seed.Value()) {
                        seeds.push_back(Seed(x, y, cds[i]));
                      }
                      ++i;
                    }
                  }
                  std::sort(seeds.begin(), seeds.end(), &Seed::compare);
                  std::vector<std::pair<short, short> > clusters;
                  for (size_t i = 0; i < seeds.size(); ++i) {
                    int charge = 0;
                    bool badseed = false;
                    for (int dy = -1; dy < 2; ++dy) {
                      int y = seeds[i].m_y + dy;
                      if (y < 0 || y >= 256) continue;
                      for (int dx = -1; dx < 2; ++dx) {
                        int x = seeds[i].m_x + dx;
                        if (x < 0 || x >= 256) continue;
                        size_t idx = 256 * y + x;
                        if (cds[idx] == BADPIX) badseed = true;
                        charge += cds[idx];
                      }
                    }
                    if (!badseed) {
                      clusters.push_back(std::make_pair(seeds[i].m_x, seeds[i].m_y));
                      for (int dy = -1; dy < 2; ++dy) {
                        int y = seeds[i].m_y + dy;
                        if (y < 0 || y >= 256) continue;
                        for (int dx = -1; dx < 2; ++dx) {
                          int x = seeds[i].m_x + dx;
                          if (x < 0 || x >= 256) continue;
                          size_t idx = 256 * y + x;
                          cds[idx] = BADPIX;
                        }
                      }
                    }
                  }
                  if (clusters.size()) {
                    *files[brd.GetID()] << dev->GetEventNumber() << "\t" << clusters.size() << "\n";
                    for (size_t i = 0; i < clusters.size(); ++i) {
                      *files[brd.GetID()] << " " << clusters[i].first << "\t" << clusters[i].second << "\n";
                    }
                  }
                  boardnum++;
                }
              }
            }
          } catch (const eudaq::Exception & e) {
            std::cerr << "Exception: " << e.what() << std::endl;
          }
        }
      }
    }
  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}
