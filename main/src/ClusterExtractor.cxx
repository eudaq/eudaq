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
  Seed(short x, short y, short a) : x(x), y(y), a(a) {}
  static bool compare(const Seed & lhs, const Seed & rhs) { return lhs.a > rhs.a; }
  short x, y, a;
};

struct Cluster {
  Cluster(double x, double y, unsigned c) : x(x), y(y), c(c) {}
  double x, y;
  unsigned c;
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
  eudaq::Option<int> xsize(op, "x", "x-size", 512, "pixels", "Size of sensor in X direction");
  eudaq::Option<int> ysize(op, "y", "y-size", 512, "pixels", "Size of sensor in Y direction");
  eudaq::Option<int> clust(op, "p", "cluster-size", 3, "pixels", "Size of clusters (1=no clustering, 3=3x3, etc.)");

  eudaq::Option<double> noise(op, "n", "noise", 5.0, "val", "Noise level, in adc units");
  eudaq::Option<double> thresh_seed(op, "s", "seed-thresh", 5.0, "thresh", "Threshold for seed pixels, in units of sigma");
  eudaq::Option<double> thresh_clus(op, "c", "cluster-thresh", 10.0, "thresh", "Threshold for 3x3 clusters, in units of sigma");

  eudaq::OptionFlag weighted(op, "w", "weighted", "Use weighted average for cluster centre instead of seed position");
  eudaq::OptionFlag tracksonly(op, "t", "tracks-only", "Extract only clusters which are part of a track (not implemented)");

