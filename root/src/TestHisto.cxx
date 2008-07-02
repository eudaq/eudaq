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

using eudaq::from_string;
using eudaq::to_string;
using eudaq::split;

int main(int /*argc*/, char ** argv) {
  eudaq::OptionParser op("EUDAQtest histo", "1.0",
                         "test",
                         1, 1);
  eudaq::Option<unsigned> event(op, "e", "event", 1U, "number", "Event number");
  eudaq::Option<unsigned> board(op, "b", "board", 0U, "number", "Board number");
  eudaq::Option<std::string> rootf(op, "r", "root-file", "", "filename", "File to save root histograms");
  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL("INFO");
    std::vector<counted_ptr<TH2D> > histos;
    std::string datafile = op.GetArg(0);
    if (datafile.find_first_not_of("0123456789") == std::string::npos) {
      datafile = "../data/run" + to_string(from_string(datafile, 0), 6) + ".raw";
    }
    //EUDAQ_INFO("Reading: " + datafile);
    std::cout << "Input file: " << datafile << "\n"
              << "Event: " << event.Value() << "\n"
              << "Board: " << board.Value() << std::endl;
    eudaq::FileDeserializer des(datafile);
    counted_ptr<eudaq::Event> ev(eudaq::EventFactory::Create(des));
    eudaq::DetectorEvent * dev = dynamic_cast<eudaq::DetectorEvent*>(ev.get());
    counted_ptr<eudaq::EUDRBDecoder> decoder(new eudaq::EUDRBDecoder(*dev));

    while (des.HasData()) {
      counted_ptr<eudaq::Event> ev(eudaq::EventFactory::Create(des));
      eudaq::DetectorEvent * dev = dynamic_cast<eudaq::DetectorEvent*>(ev.get());
      if (ev->GetEventNumber() == event.Value()) {
        const eudaq::EUDRBEvent * eev = dev->GetSubEvent<eudaq::EUDRBEvent>();
        if (eev) {
          const eudaq::EUDRBBoard & brd = eev->GetBoard(board.Value());
          eudaq::EUDRBDecoder::arrays_t<short> data = decoder->GetArrays<short, short>(brd);
          for (size_t frame = 0; frame < data.m_adc.size(); ++frame) {
            std::string name = "fr" + to_string(frame);
            std::string title = "Frame " + to_string(frame);
            histos.push_back(counted_ptr<TH2D>(new TH2D(name.c_str(), title.c_str(),
                                                        decoder->Width(brd), 0, decoder->Width(brd),
                                                        decoder->Height(brd), 0, decoder->Height(brd))));
            TH2D & histo = *histos.back().get();
            for (size_t i = 0; i < data.m_x.size(); ++i) {
              histo.Fill(data.m_x[i], data.m_y[i], data.m_adc[frame][i]);
            }
          }
        } else {
          EUDAQ_THROW("No eudrb event found");
        }
        break;
      }
    }
    if (histos.size() == 0) {
      EUDAQ_THROW("No boards histogrammed");
    }
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
