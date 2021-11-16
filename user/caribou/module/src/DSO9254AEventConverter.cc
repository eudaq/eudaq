#include "CaribouEvent2StdEventConverter.hh"
#include "utils/log.hpp"
#include "CLICTDFrameDecoder.hpp"
#include "TFile.h"
#include "TH1D.h"
#include "time.h"

using namespace eudaq;

namespace{
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::
  Register<DSO9254AEvent2StdEventConverter>(DSO9254AEvent2StdEventConverter::m_id_factory);
}

uint64_t timeConverter( std::string date, std::string time );

bool DSO9254AEvent2StdEventConverter::Converting(eudaq::EventSPC d1, eudaq::StandardEventSP d2, eudaq::ConfigurationSPC conf) const{
  

  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);
  // No event
  if(!ev) {
    return false;
  }


  // load parameters from config file
  double pedStartTime = conf->Get("pedStartTime", 0 ); // integration windows in [ns]
  double pedEndTime   = conf->Get("pedEndTime"  , 0 );
  double ampStartTime = conf->Get("ampStartTime", 0 );
  double ampEndTime   = conf->Get("ampEndTime"  , 0 );
  bool generateRoot   = conf->Get("generateRoot", 0 ); 
  std::cout << "Loaded parameters from configuration file." << std::endl
	    << "  pedStartTime = " << pedStartTime << std::endl
	    << "  pedEndTime   = " << pedEndTime   << std::endl
	    << "  ampStartTime = " << ampStartTime << std::endl
	    << "  ampEndTime   = " << ampEndTime   << std::endl
	    << "  generateRoot = " << generateRoot << std::endl << std::endl;
  

  // Data container:
  caribou::pearyRawData rawdata;

  // internal containers
  std::vector<std::vector<std::vector<double>>> wavess;
  std::vector<std::vector<double>> peds;
  std::vector<std::vector<double>> amps;
  std::vector<double> time;
  uint64_t timestamp;

  // waveform parameters
  std::vector<uint64_t> np;
  std::vector<double> dx;
  std::vector<double> x0;
  std::vector<double> dy;
  std::vector<double> y0;

  
  // generate rootfile to write waveforms as TH1D
  TFile * histoFile = nullptr;
  if( generateRoot ){
    histoFile = new TFile( "waveforms.root", "UPDATE" ); // UPDATE?
    if(!histoFile) {
      LOG(ERROR) << "ERROR: " << histoFile->GetName() << " can not be opened";
      return false;
    }
  }
						       
  
  // Retrieve data from event
  if(ev->NumBlocks() == 1) {

    
    // all four scope channels in one data block
    auto datablock = ev->GetBlock(0);
    LOG(DEBUG) << "DSO9254A frame with "<< datablock.size()<<" entries";

    
    // Calulate positions and length of data blocks:
    // FIXME FIXME FIXME by Simon: this is prone to break since you are selecting bits from a 64bit
    //                             word - but there is no guarantee in the endianness here and you
    //                             might end up having the wrong one. 
    // Finn: I am not sure if I actually solved this issue! Lets discuss.    
    rawdata.resize( sizeof(datablock[0]) * datablock.size() / sizeof(uintptr_t) );
    std::memcpy(&rawdata[0], &datablock[0], sizeof(datablock[0]) * datablock.size() );

    
    // needed per event
    uint64_t block_words;
    uint64_t pream_words;
    uint64_t chann_words;
    uint64_t block_posit = 0;
    uint64_t sgmnt_count = 1;

    
    // loop 4 channels
    for( int nch = 0; nch < 4; nch++ ){      

      std::cout << "Reading channel " << nch << std::endl;

      // Obtaining Data Stucture
      block_words = rawdata[block_posit + 0];
      pream_words = rawdata[block_posit + 1];
      chann_words = rawdata[block_posit + 2 + pream_words];
      std::cout << "  " << block_posit << " current position in block\n";
      std::cout << "  " << block_words << " words per segment\n";
      std::cout << "  " << pream_words << " words per preamble\n";
      std::cout << "  " << chann_words << " words per channel data\n";
      std::cout << "  " << rawdata.size() << " size of data rawdata\n";
           

      // read preamble:
      std::string preamble = "";
      for(int i=block_posit + 2;
              i<block_posit + 2 + pream_words;
              i++ ){
        preamble += (char)rawdata[i];
      }
      LOG(DEBUG) << preamble;
      std::cout << "Preamble: \n  " << preamble << std::endl;  
      // parse preamble to vector of strings
      std::string s;
      std::vector<std::string> vals;
      std::istringstream stream(preamble);
      while(std::getline(stream,s,',')){
        vals.push_back(s);
      }
      

      // getting timestamps from preamble which is rather imprecise
      timestamp = timeConverter( vals[15], vals[16] );

      
      // Pick needed preamble elements
      np.push_back( stoi( vals[2]) );
      dx.push_back( stod( vals[4]) * 1e9 );
      x0.push_back( stod( vals[5]) * 1e9 );
      dy.push_back( stod( vals[7]) );
      y0.push_back( stod( vals[8]) );
      if( vals.size() == 25 ) {// this is segmented mode, possibly more than one waveform in block
	sgmnt_count = stoi( vals[24] );
      }
      int points_per_words = np.at(nch)/(chann_words/sgmnt_count);
      if( np.at(nch)%chann_words/sgmnt_count ) LOG(WARNING) << "incomplete waveform";
      /*
      std::cout << "  dx " << dx.at(nch) << std::endl
		<< "  x0 " << x0.at(nch) << std::endl
		<< "  dy " << dy.at(nch) << std::endl
		<< "  y0 " << y0.at(nch) << std::endl;
      */

      // need once per channel
      std::vector<double> ped;
      std::vector<double> amp;
      std::vector<std::vector<double>> waves;
      
      
      for( int s=0; s<sgmnt_count; s++){ // loop semgents

     	// container for one waveform
	std::vector<double> wave;
	
	// prepare histogram with corresponding binning
	TH1D* hist = new TH1D(
			      Form( "waveform_run%i_ev%i_ch%i_s%i", ev->GetRunN(), ev->GetEventN(),
				    nch, s ),
			      Form( "waveform_run%i_ev%i_ch%i_s%i", ev->GetRunN(), ev->GetEventN(),
				    nch, s ),
			      np.at(nch),
			      x0.at(nch) - dx.at(nch)/2.,
			      x0.at(nch) - dx.at(nch)/2. + dx.at(nch)*np.at(nch)
			      );
	hist->GetXaxis()->SetTitle("time [ns]");
	hist->GetYaxis()->SetTitle("signal [V]");
      
	// read channel data
	std::vector<int16_t> words;
	words.resize(points_per_words); // rawdata contains 8 byte words, scope sends 2 byte words 
	int16_t wfi = 0;
	for(int i=block_posit + 3 + pream_words + (s+0)*chann_words/sgmnt_count;
	        i<block_posit + 3 + pream_words + (s+1)*chann_words/sgmnt_count;
	      //i<block_posit + 3 + pream_words+chann_words;
	        i++){
	  
	  // coppy channel data from whole data block
	  memcpy(&words.front(), &rawdata[i], points_per_words*sizeof(int16_t));
	  
	  for( auto word : words ){
	    
	    // fill vectors with time bins and waveform
	    if( nch == 0 && s == 0 ){ // this we need only once
	      time.push_back( wfi * dx.at(nch) + x0.at(nch) );
	    }
	    wave.push_back( (word * dy.at(nch) + y0.at(nch)) );
	    
	    
	    // fill histogram
	    hist->SetBinContent(  hist->FindBin( wfi * dx.at(nch) + x0.at(nch) ),
	    			  (double)word * dy.at(nch) + y0.at(nch) );
	    
	    wfi++;
	    
	  } // words
	  
	} // waveform

	
	// store waveform vector and create vector entries for pedestal and amplitude
	waves.push_back( wave );
	ped.push_back( 0. );
	amp.push_back( 0. );

	// write and delete
	if( generateRoot ){
	  hist->Write();
	}
	delete hist;

	
      } // segments


      // not only for each semgent but for each channel
      wavess.push_back( waves );
      peds.push_back( ped );
      amps.push_back( amp );
	

      // update position for next iteration
      block_posit += block_words+1;
    } // for channel
    
  } // if numblock


  // process waveform data
  if( time.size() > 0 ){ // only if there are waveform data
  

    // re-iterate waveforms
    // wavess.at(channel).at(segment).at(point)
    // ped   .at(channel).at(segment)
    for( int s = 0; s<peds.at(0).size(); s++ ){
      for( int p = 0; p<time.size(); p++ ) {      	
	
	//try{ // hotfix in case off incomplete waveforms
	  
	// calculate pedestal
	if( time.at(p) >= pedStartTime && time.at(p) < pedEndTime ){
	  for( int c = 0; c<peds.size(); c++ ) 
	    peds.at(c).at(s) += wavess.at(c).at(s).at(p);
	}
	
	// get maximum amplitude in range
	if( time.at(p) >= ampStartTime && time.at(p) < ampEndTime ){
	  for( int c = 0; c<peds.size(); c++ ){ 
	    if( amps.at(c).at(s) < wavess.at(c).at(s).at(p) - peds.at(c).at(s) )
	      amps.at(c).at(s) = wavess.at(c).at(s).at(p) - peds.at(c).at(s);
	  }
	}

	//}catch(...){} 
	
      } // points
      
      
      // check:
      /*
      std::cout << std::endl;
      for( int c = 0; c<peds.size(); c++ ){
	std::cout << "  CHECK: channel " << c << " segment " << s << std::endl 
	          << "         points  " << wavess.at(c).at(s).size() << std::endl
		  << "         pedestl " << peds.at(c).at(s) << std::endl
		  << "         ampli   " << amps.at(c).at(s) << std::endl
		  << "         timestp " << timestamp << std::endl;
      }
      */
  
    } // segments
    
    
    // Create a StandardPlane representing one sensor plane
    // FIXME For now pushing just the first segment... need to decide how to treat several of them
    eudaq::StandardPlane plane(0, "Caribou", "DSO9254A");
    plane.SetSizeZS(2, 2, 0);
    plane.PushPixel( 1, 1, amps.at(0).at(0), timestamp );
    plane.PushPixel( 1, 0, amps.at(1).at(0), timestamp );
    plane.PushPixel( 0, 0, amps.at(2).at(0), timestamp );
    plane.PushPixel( 0, 1, amps.at(3).at(0), timestamp );
    // Add the plane to the StandardEvent
    d2->AddPlane(plane);
    // Identify the detetor type
    d2->SetDetectorType("DSO9254A");

    
    // close rootfile
    if( generateRoot ){
      histoFile->Close();
      std::cout << "Histograms written to " << histoFile->GetName() << std::endl;
    }
    
  }  
  else{
    
    // close rootfile anyway
    if( generateRoot ){
      histoFile->Close();
      std::cout << "Histograms written to " << histoFile->GetName() << std::endl;
    }

    
    std::cout << "Warning: No scope data in event " << ev->GetEventN() << std::endl;
    return false;
  }


  
  // Indicate that data were successfully converted
  return true;
}


