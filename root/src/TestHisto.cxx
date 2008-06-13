#include "eudaq/FileSerializer.hh"
#include "eudaq/DetectorEvent.hh"
#include "eudaq/EUDRBEvent.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Utils.hh"

#include "TROOT.h"
#include "TH2D.h"
#include "TFile.h"

#include <iostream>

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

int main(int /*argc*/, char ** argv) {
  eudaq::OptionParser op("EUDAQtest histo", "1.0",
                         "test",
                         1, 1);
  eudaq::Option<unsigned> event(op, "e", "event", 1U, "number", "Event number");
  eudaq::Option<unsigned> board(op, "b", "board", 0U, "number", "Board number");
  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL("INFO");
    counted_ptr<eudaq::Event> lastevent;
    TH2D fr1("fr1", "Frame1", 264, 0.0, 264.0, 256, 0.0, 256.0);
    TH2D fr2("fr2", "Frame2", 264, 0.0, 264.0, 256, 0.0, 256.0);
    std::string datafile = op.GetArg(0);
    if (datafile.find_first_not_of("0123456789") == std::string::npos) {
      datafile = "../data/run" + to_string(from_string(datafile, 0), 6) + ".raw";
    }
    EUDAQ_INFO("Reading: " + datafile);
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
          if (data.m_adc.size() != 2) EUDAQ_THROW("Not in RAW2 mode");
          for (size_t i = 0; i < data.m_x.size(); ++i) {
            fr1.Fill(data.m_x[i], data.m_y[i], data.m_adc[0][i]);
            fr2.Fill(data.m_x[i], data.m_y[i], data.m_adc[1][i]);
          }
        } else {
          EUDAQ_THROW("No eudrb event found");
        }
        break;
      }
    }
    TFile froot("testhisto.root", "RECREATE", "", 2);
    fr1.Write("fr1", TObject::kOverwrite);
    fr2.Write("fr2", TObject::kOverwrite);
  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}
