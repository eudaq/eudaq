#include "eudaq/FileSerializer.hh"
#include "eudaq/DetectorEvent.hh"
#include "eudaq/EUDRBEvent.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Utils.hh"

#include "lcio.h"
#include "IO/LCWriter.h"
#include "IMPL/LCRunHeaderImpl.h"
#include "IMPL/LCEventImpl.h"
#include "IMPL/LCCollectionVec.h"
#include "IMPL/TrackerRawDataImpl.h"

#include <iostream>

using eudaq::from_string;
using eudaq::to_string;
using eudaq::split;

using namespace lcio;

int main(int /*argc*/, char ** argv) {
  eudaq::OptionParser op("EUDAQ Raw Data file converter", "1.0",
                         "A command-line tool for converting a raw data file to LCIO format",
                         2, 2);
  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL("INFO");
    //for (unsigned i = 0; i < displaynumbers.size(); ++i) std::cout << "+ " << displaynumbers[i] << std::endl;
    counted_ptr<eudaq::Event> lastevent;
    std::string datafile = op.GetArg(0);
    std::string lciofile = op.GetArg(1);
    if (datafile.find_first_not_of("0123456789") == std::string::npos) {
      datafile = "../data/run" + to_string(from_string(datafile, 0), 6) + ".raw";
    }
    EUDAQ_INFO("Reading: " + datafile);
    eudaq::FileDeserializer des(datafile);
    LCWriter * lcwriter = LCFactory::getInstance()->createLCWriter();
    lcwriter->open(lciofile);

    unsigned ndata = 0, nnondet = 0, nbore = 0, neore = 0;
    counted_ptr<eudaq::EUDRBDecoder> decoder;
    while (des.HasData()) {
      counted_ptr<eudaq::Event> ev(eudaq::EventFactory::Create(des));
      eudaq::DetectorEvent * dev = dynamic_cast<eudaq::DetectorEvent*>(ev.get());
      if (ev->IsBORE()) {
        nbore++;
        if (nbore > 1) {
          EUDAQ_WARN("Multiple BOREs (" + to_string(nbore) + ")");
        }
        decoder = new eudaq::EUDRBDecoder(*dev);
        LCRunHeaderImpl * runhdr = new LCRunHeaderImpl;
        runhdr->setRunNumber(dev->GetRunNumber());
        lcwriter->writeRunHeader(runhdr);
        delete runhdr;
      } else if (ev->IsEORE()) {
        neore++;
        if (neore > 1) {
          EUDAQ_WARN("Multiple EOREs (" + to_string(neore) + ")");
        }
      } else if (!dev) {
        nnondet++;
        EUDAQ_WARN("Not a DetectorEvent(" + to_string(nnondet) + ")");
        std::cout << *ev << std::endl;
      } else {
        ndata++;
        // TODO: check event number matches ndata
        LCEventImpl * evt = new LCEventImpl();
        evt->setRunNumber(dev->GetRunNumber());
        evt->setEventNumber(dev->GetEventNumber());
        LCCollectionVec * coll = new LCCollectionVec( LCIO::TRACKERRAWDATA );
        TrackerRawDataImpl * data = new TrackerRawDataImpl;

        unsigned boardnum = 0;
        for (size_t i = 0; i < dev->NumEvents(); ++i) {
          eudaq::Event * subev = dev->GetEvent(i);
          eudaq::EUDRBEvent * eudev = dynamic_cast<eudaq::EUDRBEvent*>(subev);
          if (eudev) {
            if (!decoder.get()) EUDAQ_ERROR("Missing EUDRB BORE, cannot decode events");
            for (size_t j = 0; j < eudev->NumBoards(); ++j) {
              eudaq::EUDRBBoard & brd = eudev->GetBoard(j);
              boardnum++;
              try {
                decoder->GetArrays<short, short>(brd);
                
              } catch (const eudaq::Exception & e) {
                std::cout << "Error in event " << ndata << ": " << e.what() << std::endl;
              }
            }
          }
        }

        std::vector<short> tmp;
        for (int i = 0; i < 10; ++i) tmp.push_back(dev->GetEventNumber() + i);
        data->setADCValues(tmp);
        coll->push_back(data);
        evt->addCollection(coll, "Rubbish");
        lcwriter->writeEvent(evt);
        delete evt;
      }
    }
    EUDAQ_INFO("Number of data events: " + to_string(ndata));
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
    lcwriter->close();
  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}
