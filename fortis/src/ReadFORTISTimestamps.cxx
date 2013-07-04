#include "eudaq/FileReader.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/PluginManager.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Utils.hh"
#include "eudaq/DetectorEvent.hh"
#include "eudaq/RawDataEvent.hh"

#include <iostream>
#include <fstream>
#include <assert.h>

#include "fortis/FORTIS.hh"

#ifdef USE_ROOT
// Include files for ROOT
#include "TH3F.h"
#include "TH2I.h"
#include "TROOT.h"
#include "TCanvas.h"
#include "TStyle.h"
#include "TFile.h"
#endif


// For 128x128 pixels
// Pixel rate                              0.96125 MHz
// pixel time = 1.0403us
// Frame rate                              32.093 fps
// Frame period                            31.1594 ms
// Designed for clock of 40.625 MHz clock ??
// Clock multiplier from 48MHz clock inside TLU ( effectively 384MHz with x8 )
// 128x128x2x1.0403us = 3.4088ms
// two clocks for each pixel - corellated double sampling.
//

using eudaq::StandardEvent;
using eudaq::from_string;
using eudaq::to_string;
using eudaq::split;
using namespace std;

unsigned int GetFrameNumber( const std::vector<unsigned char> data , const int numRows , const int numColumns  , unsigned int frame) {

  unsigned int wordsPerRow =  numColumns +  WORDS_IN_ROW_HEADER;

  unsigned numPixels = wordsPerRow * numRows;

  unsigned numFrames = 2;

  if ( data.size() != numPixels * sizeof(short) * numFrames) {
    std::cout << "GetFrameNumber: Fatal error. Data isn't the correct size. Predicted, actual size = " << numPixels << "\t" << data.size() << " for num columns, rows = " << numRows << "\t" << numColumns << std::endl;
    EUDAQ_THROW("Data size mis-match");
  }

  unsigned int frameNumber = 
    data[0+ (numRows*wordsPerRow)*sizeof(short)*frame ] +
    data[1+ (numRows*wordsPerRow)*sizeof(short)*frame]*0x00000100 + 
    data[2+ (numRows*wordsPerRow)*sizeof(short)*frame]*0x00010000 + 
    data[3+ (numRows*wordsPerRow)*sizeof(short)*frame]*0x01000000 ;
  
  return frameNumber;
}

std::vector<unsigned int> GetPivotRows( const std::vector<unsigned char> data , const int numRows , const int numColumns  ) {

  //  const int debug_level = 0xFFFFFFFF;
  const int debug_level = 0x00000000;

  std ::vector<unsigned int> pivotRows;

  unsigned triggersPending = 0;
 
  unsigned int wordsPerRow =  numColumns +  WORDS_IN_ROW_HEADER;

  unsigned numPixels = wordsPerRow * numRows;

  const unsigned numFrames = 2; // number of frames in raw data.

  if ( data.size() != numPixels * sizeof(short) * numFrames) {
    std::cout << "GetPivotRows: Fatal error. Data isn't the correct size. Predicted, actual size = " << numPixels << "\t" << data.size() << " for num columns, rows = " << numRows << "\t" << numColumns << std::endl;
    EUDAQ_THROW("Data size mis-match");
  }

  // std::cout << "GetPivotRows: Rows, Columns = " << numRows << "\t" << numColumns << std::endl;

  // look through first frame for triggers.....
  for ( unsigned int row_counter=(numRows-1); !(row_counter == 0) ; --row_counter) {

      unsigned int triggersInCurrentRow , triggersInRowZero;
      unsigned int TriggerWord = data[0 + (row_counter*wordsPerRow)*sizeof(short) ] + data[1+ (row_counter*wordsPerRow)*sizeof(short) ]*0x100 ;

      unsigned int LatchWord   = data[2+ (row_counter*wordsPerRow)*sizeof(short) ] + data[3+ (row_counter*wordsPerRow)*sizeof(short) ]*0x100;

      if ( debug_level & FORTIS_DEBUG_TRIGGERWORDS ) {
	std::cout << "row, Trigger words : " <<  hex << row_counter << "  " << TriggerWord << "   " << LatchWord  << "\tTriggers pending = " << triggersPending << std::endl;
      }

      if ( row_counter == 1 ) { // if we are on the first row, include the previous row ( zero ) where the trigger counter is taken over by the frame-counter.
	triggersInRowZero = (( 0xFF00 & TriggerWord )>>8) ;
	triggersPending += triggersInRowZero;
	for ( size_t trig = 0; trig < triggersInRowZero; trig++) { pivotRows.push_back(row_counter); } // push current row into pivot-pixels FIFO
      }

      triggersInCurrentRow = ( 0x00FF & TriggerWord );	  
      triggersPending += triggersInCurrentRow ;
      for ( size_t trig = 0; trig < triggersInCurrentRow; trig++) { pivotRows.push_back(row_counter); } // push current row into pivot-pixels FIFO

      // end of search for triggers

      if ( triggersPending > MAX_TRIGGERS_PER_FORTIS_FRAME ) {
	std::cout << "Too many triggers found. Dumping frame" << std::endl;
	//printFrames( m_bufferNumber );

	EUDAQ_THROW("Error - too many triggers found. #trigs = " + eudaq::to_string(triggersPending)); 
      }
    }
  assert ( triggersPending == pivotRows.size() ) ;


  if ( debug_level & FORTIS_DEBUG_TRIGGERWORDS ) {
    for ( unsigned pivotRowID = 0 ; pivotRowID < pivotRows.size(); ++pivotRowID ) {
      std::cout << "Pivot ID, row = " << pivotRowID << "\t" << pivotRows[pivotRowID] << std::endl;
    }
  }
  return pivotRows;

}


