#include "eudaq/FileReader.hh"
#include "eudaq/PluginManager.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Utils.hh"

#include "TROOT.h"
#include "TStyle.h"
#include "TApplication.h"
#include "TCanvas.h"
#include "TH2D.h"
#include "TH1D.h"
#include "TFile.h"

#include <iostream>
#include <ostream>
#include <algorithm>

using eudaq::from_string;
using eudaq::to_string;
using eudaq::split;

std::vector<unsigned> parsenumbers(const std::string & s) {
  std::vector<unsigned> result;
  std::vector<std::string> ranges = split(s, ",");
  for (size_t i = 0; i < ranges.size(); ++i) {
    size_t j = ranges[i].find('-');
    if (j == std::string::npos) {
      unsigned v = from_string(ranges[i], 0);
      result.push_back(v);
    } else {
      unsigned min = from_string(ranges[i].substr(0, j), 0);
      unsigned max = from_string(ranges[i].substr(j+1), 0);
      if (j == 0 && max == 1) {
        result.push_back((unsigned)-1);
      } else if (j == 0 || j == ranges[i].length()-1 || max < min) {
        EUDAQ_THROW("Bad range");
      } else {
        for (unsigned n = min; n <= max; ++n) {
          result.push_back(n);
        }
      }
    }
  }
  return result;
}

#define max(a, b) ((a) > (b) ? (a) : (b))

int main(int /*argc*/, char ** argv) {
  eudaq::OptionParser op("EUDAQtest histo", "1.0",
                         "test",
                         1, 1);
  eudaq::Option<std::string> events(op, "e", "events", "", "numbers", "Event numbers to accumulate (eg. '1-10,99,-1')");
  eudaq::Option<unsigned> board(op, "b", "board", 0U, "number", "Board number");
  eudaq::Option<std::string> rootf(op, "r", "root-file", "", "filename", "File to save root histograms");
  eudaq::Option<int> thresh(op, "t", "thresh", 10, "adcs", "Threshold for CDS");
  eudaq::Option<std::string> ipat(op, "i", "inpattern", "../data/run$6R.raw", "string", "Input filename pattern");
  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL("INFO");
    std::vector<unsigned> eventlist = parsenumbers(events.Value());
    std::vector<counted_ptr<TH2D> > frames;
    std::vector<counted_ptr<TH1D> > histos;
    bool booked = false;
    unsigned numevents = 0;
    //EUDAQ_INFO("Reading: " + datafile);
    std::cout << "Input file pattern: " << ipat.Value() << "\n"
              << "Events: " << events.Value() << " (" << eventlist.size() << ")\n"
              << "Board: " << board.Value() << std::endl;
    for (size_t i = 0; i < op.NumArgs(); ++i) {
      eudaq::FileReader reader(op.GetArg(i), ipat.Value());
      EUDAQ_INFO("Reading: " + reader.Filename());
      if (!reader.GetEvent().IsBORE()) {
        EUDAQ_THROW("First event is not a BORE!");
      }
      eudaq::PluginManager::Initialize(reader.GetDetectorEvent());
      while (reader.NextEvent()) {
        std::vector<unsigned>::iterator it = std::find(eventlist.begin(),
                                                       eventlist.end(),
                                                       reader.GetEvent().GetEventNumber());
        if (it == eventlist.end()) continue;
        numevents++;
        eudaq::StandardEvent ev = eudaq::PluginManager::ConvertToStandard(reader.GetDetectorEvent());
        eudaq::StandardPlane pl = ev.GetPlane(board.Value());
        if (!booked) {
          booked = true;
          unsigned w = pl.XSize(), h = pl.YSize();
          for (unsigned f = 0; f < pl.NumFrames(); ++f) {
            std::string namef = "fr" + to_string(f);
            std::string nameh = "hs" + to_string(f);
            std::string title = "Frame " + to_string(f);
            histos.push_back(counted_ptr<TH1D>(new TH1D(nameh.c_str(), title.c_str(), 512, 0, 4096)));
            frames.push_back(counted_ptr<TH2D>(new TH2D(namef.c_str(), title.c_str(), w, 0, w, h, 0, h)));
          }
          if (pl.NumFrames() > 1) {
            unsigned f = pl.NumFrames();
            std::string namef = "fr" + to_string(f);
            std::string nameh = "hs" + to_string(f);
            std::string title = "CDS";
            histos.push_back(counted_ptr<TH1D>(new TH1D(nameh.c_str(), title.c_str(), 512, 0, 4096)));
            frames.push_back(counted_ptr<TH2D>(new TH2D(namef.c_str(), title.c_str(), w, 0, w, h, 0, h)));
          }
        }

        for (size_t f = 0; f < pl.NumFrames(); ++f) {
          TH1D & histo = *histos[f];
          TH2D & frame = *frames[f];
          for (size_t i = 0; i < pl.HitPixels(); ++i) {
            histo.Fill(pl.GetPixel(i, f));
            frame.Fill(pl.GetX(i, f), pl.GetY(i, f), pl.GetPixel(i, f));
          }
          if (pl.NumFrames() > 1) { // calc cds
            std::vector<short> cds = pl.GetPixels<short>();
            TH1D & histo = *histos.back();
            TH2D & frame = *frames.back();
            for (size_t i = 0; i < cds.size(); ++i) {
              double v = cds[i];
              if (v >= thresh.Value()) {
                histo.Fill(v);
                frame.Fill(pl.GetX(i), pl.GetY(i), v);
              }
            }
          }
        }
      }
    }

    std::cout << "Histogrammed " << numevents << " events" << std::endl;
    for (size_t i = 0; i < frames.size(); ++i) { // make frames average (not sum)
      for (int iy = 1; iy <= frames[i]->GetNbinsY(); ++iy) {
        for (int ix = 1; ix <= frames[i]->GetNbinsX(); ++ix) {
          double a = frames[i]->GetBinContent(ix, iy);
          frames[i]->SetBinContent(ix, iy, a / numevents);
        }
      }
    }
    if (rootf.Value() != "") {
      TFile froot(rootf.Value().c_str(), "RECREATE", "", 2);
      for (size_t f = 0; f < frames.size(); ++f) {
        std::string nameh = "hs" + to_string(f);
        std::string namef = "fr" + to_string(f);
        histos[f]->Write(nameh.c_str(), TObject::kOverwrite);
        frames[f]->Write(namef.c_str(), TObject::kOverwrite);
      }
    } else {
      std::cout << "Displaying " << frames.size() << " histos" << std::endl;
      int x_argc = 0;
      char emptystr[] = "";
      char * x_argv[] = {emptystr, 0};
      TApplication app("TestHisto", &x_argc, x_argv);
      gROOT->Reset();
      gROOT->SetStyle("Plain");
      gStyle->SetPalette(1);
      gStyle->SetOptStat(0);
      const int n = frames.size();
      TCanvas * ch = new TCanvas("ch", "Histos");
      ch->Divide(n > 1 ? 2 : 1, n > 2 ? 2 : 1);
      for (int f = 0; f < n; ++f) {
        ch->cd(f+1);
        histos[f]->Draw();
      }
      TCanvas * cf = new TCanvas("cf", "Frames");
      cf->Divide(n > 1 ? 2 : 1, n > 2 ? 2 : 1);
      for (int f = 0; f < n; ++f) {
        cf->cd(f+1);
        frames[f]->Draw("colz");
      }
      app.Run();
    }
  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}