  typedef counted_ptr<std::ofstream> fileptr_t;
  typedef std::map<int, fileptr_t> filemap_t;
  filemap_t files;
  try {
    op.Parse(argv);
    //EUDAQ_LOG_LEVEL("INFO");
    for (size_t i = 0; i < op.NumArgs(); ++i) {
      std::string datafile = op.GetArg(i);
      if (datafile.find_first_not_of("0123456789") == std::string::npos) {
        datafile = "../data/run" + to_string(from_string(datafile, 0), 6) + ".raw";
      }
      if (tracksonly.IsSet()) EUDAQ_THROW("Tracking is not yet implemented");
      if (clust.Value() < 1 || (clust.Value() % 2) != 1) EUDAQ_THROW("Cluster size must be an odd number");
      std::cout << "Reading: " << datafile << std::endl;
      eudaq::FileDeserializer des(datafile);
      counted_ptr<eudaq::EUDRBDecoder> decoder;
      std::vector<std::vector<Cluster> > track;
      unsigned runnum = 0;
      const size_t npixels = xsize.Value() * ysize.Value();
      const int dclust = clust.Value()/2;
      std::cout << "Using sensor size " << xsize.Value() << "x" << ysize.Value() << " = " << npixels << " pixels" << std::endl;
      std::cout << "Cluster size " << clust.Value() << "x" << clust.Value() << " (dclust=" << dclust << ")" << std::endl;
      std::cout << "Seed threshold: " << thresh_seed.Value() << " sigma = "
                << thresh_seed.Value()*noise.Value() << " adc" << std::endl;
      std::cout << "Cluster threshold: " << thresh_clus.Value() << " sigma = "
                << clust.Value()*noise.Value()*thresh_clus.Value() << " adc" << std::endl;
      std::vector<unsigned> hit_hist;
      while (des.HasData()) {
        counted_ptr<eudaq::Event> ev(eudaq::EventFactory::Create(des));
        eudaq::DetectorEvent * dev = dynamic_cast<eudaq::DetectorEvent*>(ev.get());
        if (ev->IsBORE()) {
          runnum = ev->GetRunNumber();
          std::cout << "Found BORE, run number = " << runnum << std::endl;
          decoder = new eudaq::EUDRBDecoder(*dev);
          unsigned totalboards = 0;
          for (size_t i = 0; i < dev->NumEvents(); ++i) {
            eudaq::EUDRBEvent * drbev = dynamic_cast<eudaq::EUDRBEvent *>(dev->GetEvent(i));
            if (drbev) {
              unsigned numboards = from_string(drbev->GetTag("BOARDS"), 0);
              totalboards += numboards;
              for (unsigned i = 0; i < numboards; ++i) {
                unsigned id = from_string(drbev->GetTag("ID" + to_string(i)), i);
                if (id >= track.size()) track.resize(id+1);
                filemap_t::const_iterator it = files.find(id);
                if (it != files.end()) EUDAQ_THROW("ID is repeated: " + to_string(id));
                std::string fname = "run" + to_string(runnum) + "_eutel_" + to_string(id) + ".txt";
                std::cout << "Opening output file: " << fname << std::endl;
                files[id] = fileptr_t(new std::ofstream(fname.c_str()));
              }
            }
          }
          if (track.size() != totalboards) EUDAQ_THROW("Missing IDs");
          hit_hist = std::vector<unsigned>(totalboards+1, 0); // resize and clear histo
        } else if (ev->IsEORE()) {
          std::cout << "Found EORE" << std::endl;
        } else {
          try {
            unsigned boardnum = 0;
            if (dev->GetEventNumber() % 100 == 0) {
              std::cout << "Event " << dev->GetEventNumber() << std::endl;
            }
            for (size_t i = 0; i < track.size(); ++i) {
              track[i].clear();
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
                  std::vector<short> cds(npixels);
                  if (decoder->NumFrames(brd) == 1) {
                    for (size_t i = 0; i < a.m_adc[0].size(); ++i) {
                      short x = a.m_x[i]; //delmarker(a.m_x[i]);
                      if (x < 0) continue;
                      size_t idx = x + xsize.Value()*a.m_y[i];
                      //std::cout << "pixel " << x << ", " << a.m_y[i] << " (" << idx << ") = " << a.m_adc[0][i] << std::endl;
                      cds[idx] = a.m_adc[0][i];
                    }
                  } else if (decoder->NumFrames(brd) == 3) {
                    for (size_t i = 0; i < cds.size(); ++i) {
                      short x = delmarker(a.m_x[i]);
                      if (x < 0) continue;
                      size_t idx = x + xsize.Value()*a.m_y[i];
                      cds[idx] = a.m_adc[0][i] * (a.m_pivot[i])
                        + a.m_adc[1][i] * (1-2*a.m_pivot[i])
                        + a.m_adc[2][i] * (a.m_pivot[i]-1);
                    }
                  } else {
                    EUDAQ_THROW("Not implemented: frames=" + to_string(decoder->NumFrames(brd)));
                  }
                  std::vector<Seed> seeds;
                  size_t i = 0;
                  for (short y = 0; y < ysize.Value(); ++y) {
                    for (short x = 0; x < xsize.Value(); ++x) {
                      if (cds[i] > noise.Value() * thresh_seed.Value()) {
                        seeds.push_back(Seed(x, y, cds[i]));
                      }
                      ++i;
                    }
                  }
                  std::sort(seeds.begin(), seeds.end(), &Seed::compare);
                  std::vector<Cluster> clusters;
                  for (size_t i = 0; i < seeds.size(); ++i) {
                    bool badseed = false;
                    long long charge = 0, sumx = 0, sumy = 0;
                    for (int dy = -dclust; dy <= dclust; ++dy) {
                      int y = seeds[i].y + dy;
                      if (y < 0 || y >= ysize.Value()) continue;
                      for (int dx = -dclust; dx <= dclust; ++dx) {
                        int x = seeds[i].x + dx;
                        if (x < 0 || x >= xsize.Value()) continue;
                        size_t idx = xsize.Value() * y + x;
                        if (cds[idx] == BADPIX) {
                          badseed = true;
                        } else {
                          charge += cds[idx];
                          sumx += x*cds[idx];
                          sumy += y*cds[idx];
                        }
                      }
                    }
                    if (!badseed && charge > clust.Value() * noise.Value() * thresh_clus.Value()) {
                      double cx = seeds[i].x, cy = seeds[i].y;
                      if (weighted.IsSet()) {
                        cx = sumx / (double)charge;
                        cy = sumy / (double)charge;
                      }
                      clusters.push_back(Cluster(cx, cy, charge));
                      for (int dy = -dclust; dy <= dclust; ++dy) {
                        int y = seeds[i].y + dy;
                        if (y < 0 || y >= ysize.Value()) continue;
                        for (int dx = -dclust; dx <= dclust; ++dx) {
                          int x = seeds[i].x + dx;
                          if (x < 0 || x >= xsize.Value()) continue;
                          size_t idx = xsize.Value() * y + x;
                          cds[idx] = BADPIX;
                        }
                      }
                    }
                  }
                  track[brd.GetID()] = clusters;
                  boardnum++;
                }
              }
            }
            
            if (tracksonly.IsSet()) {
              for (size_t c1 = 0; c1 < track[0].size(); ++c1) {
              }
            }
            unsigned numhit = 0;
            for (size_t b = 0; b < track.size(); ++b) {
              if (track[b].size()) {
                numhit++;
                *files[b] << dev->GetEventNumber() << "\t" << track[b].size() << "\n";
                for (size_t c = 0; c < track[b].size(); ++c) {
                  *files[b] << " " << track[b][c].x << "\t" << track[b][c].y << "\t" << track[b][c].c << "\n";
                }
                *files[b] << std::flush;
              }
            }
            hit_hist[numhit]++;

          } catch (const eudaq::Exception & e) {
            std::cerr << "Exception: " << e.what() << std::endl;
          }
        }
      }
      std::cout << "Done. Planes hit:\n";
      for (size_t i = 0; i < hit_hist.size(); ++i) {
        std::cout << " " << i << ": " << hit_hist[i] << std::endl;
      }
    }
  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}
