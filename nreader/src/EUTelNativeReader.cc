// Version $Id: EUTelNativeReader.cc 2603 2013-05-13 08:25:41Z diont $
/*
 *   This source code is part of the Eutelescope package of Marlin.
 *   You are free to use this source files for your own development as
 *   long as it stays in a public research context. You are not
 *   allowed to use it for commercial purpose. You must put this
 *   header with author names in all development based on this file.
 *
 */

#ifdef USE_EUDAQ
// in this case, read immediately the info
#include <eudaq/Info.hh>

// now check if we have the new plugin mechanism, otherwise quit
#ifdef  EUDAQ_NEW_DECODER

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
#include "EUTelSparseDataImpl.h"
#include "EUTelSimpleSparsePixel.h"
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
#include <eudaq/PluginManager.hh>

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

using namespace std;
using namespace marlin;
using namespace eutelescope;

// initialize static members
const unsigned short EUTelNativeReader::_eudrbOutOfSyncThr = 2;
const unsigned short EUTelNativeReader::_eudrbMaxConsecutiveOutOfSyncWarning = 20;

EUTelNativeReader::EUTelNativeReader ():
  DataSourceProcessor("EUTelNativeReader"),
  _depfetOutputCollectionName(""),
  _eudrbRawModeOutputCollectionName(""),
  _eudrbZSModeOutputCollectionName(""),
  _fileName(""),
  _geoID(0),
  _syncTriggerID(0),
  _depfetDetectors(),
  _eudrbDetectors(),
  _tluDetectors(),
  _eudrbConsecutiveOutOfSyncWarning(0),
  _eudrbPreviousOutOfSyncEvent(0),
  _eudrbSparsePixelType(0),
  _eudrbTotalOutOfSyncEvent(0)
{
  // initialize few variables

  _description = "Reads data streams produced by EUDAQ and produced the corresponding LCIO output";

  registerProcessorParameter("InputFileName", "This is the input file name",
                             _fileName, string("run012345.raw") );

  registerProcessorParameter("GeoID", "The geometry identification number", _geoID, static_cast<int> ( 0 ));


  // from here below only detector specific parameters.

  // ---------- //
  //  EUDRB     //
  // ---------- //

  // first the compulsory parameters

  registerOutputCollection(LCIO::TRACKERRAWDATA, "EUBRDRawModeOutputCollection",
                           "This is the eudrb producer output collection when read in RAW mode",
                           _eudrbRawModeOutputCollectionName, string("rawdata") );

  registerOutputCollection(LCIO::TRACKERDATA, "EUDRBZSModeOutputCollection",
                           "This si the mimotel output collection when read in ZS mode",
                           _eudrbZSModeOutputCollectionName, string("zsdata") );

  registerOutputCollection(LCIO::TRACKERDATA, "DEPFETOutputCollection",
                           "This is the depfet produced output collection",
                           _depfetOutputCollectionName, string("rawdata_dep"));

  registerOptionalParameter("EUDRBSparsePixelType",
                            "Type of sparsified pixel data structure (use SparsePixelType enumerator)",
                            _eudrbSparsePixelType , static_cast<int> ( 1 ) );

  registerProcessorParameter("SyncTriggerID", "Resynchronize the events based on the TLU trigger ID",
                             _syncTriggerID, false );
}

EUTelNativeReader * EUTelNativeReader::newProcessor () {
  return new EUTelNativeReader;
}