int main(int /*argc*/, char ** argv) {

  eudaq::OptionParser op("EUDAQ FORTIS Frame number / TLU timestamp reader", "1.0",
                         "A command-line tool for extracting Fortis frame number and TLU timestamp from a raw data file",
                         1);
  eudaq::Option<std::string> ipat(op, "i", "inpattern", "../data/run$6R.raw", "string", "Input filename pattern");
  eudaq::Option<int> limit(op, "l", "limit", -1, "event limit",
				     "Maximum number of events to process.");
  eudaq::Option<int> rows(op, "r", "rows", 0, "#rows",
				     "Number of rows in frame. By default read from BORE");
  eudaq::Option<int> columns(op, "c", "columns", 0, "#columns",
				     "Number of columns in frame. By default read from BORE");
  eudaq::Option<std::string> rootfile(op, "o", "root", "fortisTimestamps.root", "Root File",
				     "Root output file containing timestamp histograms");
  eudaq::Option<std::string> textfile(op, "t", "text", "fortisTimestmps.txt", "Text File",
				     "Text output file containing timestamps");

  try {
        
    op.Parse(argv);

#   ifdef USE_ROOT
    EUDAQ_INFO("Opening root file: " + rootfile.Value());
    TFile RootFile( rootfile.Value().c_str() , "recreate" );
    TCanvas *c1 = new TCanvas("c1","c1");
    TH2I* h1 = new TH2I("TimestampCorrelation","Delta-Timestamp Correlation",1000,0,1000,1000,0,1000);
    TH2F* h2 = new TH2F("TimestampCorrelationFraction","Delta-Timestamp Correlation Fraction",1000,0.0,1000.0,1000,0.0,2.0);
    TH2I* h3 = new TH2I("TimestampCorrelationPrevious","Delta-Timestamp Correlation with Previous Event",1000,0,1000,1000,0,1000);
    TH1F* h4 = new TH1F("DeltaTimestampDifference","Delta-Timestamp Difference",1000,-1000.0,1000.0);
    TH1I* h5 = new TH1I("TLUDeltaTimestamp","TLU Delta-Timestamp",1000,-1000,1000);
#   endif

    EUDAQ_INFO("Opening text file: " + textfile.Value());
    std::ofstream timestampStream( textfile.Value().c_str());
    timestampStream << "# Run\tEvent\tTLUTimestamp\tFrame\tPivotRow" << std::endl; //print out header

    unsigned long long previousTimestamp = 0 ;
    unsigned long long startTimestamp = 0 ;

    int previousFortisFrameNumber =0 ;
    int startFortisFrameNumber =0 ;

    // int previousPivotRow = 0;

    unsigned long int  previousFortisTimestamp = 0 ;
    unsigned long int  startFortisTimestamp = 0 ;

    int previousDeltaTimestamp = 0;

    int numRows =  rows.Value();
    int numColumns = columns.Value();

    bool firstEvent = true;

    int runNumber = 0;

    for (size_t i = 0; i < op.NumArgs(); ++i) { // loop through run numbers

      eudaq::FileReader reader(op.GetArg(i), ipat.Value(), false );
      EUDAQ_INFO("Reading: " + reader.Filename());

      {
        const eudaq::Event & borev = reader.GetEvent();
        runNumber = borev.GetRunNumber();
	if ( borev.IsBORE() ) { std::cout << "Found BORE" << std::endl; } else { EUDAQ_THROW("Fatal error - first event isn't BORE");}
        std::cout << "Run number = " << runNumber << std::endl;

        const eudaq::DetectorEvent * dev = dynamic_cast<const eudaq::DetectorEvent *>(&borev);
        for (size_t i = 0; i < dev->NumEvents(); ++i) { // loop over detectors in event
	    const eudaq::RawDataEvent * rev = dynamic_cast<const eudaq::RawDataEvent *>(dev->GetEvent(i));
	    if (rev && rev->GetSubType() == "FORTIS" ) {
		if ( ! (numRows && numColumns) ) {
  		  numRows = from_string(rev->GetTag("NumRows"), -1);
		  numColumns = from_string(rev->GetTag("NumColumns"), -1);
		  std::cout << "Setting num rows, num columns from data = " << numRows << "\t" << numColumns << std::endl;
		} else {
		  std::cout << "Setting num rows, num columns from command line = " << numRows << "\t" << numColumns << std::endl;
		}

	    }
        }

      }

      if ( ! (numRows && numColumns) ) { EUDAQ_THROW("Failed to get Rows, Columns. Halting. (Are you sure this file contains FORTIS data?"); }
      std::vector<unsigned int> pivotRows;
      while (reader.NextEvent()) { // loop over events
	const eudaq::Event & ev = reader.GetEvent();

	if (! ( ev.IsBORE() ||ev.IsEORE()) ) { // not a BORE or EORE

	  const eudaq::DetectorEvent * dev = dynamic_cast<const eudaq::DetectorEvent *>(&ev);

	  if (limit.Value() > 0 && (int) ev.GetEventNumber() >= limit.Value()) break;
	  if (ev.GetEventNumber() % 100 == 0) {
	    std::cout << "Event " << ev.GetEventNumber() << std::endl;
	  }



	  for (size_t i = 0; i < dev->NumEvents(); ++i) { // loop over detectors in event
	
	    const eudaq::RawDataEvent * rev = dynamic_cast<const eudaq::RawDataEvent *>(dev->GetEvent(i));
	    if (rev && rev->GetSubType() == "FORTIS" ) {
	      
	      unsigned eventNumber = dev->GetEventNumber();
	      unsigned long long Timestamp = (dev->GetTimestamp() == eudaq::NOTIMESTAMP) ? 0 : dev->GetTimestamp();

	      int fortisFrameNumber = from_string(rev->GetTag( "FRAMENUMBER"),-1);
	      int fortisPivotRow = from_string(rev->GetTag("PIVOTROW"),-1);
	      long int FortisTimestamp = fortisFrameNumber*numRows + fortisPivotRow;

	      if ( rev->NumBlocks() ) {
		 if ( ! pivotRows.empty() ) {
                   std::cout << "Error pivot rows vector not empty. Size = " << pivotRows.size() << std::endl;
		   EUDAQ_THROW("pivotRows not empty");
		 }
		 // std::cout << " (ID " << rev->GetID(0) << ") ####" << std::endl;
		 const std::vector<unsigned char> & data = rev->GetBlock(0);
		 pivotRows = GetPivotRows( data , numRows , numColumns );

                if ( fortisFrameNumber != (int) GetFrameNumber( data , numRows , numColumns ,0 )) {
                  std::cout << "Frame from tag, raw-data = " << fortisFrameNumber << "\t" << GetFrameNumber( data , numRows , numColumns ,0 ) << std::endl;
                  EUDAQ_THROW("Frame number mis-match");
                }

               if ( GetFrameNumber( data , numRows , numColumns ,1 ) != (GetFrameNumber( data , numRows , numColumns ,0 )+1) ) {
                 std::cout << "Frame numbers in raw data pair incorrect: 0,1 = " << GetFrameNumber( data , numRows , numColumns ,0 ) << "\t" <<  GetFrameNumber( data , numRows , numColumns ,1 ) << std::endl;
		 EUDAQ_THROW("Frame numbers wrong in raw data");
	       }

	      }

	      unsigned int currentPivotRow = pivotRows.back();
	      pivotRows.pop_back();
              if ( currentPivotRow != (unsigned) fortisPivotRow ) {
		std::cout << "Row from tag, raw-data = " << fortisPivotRow << "\t" << currentPivotRow << std::endl;
		EUDAQ_THROW("Row number mis-match");
	      }



	      if (firstEvent) {
		startTimestamp = Timestamp; previousTimestamp = Timestamp;
		startFortisFrameNumber = fortisFrameNumber; previousFortisFrameNumber = fortisFrameNumber-1;
		startFortisTimestamp = FortisTimestamp; previousFortisTimestamp = FortisTimestamp;
		firstEvent = false;
	      }


              if ( fortisFrameNumber == previousFortisFrameNumber  ) {
                std::cout << "Current, previous frame = " << fortisFrameNumber << "\t" << previousFortisFrameNumber << std::endl;
                EUDAQ_THROW("Frame number identical to previous");
              }

	      int deltaFortisTimestamp = FortisTimestamp - previousFortisTimestamp ; previousFortisTimestamp =  FortisTimestamp;
	      int deltaTimestamp = (int)((Timestamp - previousTimestamp)/80000) ; previousTimestamp = Timestamp;

	      float timestampMultiplier = 1.109*128.0/float(numColumns);
#             ifdef USE_ROOT	      
	      h1->Fill( deltaFortisTimestamp , deltaTimestamp );
	      h2->Fill( deltaTimestamp , ( float(deltaFortisTimestamp)/float(deltaTimestamp) ) );
	      h3->Fill( previousDeltaTimestamp , deltaTimestamp );
	      h4->Fill( float(deltaFortisTimestamp) - float(deltaTimestamp)*timestampMultiplier );
	      h5->Fill( deltaTimestamp );
#             endif

	      // std::cout << "event , TLU Timestamp , event " << eventNumber  << " \t" << Timestamp << " Fortis frame, row = "<<fortisFrameNumber << "\t" << fortisPivotRow << " Fortis timestamp = " <<  FortisTimestamp <<  std::endl;
	      // std::cout << "delta fortis timestamp , delta TLU timestamp " << deltaFortisTimestamp << "\t" << deltaTimestamp << std::endl;
	    
	      previousDeltaTimestamp = deltaTimestamp;

	      timestampStream << runNumber << "\t" << eventNumber << "\t" << Timestamp << "\t"  << fortisFrameNumber << "\t"  << fortisPivotRow << std::endl;
	    }
	  }
	}
	}
      } // end of file loop


#   ifdef USE_ROOT
    // set histogram parameters
    h1->SetTitle("TLU Timestamp difference vs. FORTIS Timestamp difference");
    h1->GetYaxis()->SetTitle("delta-TLUTimestamp");
    h1->GetXaxis()->SetTitle("delta-FORTISTimestamp");

    h2->SetTitle("FORTIS/TLU-Timestamp vs.TLU Timestamp difference");
    h2->GetYaxis()->SetTitle("(delta-TLUTimestamp)/(delta-TLUTimestamp)");
    h2->GetXaxis()->SetTitle("delta-TLUTimestamp");

    RootFile.Write();
#   endif

    timestampStream.close();

  } catch (...) {      
    return op.HandleMainException();
  }
     

  return 0;

} // end of main

