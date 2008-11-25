#include "eudaq/FileSerializer.hh"
#include "eudaq/DetectorEvent.hh"
#include "eudaq/EUDRBEvent.hh"
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

int main(int /*argc*/, char ** argv) {
  eudaq::OptionParser op("EUDAQtest histo", "1.0",
                         "test",
                         1, 1);
  eudaq::Option<std::string> events(op, "e", "events", "", "numbers", "Event numbers to accumulate (eg. '1-10,99,-1')");
  eudaq::Option<unsigned> board(op, "b", "board", 0U, "number", "Board number");
  eudaq::Option<std::string> rootf(op, "r", "root-file", "", "filename", "File to save root histograms");
  eudaq::Option<int> thresh(op, "t", "thresh", 10, "adcs", "Threshold for CDS");
  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL("INFO");
    std::vector<unsigned> eventlist = parsenumbers(events.Value());
    std::vector<counted_ptr<TH2D> > frames;
    std::vector<counted_ptr<TH1D> > histos;
    std::string datafile = op.GetArg(0);
    if (datafile.find_first_not_of("0123456789") == std::string::npos) {
      datafile = "../data/run" + to_string(from_string(datafile, 0), 6) + ".raw";
    }
    //EUDAQ_INFO("Reading: " + datafile);
    std::cout << "Input file: " << datafile << "\n"
              << "Events: " << events.Value() << " (" << eventlist.size() << ")\n"
              << "Board: " << board.Value() << std::endl;
    eudaq::FileDeserializer des(datafile);
    counted_ptr<eudaq::Event> ev(eudaq::EventFactory::Create(des));
    eudaq::DetectorEvent * dev = dynamic_cast<eudaq::DetectorEvent*>(ev.get());
    counted_ptr<eudaq::EUDRBDecoder> decoder(new eudaq::EUDRBDecoder(*dev));
    const eudaq::EUDRBEvent * eev = dev->GetSubEvent<eudaq::EUDRBEvent>();
    std::string mode = eev->GetTag("MODE" + to_string(board.Value()));
    std::string det = eev->GetTag("DET" + to_string(board.Value()));
    int numframes = 0, w = 0, h = 0;
    size_t numevents = 0;
    if (mode == "ZS") numframes = 1;
    else if (mode == "RAW2") numframes = 3; // inc cds
    else if (mode == "RAW3") numframes = 4; // inc cds
    else EUDAQ_THROW("Unknown MODE: " + mode);
    if (det == "MIMOTEL") {
      w = 264; h = 256;
    } else if (det == "MIMOSA18") {
      w = 512; h = 512;
    } else EUDAQ_THROW("Unknown sensor: " + det);
    for (int f = 0; f < numframes; ++f) {
      std::string namef = "fr" + to_string(f);
      std::string nameh = "hs" + to_string(f);
      std::string title = "Frame " + to_string(f);
      if (f == numframes-1) {
        title = "CDS";
        histos.push_back(counted_ptr<TH1D>(new TH1D(nameh.c_str(), title.c_str(), 2000, -1000, 1000)));
      } else {
        histos.push_back(counted_ptr<TH1D>(new TH1D(nameh.c_str(), title.c_str(), 4096, 0, 4096)));
      }
      frames.push_back(counted_ptr<TH2D>(new TH2D(namef.c_str(), title.c_str(), w, 0, w, h, 0, h)));
    }
    while (des.HasData()) {
      counted_ptr<eudaq::Event> ev(eudaq::EventFactory::Create(des));
      if (ev->IsEORE()) break;
      eudaq::DetectorEvent * dev = dynamic_cast<eudaq::DetectorEvent*>(ev.get());
      std::vector<unsigned>::iterator it = std::find(eventlist.begin(), eventlist.end(), ev->GetEventNumber());
      if (it != eventlist.end()) {
        eventlist.erase(it);
        const eudaq::EUDRBEvent * eev = dev->GetSubEvent<eudaq::EUDRBEvent>();
        if (eev) {
          numevents++;
          const eudaq::EUDRBBoard & brd = eev->GetBoard(board.Value());
          eudaq::EUDRBDecoder::arrays_t<short> data = decoder->GetArrays<short, short>(brd);
          for (size_t f = 0; f < data.m_adc.size(); ++f) {
            TH1D & histo = *histos[f];
            TH2D & frame = *frames[f];
            for (size_t i = 0; i < data.m_x.size(); ++i) {
              histo.Fill(data.m_adc[f][i]);
              frame.Fill(data.m_x[i], data.m_y[i], data.m_adc[f][i]);
            }
          }
          if (data.m_adc.size() > 1) { // calc cds
            std::vector<short> cds(data.m_x.size(), 0);
            if (data.m_adc.size() == 3) {
              for (size_t i = 0; i < data.m_x.size(); ++i) {
                cds[i] = data.m_adc[0][i] * (data.m_pivot[i])
                  + data.m_adc[1][i] * (1-2*data.m_pivot[i])
                  + data.m_adc[2][i] * (data.m_pivot[i]-1);
               }
            } else {
              for (size_t i = 0; i < data.m_x.size(); ++i) {
                cds[i] = data.m_adc[0][i] - data.m_adc[1][i];
              }
            }
            TH1D & histo = *histos.back();
            TH2D & frame = *frames.back();
            for (size_t i = 0; i < data.m_x.size(); ++i) {
              if (cds[i] >= thresh.Value()) {
                histo.Fill(cds[i]);
                frame.Fill(data.m_x[i], data.m_y[i], cds[i]);
              }
            }
          }
        } else {
          EUDAQ_THROW("No eudrb event found");
        }
        if (eventlist.size() == 0) break;
      }
    }
    std::cout << "Histogrammed " << numevents << " events" << std::endl;
    for (size_t i = 0; i < frames.size() - 1; ++i) { // make non-cds frames average (not sum)
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
