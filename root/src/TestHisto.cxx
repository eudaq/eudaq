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
  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL("INFO");
    std::vector<unsigned> eventlist = parsenumbers(events.Value());
    std::vector<counted_ptr<TH2D> > histos;
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
    else if (mode == "RAW2") numframes = 2;
    else if (mode == "RAW3") numframes = 3;
    else EUDAQ_THROW("Unknown MODE: " + mode);
    if (det == "MIMOTEL") {
      w = 264; h = 256;
    } else if (det == "MIMOSA18") {
      w = 512; h = 512;
    } else EUDAQ_THROW("Unknown sensor: " + det);
    for (int f = 0; f < numframes; ++f) {
      std::string name = "fr" + to_string(f);
      std::string title = "Frame " + to_string(f);
      histos.push_back(counted_ptr<TH2D>(new TH2D(name.c_str(), title.c_str(), w, 0, w, h, 0, h)));
    }
    while (des.HasData()) {
      counted_ptr<eudaq::Event> ev(eudaq::EventFactory::Create(des));
      eudaq::DetectorEvent * dev = dynamic_cast<eudaq::DetectorEvent*>(ev.get());
      std::vector<unsigned>::iterator it = std::find(eventlist.begin(), eventlist.end(), ev->GetEventNumber());
      if (it != eventlist.end()) {
        eventlist.erase(it);
        const eudaq::EUDRBEvent * eev = dev->GetSubEvent<eudaq::EUDRBEvent>();
        if (eev) {
          numevents++;
          const eudaq::EUDRBBoard & brd = eev->GetBoard(board.Value());
          eudaq::EUDRBDecoder::arrays_t<short> data = decoder->GetArrays<short, short>(brd);
          for (size_t frame = 0; frame < data.m_adc.size(); ++frame) {
            TH2D & histo = *histos[frame];
            for (size_t i = 0; i < data.m_x.size(); ++i) {
              histo.Fill(data.m_x[i], data.m_y[i], data.m_adc[frame][i]);
            }
          }
        } else {
          EUDAQ_THROW("No eudrb event found");
        }
        if (eventlist.size() == 0) break;
      }
    }
    std::cout << "Histogrammed " << numevents << " events" << std::endl;
    if (rootf.Value() != "") {
      TFile froot(rootf.Value().c_str(), "RECREATE", "", 2);
      for (size_t frame = 0; frame < histos.size(); ++frame) {
        std::string name = "fr" + to_string(frame);
        histos[frame]->Write(name.c_str(), TObject::kOverwrite);
      }
    } else {
      std::cout << "Displaying " << histos.size() << " histos" << std::endl;
      int x_argc = 0;
      char * x_argv[] = {"", 0};
      TApplication app("TestHisto", &x_argc, x_argv);
      gROOT->Reset();
      gROOT->SetStyle("Plain");
      gStyle->SetPalette(1);
      gStyle->SetOptStat(0);
      TCanvas * c = new TCanvas("c1", "Histos");
      c->Divide(2, 2);
      for (size_t frame = 0; frame < histos.size(); ++frame) {
        c->cd(frame+1);
        histos[frame]->Draw("colz");
      }
      std::cout << "Running root" << std::endl;
      app.Run();
      std::cout << "Quitting" << std::endl;
    }
  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}
