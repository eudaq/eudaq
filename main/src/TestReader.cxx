#include "eudaq/FileSerializer.hh"
#include "eudaq/DetectorEvent.hh"
#include "eudaq/EUDRBEvent.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Utils.hh"

#include <iostream>

using eudaq::from_string;
using eudaq::to_string;

int main(int /*argc*/, char ** argv) {
  eudaq::OptionParser op("EUDAQ Raw Data file reader", "1.0",
                         "A command-line tool for printing out a summary of a raw data file",
                         1);
  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL("INFO");
    for (size_t i = 0; i < op.NumArgs(); ++i) {
      std::string datafile = op.GetArg(i);
      if (datafile.find_first_not_of("0123456789") == std::string::npos) {
        datafile = "data/run" + to_string(from_string(datafile, 0), 6) + ".raw";
      }
      std::cout << "Reading: " << datafile << std::endl;
      eudaq::FileDeserializer des(datafile);

      bool seendata = false;
      unsigned ndata = 0, nnondet = 0, nbore = 0, neore = 0;
      counted_ptr<eudaq::EUDRBDecoder> decoder;
      while (des.HasData()) {
        counted_ptr<eudaq::Event> ev(eudaq::EventFactory::Create(des));
        eudaq::DetectorEvent * dev = dynamic_cast<eudaq::DetectorEvent*>(ev.get());
        if (ev->IsBORE()) {
          nbore++;
          if (nbore > 1) {
            std::cout << "Warning: Multiple BOREs (" << nbore << "):\n" << std::endl;
          }
          std::cout << *ev << std::endl;
          for (size_t i = 0; i < dev->NumEvents(); ++i) {
            if (eudaq::EUDRBEvent * eudev = dynamic_cast<eudaq::EUDRBEvent*>(dev->GetEvent(i))) {
              if (eudev->IsBORE()) {
                decoder = new eudaq::EUDRBDecoder(*eudev);
                break;
              }
            }
          }
        } else if (ev->IsEORE()) {
          neore++;
        if (neore > 1) {
          std::cout << "Warning: Multiple EOREs (" << neore << "):\n" << std::endl;
        }
        std::cout << *ev << std::endl;
        } else if (!dev) {
          nnondet++;
          std::cout << "Warning: not a DetectorEvent(" << nnondet << "):\n" << *ev << std::endl;
        } else {
          ndata++;
          if (!seendata || ndata<10) {
            seendata = true;
            std::cout << *ev << std::endl;
            for (size_t i = 0; i < dev->NumEvents(); ++i) {
              eudaq::Event * subev = dev->GetEvent(i);
              eudaq::EUDRBEvent * eudev = dynamic_cast<eudaq::EUDRBEvent*>(subev);
              if (eudev) {
                std::cout << "EUDRB Event:" << std::endl;
                for (size_t j = 0; j < eudev->NumBoards(); ++j) {
                  eudaq::EUDRBBoard & brd = eudev->GetBoard(j);
                  std::cout << " Board " << j << ":\n" << brd;
                  decoder->GetArrays<short, short>(brd);
                }
              }
            }
          }
        }
      }
      std::cout << "Number of data events: " << ndata << std::endl;
      if (nnondet) std::cout << "Warning: Non-DetectorEvents found: " << nnondet << std::endl;
      if (!nbore) {
        std::cout << "Warning: No BORE found" << std::endl;
      } else if (nbore > 1) {
        std::cout << "Warning: Multiple BOREs found: " << nbore << std::endl;
      }
      if (!neore) {
        std::cout << "Warning: No EORE found, possibly truncated file." << std::endl;
      } else if (neore > 1) {
        std::cout << "Warning: Multiple EOREs found: " << nbore << std::endl;
      }
      if (nnondet || (nbore != 1) || (neore > 1)) std::cout << "Probably corrupt file." << std::endl;
    }
  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}
