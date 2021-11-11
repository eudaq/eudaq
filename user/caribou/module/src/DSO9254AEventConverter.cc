#include "CaribouEvent2StdEventConverter.hh"
#include "utils/log.hpp"
#include "CLICTDFrameDecoder.hpp"
#include "TFile.h"
#include "TH1D.h"


using namespace eudaq;

namespace{
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::
  Register<DSO9254AEvent2StdEventConverter>(DSO9254AEvent2StdEventConverter::m_id_factory);
}


bool DSO9254AEvent2StdEventConverter::Converting(eudaq::EventSPC d1, eudaq::StandardEventSP d2, eudaq::ConfigurationSPC conf) const{
  

  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);


  // FIXME load from config file
  // integration window for pedestal and waveform
  double pedestalStartTime = -70; //[ns]
  double pedestalEndTime =   -20;
    
  
  // No event
  if(!ev) {
    return false;
  }


  // Data container:
  caribou::pearyRawData rawdata;

  // internal containers
  std::vector<std::vector<double>> waves;
  std::vector<double> time;  
  std::vector<double> ped;
  std::vector<double> amp;
  
  // waveform parameters
  std::vector<uint64_t> np;
  std::vector<double> dx;
  std::vector<double> x0;
  std::vector<double> dy;
  std::vector<double> y0;
  
  
  // FIXME this should become optional at some point
  TFile * histoFile = new TFile( "waveforms.root", "UPDATE" ); // UPDATE?
  if(!histoFile) {
    LOG(ERROR) << "ERROR: " << histoFile->GetName() << " can not be opened";
    return false;
  }

  
  // Retrieve data from event
  if(ev->NumBlocks() == 1) {
    
    // all four scope channels in one data block
    auto datablock = ev->GetBlock(0);
    LOG(DEBUG) << "DSO9254A frame with "<< datablock.size()<<" entries";

      
    // Calulate positions and length of data blocks:
    // FIXME FIXME FIXME by Simon: this is prone to break since you are selecting bits from a 64bit word - but there is no guarantee in the endianness here and you might end up having the wrong one. Also, there is no guarantee for all four channels to have the same preamble and data length - theu should, but better check...
    
    
    rawdata.resize( sizeof(datablock[0]) * datablock.size() / sizeof(uintptr_t) );
    std::memcpy(&rawdata[0], &datablock[0], sizeof(datablock[0]) * datablock.size() );
    
    uint64_t block_words;
    uint64_t pream_words;
    uint64_t chann_words;
    
    // loop 4 channels
    for( int nch = 0; nch < 4; nch++ ){
      
      std::cout << "Reading channel " << nch << std::endl;
      
      // container for one waveform
      std::vector<double> wave;
      
      block_words = rawdata[0 + nch * (block_words + 1 )];
      pream_words = rawdata[1 + nch * (block_words + 1 )];
      chann_words = rawdata[2 + nch * (block_words + 1 ) + pream_words ];
      std::cout << "        " << block_words << " words per waveform\n";
      std::cout << "        " << pream_words << " words per preamble\n";
      std::cout << "        " << chann_words << " words per channel data\n";
      std::cout << "        " << rawdata.size() << " size of data rawdata\n";
           

      // read preamble:
      // is this better?
      // in fetch_channel_data we memcpy a dataVector_type? but the querry does return a string...
      // I did not manage to get the entire string though
      std::string preamble = "";
      for(int i=2 + nch * (block_words+1); 
              i<2 + nch * (block_words+1) + pream_words;
              i++ ){
        preamble += (char)rawdata[i];
      }
      LOG(DEBUG) << preamble;
      std::cout << preamble << std::endl;
        
      // parse preamble to vector of strings
      std::string s;
      std::vector<std::string> vals;
      std::istringstream stream(preamble);
      while(std::getline(stream,s,',')){
        vals.push_back(s);
      }
      
      // Time might still help for online sync
      date.push_back(vals[15]);
      clockTime.push_back(vals[16]);
      timeSinceStart.push_back(getTimeSinceStart(date[nch], clockTime[nch]));
      
      // pick preamble elements
      np.push_back( stoi( vals[2]) );
      dx.push_back( stod( vals[4]) * 1e9 );
      x0.push_back( stod( vals[5]) * 1e9 );
      dy.push_back( stod( vals[7]) );
      y0.push_back( stod( vals[8]) );
      int points_per_words = np.at(nch)/chann_words;
      if( np.at(nch)%chann_words ) LOG(WARNING) << "incomplete waveform";
      
      std::cout << "  dx " << dx.at(nch) << std::endl
		<< "  x0 " << x0.at(nch) << std::endl
		<< "  dy " << dy.at(nch) << std::endl
		<< "  y0 " << y0.at(nch) << std::endl;

      
      // prepare histogram with corresponding binning
      TH1D* hist = new TH1D(
			    Form( "waveform_run%i_ev%i_ch%i", ev->GetRunN(), ev->GetEventN(), nch ),
			    Form( "waveform_run%i_ev%i_ch%i", ev->GetRunN(), ev->GetEventN(), nch ),
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
      // loop waveform part of the data block
      for(int i=pream_words+3+nch*(block_words+1); 
              i<pream_words+3+nch*(block_words+1)+chann_words;
              i++){

	// coppy channel data from whole data block
	memcpy(&words.front(), &rawdata[i], points_per_words*sizeof(int16_t));

	for( auto word : words ){

	  // fill vectors with time bins and waveform
	  if( nch == 0 ){ // this we need only once
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
      
      // vector of histograms needs explicit write commad
      hist->Write();
      
    } // for channel
    
  } // if numblock
  
  
  
  // re-iterate waveforms
  std::ofstream file( "waveforms.txt");
  for( int i = 0; i < time.size(); i++ ) {
    
    try{

      // calculate pedestal
      if( time.at(i) >= pedestalStartTime && time.at(i) < pedestalEndTime ){
        ped.at(0) += waves.at(0).at(i);
        ped.at(1) += waves.at(1).at(i);
        ped.at(2) += waves.at(2).at(i);
        ped.at(3) += waves.at(3).at(i);
      }

      // get maximum amplitude in range
      if( time.at(i) >= amplitudeStartTime && time.at(i) < amplitudeEndTime ){
        if( amp.at(0) < waves.at(0).at(i) ) amp.at(0) = waves.at(0).at(i);
        if( amp.at(1) < waves.at(1).at(i) ) amp.at(1) = waves.at(1).at(i);
        if( amp.at(2) < waves.at(2).at(i) ) amp.at(2) = waves.at(2).at(i);
        if( amp.at(3) < waves.at(3).at(i) ) amp.at(3) = waves.at(3).at(i);
      }
    
    }catch(...){

      // FIXME why are some data blocks shorter than the preamble suggests?
      std::cout << "Am I still needed?" << std::endl; 

    }

  } // time bins
  

  // Create a StandardPlane representing one sensor plane
  eudaq::StandardPlane plane(0, "Caribou", "DSO9254A");
  plane.SetSizeZS(2, 2, 0);
  plane.PushPixel( 1, 1, amp.at(0), 0 ); // FIXME 0 -> propper time stamp
  plane.PushPixel( 1, 0, amp.at(1), 0 );
  plane.PushPixel( 0, 0, amp.at(2), 0 );
  plane.PushPixel( 0, 1, amp.at(3), 0 );
  // Add the plane to the StandardEvent
  d2->AddPlane(plane);
  // Identify the detetor type
  d2->SetDetectorType("DSO9254A");


  // write histograms
  histoFile->Close();
  LOG(INFO) << "Histograms written to " << histoFile->GetName();

  
  // Indicate that data were successfully converted
  return true;
}