uint64_t timeConverter( std::string date, std::string time ){

  // needed for month conversion
  std::map<std::string, int> monthToNum {
    {"JAN",  1},
    {"FEB",  2},
    {"MAR",  3},
    {"APR",  4},
    {"MAI",  5},
    {"JUN",  6},
    {"JUL",  7},
    {"AUG",  8},
    {"SEP",  9},
    {"OCT", 10},
    {"NOV", 11},
    {"DEC", 12},
  };
  
  // delete leading and tailing "
  date.erase( 0, 1);
  date.erase( date.size()-1, 1 );
  time.erase( 0, 1);
  time.erase( time.size()-1, 1 );

  // get day, month ... in one vector of strings
  std::string s;
  std::istringstream s_date( date );
  std::istringstream s_time( time );
  std::vector<std::string> parts;
  while(std::getline(s_date,s,' ')) parts.push_back(s);
  while(std::getline(s_time,s,':')) parts.push_back(s);
  //for( auto part : parts ) std::cout << part << " ";
  //std::cout << std::endl;

  // fill time structure
  struct std::tm build_time = {0};
  build_time.tm_mday = stoi( parts.at(0) );       // day in month 1-31
  build_time.tm_year = stoi( parts.at(2) )-1900;  // years since 1900
  build_time.tm_hour = stoi( parts.at(3) );       // hours since midnight 0-23
  build_time.tm_min  = stoi( parts.at(4) );       // minutes 0-59
  build_time.tm_sec  = stoi( parts.at(5) );       // seconds 0-59
  build_time.tm_mon  = monthToNum[parts.at(1)]-1; // month 0-11
  if( monthToNum[parts.at(1)]==0 ){ // failed conversion
    std::cout << "ERROR in timeConverter, month " << parts.at(1) << " not defined!" << std::endl;
  }

  // now convert to unix timestamp, to pico seconds and add sub-second information from scope
  std::time_t tunix;
  tunix = std::mktime( &build_time );
  uint64_t result = tunix * 1e9;                // [   s->ps]
  uint64_t mill10 = stoi( parts.at(6) ) * 1e7 ; // [10ms->ps]
  result += mill10;
   
  return result;
}
