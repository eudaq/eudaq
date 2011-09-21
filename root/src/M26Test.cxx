#include "eudaq/OptionParser.hh"
#include "eudaq/Exception.hh"
#include "eudaq/FileReader.hh"
#include "eudaq/PluginManager.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/Utils.hh"
#include <set>

#include "TROOT.h"
#include "TStyle.h"
#include "TApplication.h"
#include "TCanvas.h"
#include "TH1D.h"
#include "TFile.h"

static const int M26PIVOTRANGE = 9216;

using namespace eudaq;

std::vector<unsigned> parsenumbers(const std::string & s) {
  std::vector<unsigned> result;
  std::vector<std::string> ranges = split(s, ",");
  for (size_t i = 0; i < ranges.size(); ++i) {
    size_t j = ranges[i].find('-');
    if (j == std::string::npos) {
      unsigned v = from_string(ranges[i], 0);
      result.push_back(v);
    } else {
      long min = from_string(ranges[i].substr(0, j), 0);
      long max = from_string(ranges[i].substr(j+1), 0);
      if (j == 0 && max == 1) {
        result.push_back((unsigned)-1);
      } else if (j == 0 || j == ranges[i].length()-1 || min < 0 || max < min) {
        EUDAQ_THROW("Bad range");
      } else {
        for (long n = min; n <= max; ++n) {
          result.push_back(n);
        }
      }
    }
  }
  return result;
}

struct m26pix_t {
  m26pix_t(int f, int x, int y) : f(f), x(x), y(y) {}
  m26pix_t(const m26pix_t & m, int dx, int dy) : f(m.f), x(m.x+dx), y(m.y+dy) {}
  bool operator < (const m26pix_t & other) const {
    if (f < other.f) return true; else if (f > other.f) return false;
    if (x < other.x) return true; else if (x > other.x) return false;
    if (y < other.y) return true; else if (y > other.y) return false;
    return false;
  }
  int f, x, y;
};

struct clust_t {
  clust_t(const m26pix_t & m)
    : f(m.f), x(m.x), y(m.y) {
    p.insert(m);
  }
  bool operator < (const clust_t & other) const {
    if (p.size() < other.p.size()) return true; else if (p.size() > other.p.size()) return false;
    if (f < other.f) return true; else if (f > other.f) return false;
    if (x < other.x) return true; else if (x > other.x) return false;
    if (y < other.y) return true; else if (y > other.y) return false;
    return false;
  }
  int f;
  double x, y;
  std::set<m26pix_t> p;
};

unsigned offset(unsigned f, unsigned x, unsigned y, unsigned p) {
  return M26PIVOTRANGE * f + y * 16 + (x % 64) / 4 - p;
}