void EUTelNativeReader::init () {
  printParameters ();
  ::eudaq::GetLogger().SetErrLevel("WARN"); // send only eudaq messages above (or equal?) "warn" level to stderr
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
  streamlog_out( DEBUG4 ) << "Reading " << _fileName << " with eudaq file deserializer " << endl;

  eudaq::FileReader * reader = 0;
  // open the input file with the eudaq reader
  try{
    reader = new eudaq::FileReader( _fileName, "", _syncTriggerID );
  }
  catch(...){
    streamlog_out( ERROR5 ) << "eudaq::FileReader could not read the input file ' " << _fileName << " '. Please verify that the path and file name are correct." << endl;
//    exit(1);
     throw ParseException("Problems with reading file " + _fileName );
  }

  if ( reader->Event().IsBORE() ) {
    eudaq::PluginManager::Initialize(  reader->Event() );
    // this is the case in which the eudaq event is a Begin Of Run
    // Event. This is translating into a RunHeader in the case of
    // LCIO
    processBORE( reader->Event() );
  }

  while ( reader->NextEvent() && (eventCounter < numEvents ) ) {

    const eudaq::DetectorEvent& eudaqEvent = reader->Event();

    if ( eudaqEvent.IsBORE() ) {

      streamlog_out( WARNING9 ) << "Found another BORE event: This is a strange case but the event will be processed anyway" << endl;
      streamlog_out( WARNING9 ) << eudaqEvent << endl;

      // this is a very strange case, because there should be one and
      // one only BORE in a run and this should be processed already
      // outside the while loop.
      //
      // Anyway I'm processing this BORE again,
      processBORE( eudaqEvent );

    } else if ( eudaqEvent.IsEORE() ) {

      streamlog_out( DEBUG4 ) << "Found a EORE event " << endl;
      streamlog_out( DEBUG4 ) << eudaqEvent << endl;

      processEORE( eudaqEvent );

    } else {
 
      LCEvent * lcEvent = eudaq::PluginManager::ConvertToLCIO( eudaqEvent );

      if ( lcEvent == NULL ) {
	streamlog_out ( ERROR1 ) << "The eudaq plugin manager is not able to create a valid LCEvent" << endl
				 << "Check that eudaq was compiled with LCIO and EUTELESCOPE active "<< endl;
	throw MissingLibraryException( this, "eudaq" );
      }
      ProcessorMgr::instance()->processEvent( lcEvent );
      delete lcEvent;
    }
    ++eventCounter;
  }

  delete reader;

}

void EUTelNativeReader::processEORE( const eudaq::DetectorEvent & eore) {
  streamlog_out( DEBUG4 ) << "Found a EORE, so adding an EORE to the LCIO file as well" << endl;
  EUTelEventImpl * lcioEvent = new EUTelEventImpl;
  lcioEvent->setEventType(kEORE);
  lcioEvent->setTimeStamp  ( eore.GetTimestamp()   );
  lcioEvent->setRunNumber  ( eore.GetRunNumber()   );
  lcioEvent->setEventNumber( eore.GetEventNumber() );

  // sent the lcioEvent to the processor manager for further processing
  ProcessorMgr::instance()->processEvent( static_cast<LCEventImpl*> ( lcioEvent ) ) ;
  delete lcioEvent;
}


void EUTelNativeReader::end () {

  if ( _eudrbTotalOutOfSyncEvent != 0 ) {
    streamlog_out ( MESSAGE5 ) << "Found " << _eudrbTotalOutOfSyncEvent << " events out of sync " << endl;
  }
  streamlog_out ( MESSAGE5 )  << "Successfully finished" << endl;
}


void EUTelNativeReader::processBORE( const eudaq::DetectorEvent & bore ) {
  streamlog_out( DEBUG4 ) << "Found a BORE, so processing the RunHeader" << endl;

  LCRunHeader * runHeader = eudaq::PluginManager::GetLCRunHeader( bore );

  if ( runHeader == NULL ) {
    streamlog_out ( ERROR1 ) << "The eudaq plugin manager is not able to create a valid LCRunHeader" << endl
                             << "Check that eudaq was compiled with LCIO and EUTELESCOPE active "<< endl;
    throw MissingLibraryException( this, "eudaq" );
  }

  ProcessorMgr::instance()->processRunHeader(runHeader);
  delete runHeader;
}

#endif
#endif

//  LocalWords:  serialiazer EUTelNativeReader MIMOTEL rawdata eudrb zsdata
//  LocalWords:  EUBRDRawModeOutputCollection EUDRBZSModeOutputCollection xMin
//  LocalWords:  EUDRBSparsePixelType sensorID xMax yMin yMax EUDRBBoard
//  LocalWords:  EUDRBDecoder TrackerRawData
