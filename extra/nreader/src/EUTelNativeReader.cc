#ifdef USE_EUDAQ
// in this case, read immediately the info
#include "eudaq/Event.hh"

// now check if we have the new plugin mechanism, otherwise quit
#ifdef EUDAQ_NEW_DECODER

// continue with the normal sequence of includes and definitions.

// personal includes
#include "EUTelNativeReader.h"
#include "EUTelRunHeaderImpl.h"
#include "EUTelEventImpl.h"
#include "EUTelBaseDetector.h"
#include "EUTelPixelDetector.h"
#include "EUTelMimoTelDetector.h"
#include "EUTelMimosa18Detector.h"
#include "EUTelMimosa26Detector.h"
#include "EUTelTLUDetector.h"
#include "EUTelDEPFETDetector.h"
#include "EUTelSetupDescription.h"

// marlin includes
#include "marlin/Exceptions.h"
#include "marlin/Processor.h"
#include "marlin/DataSourceProcessor.h"
#include "marlin/ProcessorMgr.h"

// eudaq includes
#include <eudaq/FileReader.hh>
#include <eudaq/Event.hh>
#include <eudaq/Logger.hh>

// lcio includes
#include <IMPL/LCCollectionVec.h>
#include <IMPL/TrackerRawDataImpl.h>
#include <IMPL/TrackerDataImpl.h>
#include <IMPL/LCGenericObjectImpl.h>
#include <IMPL/LCEventImpl.h>
#include <UTIL/CellIDEncoder.h>

// system includes
#include <iostream>
#include <cassert>
#include <memory>

#include "config.h" // for version symbols

using namespace std;
using namespace marlin;
using namespace eutelescope;
using namespace eudaq;

// initialize static members
const unsigned short EUTelNativeReader::_eudrbOutOfSyncThr = 2;
const unsigned short EUTelNativeReader::_eudrbMaxConsecutiveOutOfSyncWarning =
    20;

EUTelNativeReader::EUTelNativeReader()
    : DataSourceProcessor("EUTelNativeReader"), _depfetOutputCollectionName(""),
      _eudrbRawModeOutputCollectionName(""),
      _eudrbZSModeOutputCollectionName(""), _fileName(""), _geoID(0),
      _syncTriggerID(0), _depfetDetectors(), _eudrbDetectors(), _tluDetectors(),
      _eudrbConsecutiveOutOfSyncWarning(0), _eudrbPreviousOutOfSyncEvent(0),
      _eudrbSparsePixelType(0), _eudrbTotalOutOfSyncEvent(0) {
  // initialize few variables

  _description = "Reads data streams produced by EUDAQ and produced the "
                 "corresponding LCIO output";

  registerProcessorParameter("InputFileName", "This is the input file name",
                             _fileName, string("run012345.raw"));

  registerProcessorParameter("GeoID", "The geometry identification number",
                             _geoID, static_cast<int>(0));

  // from here below only detector specific parameters.

  // ---------- //
  //  EUDRB     //
  // ---------- //

  // first the compulsory parameters

  registerOutputCollection(
      LCIO::TRACKERRAWDATA, "EUBRDRawModeOutputCollection",
      "This is the eudrb producer output collection when read in RAW mode",
      _eudrbRawModeOutputCollectionName, string("rawdata"));

  registerOutputCollection(
      LCIO::TRACKERDATA, "EUDRBZSModeOutputCollection",
      "This si the mimotel output collection when read in ZS mode",
      _eudrbZSModeOutputCollectionName, string("zsdata"));

  registerOutputCollection(LCIO::TRACKERDATA, "DEPFETOutputCollection",
                           "This is the depfet produced output collection",
                           _depfetOutputCollectionName, string("rawdata_dep"));

  registerOptionalParameter("EUDRBSparsePixelType",
                            "Type of sparsified pixel data structure (use "
                            "SparsePixelType enumerator)",
                            _eudrbSparsePixelType, static_cast<int>(1));
}

EUTelNativeReader *EUTelNativeReader::newProcessor() {
  return new EUTelNativeReader;
}

void EUTelNativeReader::init() {
  printParameters();
  ::eudaq::GetLogger().SetErrLevel("WARN"); // send only eudaq messages above
                                            // (or equal?) "warn" level to
                                            // stderr
  streamlog_out(MESSAGE5) << "Initializing EUDAQ native reader Marlin library "
                          << PACKAGE_VERSION << endl;
}

void EUTelNativeReader::readDataSource(int numEvents) {
  // this event counter is used to stop the processing when it is
  // greater than numEvents.
  int eventCounter = 0;

  // by definition this is the first event!
  _isFirstEvent = true;

  // this is to make the output messages nicer
  streamlog::logscope scope(streamlog::out);
  scope.setName(name());

  // print out a debug message
  streamlog_out(DEBUG4) << "Reading " << _fileName
                        << " with eudaq file deserializer " << endl;

  eudaq::FileReaderUP reader;
  // open the input file with the eudaq reader
  try {
    reader = eudaq::Factory<eudaq::FileReader>::MakeUnique(eudaq::str2hash("native"), _fileName);    
  } catch (...) {
    streamlog_out(ERROR5)
        << "eudaq::FileReader could not read the input file ' " << _fileName
        << " '. Please verify that the path and file name are correct." << endl;
    //    exit(1);
    throw ParseException("Problems with reading file " + _fileName);
  }

  while (eventCounter < numEvents) {
    auto ev = reader->GetNextEvent();
    if(!ev)
      break;
    LCEventSP lcEvent(new lcio::LCEventImpl);
    LCEventConverter::Convert(ev, lcEvent, nullptr);
    if (lcEvent == NULL) {
      streamlog_out(ERROR1)
            << "The eudaq plugin manager is not able to create a valid LCEvent"
            << endl
            << "Check that eudaq was compiled with LCIO and EUTELESCOPE active "
            << endl;
        throw MissingLibraryException(this, "eudaq");
    }
    if (lcEvent->getParameters().getIntVal(
              "FLAG") == Event::FLAG_STATUS) { /*cout << "Skipping event " <<
                                                  lcEvent->getEventNumber() <<
                                                  ": status event" <<  endl;*/
      continue;
    } else if (lcEvent->getParameters().getIntVal(
						  "FLAG") == Event::
	       FLAG_BROKEN) { /*cout << "Skipping event "
				<<
				lcEvent->getEventNumber()
				<< ": broken or out of
				sync" <<  endl;*/
      continue;
    }
    ProcessorMgr::instance()->processEvent(lcEvent.get());
    ++eventCounter;
  }
}

void EUTelNativeReader::end() {

  if (_eudrbTotalOutOfSyncEvent != 0) {
    streamlog_out(MESSAGE5) << "Found " << _eudrbTotalOutOfSyncEvent
                            << " events out of sync " << endl;
  }
  streamlog_out(MESSAGE5) << "Successfully finished" << endl;
}

#endif
#endif