int main(int /*argc*/, char ** argv) {
  eudaq::OptionParser op("EUDAQ M26 decoding test", "1.0", "Testing", 1);
  eudaq::OptionFlag do_display(op, "d", "display", "Display histograms");
  eudaq::Option<unsigned> limit(op, "l", "limit", 1U, "hits", "Maximum hits per event");
  eudaq::Option<unsigned> maxevents(op, "m", "maxevents", 0U, "events", "Stop after processing enough events (0=whole run)");
  eudaq::Option<int> xmin(op, "xmin", "xmin", 0, "pixels", "Minimum X");
  eudaq::Option<int> xmax(op, "xmax", "xmax", 1151, "pixels", "Maximum X");
  eudaq::Option<int> ymin(op, "ymin", "ymin", 0, "pixels", "Minimum Y");
  eudaq::Option<int> ymax(op, "ymax", "ymax", 575, "pixels", "Maximum Y");
  eudaq::Option<std::string> boards(op, "b", "boards", "", "numbers", "Board numbers to extract (empty=all)");
  eudaq::Option<std::string> ipat(op, "i", "inpattern", "../data/run$6R.raw", "string", "Input filename pattern");
  unsigned histo[M26PIVOTRANGE*3] = {0};
  try {
    op.Parse(argv);
    std::vector<unsigned> planes = parsenumbers(boards.Value());
    std::cout << "Boards: ";
    if (planes.empty()) std::cout << "all";
    for (size_t i = 0; i < planes.size(); ++i) std::cout << (i ? ", " : "") << planes[i];
    std::cout << std::endl;
    unsigned numevents = 0;
    for (size_t i = 0; i < op.NumArgs(); ++i) {
      eudaq::FileReader reader(op.GetArg(i), ipat.Value());
      std::cout << "Reading: " << reader.Filename() << std::endl;
      eudaq::PluginManager::Initialize(reader.GetDetectorEvent());
      while (reader.NextEvent()) {
        const eudaq::DetectorEvent & dev = reader.GetDetectorEvent();
        if (dev.IsBORE()) {
          std::cout << "ERROR: Found another BORE !!!" << std::endl;
          continue;
        } else if (dev.IsEORE()) {
          std::cout << "Found EORE" << std::endl;
          continue;
        }
        try {
          //unsigned boardnum = 0;
          if (dev.GetEventNumber() % 1000 == 0) {
            std::cout << "Event " << dev.GetEventNumber() << ", processed " << numevents << std::endl;
          }
          StandardEvent sev = eudaq::PluginManager::ConvertToStandard(dev);
          for (size_t p = 0; p < sev.NumPlanes(); ++p) {
            eudaq::StandardPlane & brd = sev.GetPlane(p);
            if (brd.Sensor() != "MIMOSA26") continue;
            if (!planes.empty() && std::find(planes.begin(), planes.end(), brd.ID()) == planes.end()) continue;
            std::map<m26pix_t, bool> pix;
            for (size_t f = 0; f < brd.NumFrames(); ++f) {
              for (size_t i = 0; i < brd.HitPixels(f); ++i) {
                int x = brd.GetX(i, f), y = brd.GetY(i, f);
                if (x >= xmin.Value() && x <= xmax.Value() &&
                    y >= ymin.Value() && y <= ymax.Value()) {
                  pix[m26pix_t(f, x, y)] = true;
                }
              }
            }
            // clustering (could be improved...)
            std::set<clust_t> clusters;
            for (std::map<m26pix_t, bool>::iterator it = pix.begin(); it != pix.end(); ++it) {
              if (!it->second) continue;
              clust_t clust(it->first);
              for (int dx = 0; dx < 3; ++dx) {
                for (int dy = 0; dy < 3; ++dy) {
                  std::map<m26pix_t, bool>::iterator jt = pix.find(m26pix_t(it->first, dx, dy));
                  if (jt != pix.end() && jt->second) {
                    clust.p.insert(jt->first);
                    jt->second = false;
                  }
                }
              }
              clusters.insert(clust);
            }
            if (clusters.size() > limit.Value()) {
              // remove single-pixel clusters
              std::set<clust_t>::iterator it = clusters.begin();
              while (it != clusters.end()) {
                if (it->p.size() == 1) {
                  clusters.erase(it++);
                } else {
                  ++it;
                }
              }
            }
            if (clusters.size() > 0 && clusters.size() <= limit.Value()) {
	      numevents++;
	      for (std::set<clust_t>::const_iterator itc = clusters.begin(); itc != clusters.end(); ++itc) {
		const std::set<m26pix_t> & pix = itc->p;
		for (std::set<m26pix_t>::const_iterator itp = pix.begin(); itp != pix.end(); ++itp) {
		  histo[M26PIVOTRANGE+offset(itp->f, itp->x, itp->y, brd.PivotPixel())]++;
		}
	      }
            }
            //std::cout << "Clusters:" << std::endl;
            //for (std::set<clust_t>::const_iterator it = clusters.begin(); it != clusters.end(); ++it) {
            //  std::cout << it->f << ", " << it->x << ", " << it->y << ", " << it->p.size() << std::endl;
            //}
          }
	  if (maxevents.Value() && numevents >= maxevents.Value()) break;
        } catch (const Exception & e) {
          std::cout << "Error: " << e.what() << std::endl;
        }
      }
    }
    
    TFile froot("m26test.root", "RECREATE", "", 2);
    TH1D rhisto1 ("m26test1",  "M26 Test 1",  3*M26PIVOTRANGE,    -M26PIVOTRANGE, 2*M26PIVOTRANGE),
         rhisto4 ("m26test4",  "M26 Test 4",  3*M26PIVOTRANGE/4,  -M26PIVOTRANGE, 2*M26PIVOTRANGE),
         rhisto16("m26test16", "M26 Test 16", 3*M26PIVOTRANGE/16, -M26PIVOTRANGE, 2*M26PIVOTRANGE),
         rhisto32("m26test32", "M26 Test 32", 3*M26PIVOTRANGE/32, -M26PIVOTRANGE, 2*M26PIVOTRANGE);
    //unsigned max = 0;
    for (int i = 0; i < 3*M26PIVOTRANGE; ++i) {
      rhisto1.Fill(i-M26PIVOTRANGE, histo[i]);
      rhisto4.Fill(i-M26PIVOTRANGE, histo[i]);
      rhisto16.Fill(i-M26PIVOTRANGE, histo[i]);
      rhisto32.Fill(i-M26PIVOTRANGE, histo[i]);
      //if (histo[i] > max) max = histo[i];
    }
    rhisto1.Write("m26test1", TObject::kOverwrite);
    rhisto4.Write("m26test4", TObject::kOverwrite);
    rhisto16.Write("m26test16", TObject::kOverwrite);
    rhisto32.Write("m26test32", TObject::kOverwrite);
    //froot.Close();
    if (do_display.IsSet()) {
      int x_argc = 0;
      char emptystr[] = "";
      char * x_argv[] = {emptystr, 0};
      TApplication app("M26Test", &x_argc, x_argv);
      gROOT->Reset();
      gROOT->SetStyle("Plain");
      gStyle->SetPalette(1);
      gStyle->SetOptStat(0);
      TCanvas * c = new TCanvas("c", "Histos");
      c->Divide(2, 2);
      c->cd(1);
      rhisto1.Draw();
      c->cd(2);
      rhisto4.Draw();
      c->cd(3);
      rhisto16.Draw();
      c->cd(4);
      rhisto32.Draw();
      app.Run();
    }
  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}
