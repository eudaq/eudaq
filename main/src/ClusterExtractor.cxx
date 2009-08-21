#include "eudaq/PluginManager.hh"
#include "eudaq/DetectorEvent.hh"
#include "eudaq/FileReader.hh"
#include "eudaq/OptionParser.hh"
//#include "eudaq/Logger.hh"
#include "eudaq/Utils.hh"

#include <iostream>
#include <algorithm>
#include <fstream>

using eudaq::StandardEvent;
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
  eudaq::Option<std::string> ipat(op, "i", "inpattern", "../data/run$6R.raw", "string", "Input filename pattern");
  eudaq::Option<int> clust(op, "p", "cluster-size", 3, "pixels", "Size of clusters (1=no clustering, 3=3x3, etc.)");

  eudaq::Option<double> noise(op, "n", "noise", 4.0, "val", "Noise level, in adc units");
  eudaq::Option<double> thresh_seed(op, "s", "seed-thresh", 5.0, "thresh", "Threshold for seed pixels, in units of sigma");
  eudaq::Option<double> thresh_clus(op, "c", "cluster-thresh", 10.0, "thresh", "Threshold for 3x3 clusters, in units of sigma");

  eudaq::OptionFlag weighted(op, "w", "weighted", "Use weighted average for cluster centre instead of seed position");
  eudaq::OptionFlag tracksonly(op, "t", "tracks-only", "Extract only clusters which are part of a track (not implemented)");
  eudaq::Option<unsigned> limit(op, "l", "limit-events", 0U, "events", "Maximum number of events to process");

  typedef counted_ptr<std::ofstream> fileptr_t;
  typedef std::map<int, fileptr_t> filemap_t;
  filemap_t files;
  try {
    op.Parse(argv);
    //EUDAQ_LOG_LEVEL("INFO");
    for (size_t i = 0; i < op.NumArgs(); ++i) {
      eudaq::FileReader reader(op.GetArg(i), ipat.Value());
      if (tracksonly.IsSet()) EUDAQ_THROW("Tracking is not yet implemented");
      if (clust.Value() < 1 || (clust.Value() % 2) != 1) EUDAQ_THROW("Cluster size must be an odd number");
      std::cout << "Reading: " << reader.Filename() << std::endl;
      std::vector<std::vector<Cluster> > track;
      unsigned runnum = 0;
      const int dclust = clust.Value()/2;
      std::cout << "Cluster size " << clust.Value() << "x" << clust.Value() << " (dclust=" << dclust << ")" << std::endl;
      std::cout << "Seed threshold: " << thresh_seed.Value() << " sigma = "
                << thresh_seed.Value()*noise.Value() << " adc" << std::endl;
      std::cout << "Cluster threshold: " << thresh_clus.Value() << " sigma = "
                << clust.Value()*noise.Value()*thresh_clus.Value() << " adc" << std::endl;
      std::vector<unsigned> hit_hist;

      {
        const eudaq::DetectorEvent & dev = reader.Event();
        eudaq::PluginManager::Initialize(dev);
        runnum = dev.GetRunNumber();
        std::cout << "Found BORE, run number = " << runnum << std::endl;
        //eudaq::PluginManager::ConvertToStandard(dev);
        unsigned totalboards = 0;
        for (size_t i = 0; i < dev.NumEvents(); ++i) {
          const eudaq::Event * drbev = dev.GetEvent(i);
          if (drbev->GetSubType() == "EUDRB") {
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
      }

      while (reader.NextEvent()) {
        const eudaq::DetectorEvent & dev = reader.Event();
        if (dev.IsBORE()) {
          std::cout << "ERROR: Found another BORE !!!" << std::endl;
        } else if (dev.IsEORE()) {
          std::cout << "Found EORE" << std::endl;
        } else {
          try {
            unsigned boardnum = 0;
            if (limit.Value() > 0 && dev.GetEventNumber() >= limit.Value()) break;
            if (dev.GetEventNumber() % 100 == 0) {
              std::cout << "Event " << dev.GetEventNumber() << std::endl;
            }
            for (size_t i = 0; i < track.size(); ++i) {
              track[i].clear();
            }
            StandardEvent sev = eudaq::PluginManager::ConvertToStandard(dev);
            for (size_t p = 0; p < sev.NumPlanes(); ++p) {
              eudaq::StandardPlane & brd = sev.GetPlane(p);
              //size_t npixels = brd.HitPixels();
              //std::vector<short> cds(npixels, 0);
              //for (size_t i = 0; i < brd.m_x.size(); ++i) {
              //  cds[i] = brd.GetPixel(i);
              //}
              std::vector<short> cds(brd.TotalPixels());
              std::vector<Seed> seeds;
              for (unsigned i = 0; i < brd.HitPixels(); ++i) {
                unsigned x = brd.GetX(i), y = brd.GetY(i), idx = brd.XSize() * y + x;
                cds[idx] = brd.GetPixel(i) * brd.Polarity();
                if (cds[idx] >= noise.Value() * thresh_seed.Value()) {
                  seeds.push_back(Seed(x, y, cds[idx]));
                  //if (p < 6) std::cout << i << ", " << x << ", " << y << ", " << idx << ", " << cds[idx] << std::endl;
                }
              }
              std::sort(seeds.begin(), seeds.end(), &Seed::compare);
              std::vector<Cluster> clusters;
              for (size_t i = 0; i < seeds.size(); ++i) {
                bool badseed = false;
                long long charge = 0, sumx = 0, sumy = 0;
                for (int dy = -dclust; dy <= dclust; ++dy) {
                  int y = seeds[i].y + dy;
                  if (y < 0 || y >= (int)brd.YSize()) continue;
                  for (int dx = -dclust; dx <= dclust; ++dx) {
                    int x = seeds[i].x + dx;
                    if (x < 0 || x >= (int)brd.XSize()) continue;
                    size_t idx = brd.XSize() * y + x;
                    if (cds[idx] == BADPIX) {
                      badseed = true;
                    } else {
                      charge += cds[idx];
                      sumx += x*cds[idx];
                      sumy += y*cds[idx];
                    }
                  }
                }
                if (!badseed && charge >= clust.Value() * noise.Value() * thresh_clus.Value()) {
                  double cx = seeds[i].x, cy = seeds[i].y;
                  if (weighted.IsSet()) {
                    cx = sumx / (double)charge;
                    cy = sumy / (double)charge;
                  }
                  clusters.push_back(Cluster(cx, cy, charge));
                  for (int dy = -dclust; dy <= dclust; ++dy) {
                    int y = seeds[i].y + dy;
                    if (y < 0 || y >= (int)brd.YSize()) continue;
                    for (int dx = -dclust; dx <= dclust; ++dx) {
                      int x = seeds[i].x + dx;
                      if (x < 0 || x >= (int)brd.XSize()) continue;
                      size_t idx = brd.XSize() * y + x;
                      cds[idx] = BADPIX;
                    }
                  }
                }
              }
              track[brd.ID()] = clusters;
              boardnum++;
            }

            if (tracksonly.IsSet()) {
              for (size_t c1 = 0; c1 < track[0].size(); ++c1) {
              }
            }
            unsigned numhit = 0;
            for (size_t b = 0; b < track.size(); ++b) {
              if (track[b].size()) {
                numhit++;
                std::string ts = to_string(dev.GetTimestamp() == eudaq::NOTIMESTAMP ? 0 : dev.GetTimestamp());
                *files[b] << dev.GetEventNumber() << "\t" << track[b].size() << "\t" << ts << "\n";
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
      std::cout << "Done. Number of planes with hits:\n";
      unsigned events = 0;
      for (size_t i = 0; i < hit_hist.size(); ++i) {
        events += hit_hist[i];
        std::cout << " " << i << ": " << hit_hist[i] << "\n";
      }
      std::cout << "Out of " << events << " total events." << std::endl;
    }
  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}
