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


bool   DSO9254AEvent2StdEventConverter::m_configured(0);
double DSO9254AEvent2StdEventConverter::m_pedStartTime(0);
double DSO9254AEvent2StdEventConverter::m_pedEndTime(0);
double DSO9254AEvent2StdEventConverter::m_ampStartTime(0);
double DSO9254AEvent2StdEventConverter::m_ampEndTime(0);
double DSO9254AEvent2StdEventConverter::m_chargeScale(0);
double DSO9254AEvent2StdEventConverter::m_chargeCut(0);
bool   DSO9254AEvent2StdEventConverter::m_generateRoot(0);


bool DSO9254AEvent2StdEventConverter::Converting(eudaq::EventSPC d1, eudaq::StandardEventSP d2, eudaq::ConfigurationSPC conf) const{


  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);
  // No event
  if(!ev) {
    return false;
  }

  // load parameters from config file
  if( !m_configured ){

    m_pedStartTime = conf->Get("pedStartTime", 0 ); // integration windows in [ns]
    m_pedEndTime   = conf->Get("pedEndTime"  , 0 );
    m_ampStartTime = conf->Get("ampStartTime", 0 );
    m_ampEndTime   = conf->Get("ampEndTime"  , 0 );
    m_chargeScale  = conf->Get("chargeScale" , 0 );
    m_chargeCut    = conf->Get("chargeCut"   , 0 );
    m_generateRoot   = conf->Get("generateRoot", 0 );

    EUDAQ_DEBUG( "Loaded parameters from configuration file." );
    EUDAQ_DEBUG( "  pedStartTime = " + to_string( m_pedStartTime ) + " ns" );
    EUDAQ_DEBUG( "  pedEndTime   = " + to_string( m_pedEndTime ) + " ns" );
    EUDAQ_DEBUG( "  ampStartTime = " + to_string( m_ampStartTime ) + " ns" );
    EUDAQ_DEBUG( "  ampEndTime   = " + to_string( m_ampEndTime ) + " ns" );
    EUDAQ_DEBUG( "  chargeScale  = " + to_string( m_chargeScale ) + " a.u." );
    EUDAQ_DEBUG( "  chargeCut    = " + to_string( m_chargeCut ) + " a.u." );
    EUDAQ_DEBUG( "  generateRoot = " + to_string( m_generateRoot ) );

    // check configuration
    bool noData = false;
    bool noHist = false;
    if( m_pedStartTime == 0 &&
        m_pedEndTime   == 0 &&
        m_ampStartTime == 0 &&
        m_ampEndTime   == 0 ){
      EUDAQ_WARN( "pedStartTime, pedEndTime, ampStartTime and ampEndTime are unconfigured. Converter returns no data. Introduce configuration file with these values!" );
      noData = true;
    }
    if( !m_generateRoot ){
      EUDAQ_WARN( "No waveform file generated!" );
      noHist = true;
    }
    if( noData && noHist ){
      EUDAQ_ERROR( "Writing no output data and no root file... Abort");
      return false;
    }
    m_configured = true;

  } // configure


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


  // Bad event
  if(ev->NumBlocks() != 1) {
    EUDAQ_WARN("Ignoring bad packet " + std::to_string(ev->GetEventNumber()));
    return false;
  }


  // generate rootfile to write waveforms as TH1D
  TFile * histoFile = nullptr;
  if( m_generateRoot ){
    histoFile = new TFile( Form( "waveforms_run%i.root", ev->GetRunN() ), "UPDATE" );
    if(!histoFile) {
      EUDAQ_ERROR(to_string(histoFile->GetName()) + " can not be opened");
      return false;
    }
  }


  // all four scope channels in one data block
  auto datablock = ev->GetBlock(0);
  EUDAQ_DEBUG("DSO9254A frame with " + to_string(datablock.size()) + " entries");


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

    EUDAQ_DEBUG( "Reading channel " + to_string(nch));

    // Obtaining Data Stucture
    block_words = rawdata[block_posit + 0];
    pream_words = rawdata[block_posit + 1];
    chann_words = rawdata[block_posit + 2 + pream_words];
    EUDAQ_DEBUG( "  " + to_string(block_posit) + " current position in block");
    EUDAQ_DEBUG( "  " + to_string(block_words) + " words per segment");
    EUDAQ_DEBUG( "  " + to_string(pream_words) + " words per preamble");
    EUDAQ_DEBUG( "  " + to_string(chann_words) + " words per channel data");
    EUDAQ_DEBUG( "  " + to_string(rawdata.size()) + "size of data rawdata");


    // read preamble:
    std::string preamble = "";
    for(int i=block_posit + 2;
        i<block_posit + 2 + pream_words;
        i++ ){
      preamble += (char)rawdata[i];
    }
    EUDAQ_DEBUG("Preamble");
    EUDAQ_DEBUG("  " + preamble);
    // parse preamble to vector of strings
    std::string s;
    std::vector<std::string> vals;
    std::istringstream stream(preamble);
    while(std::getline(stream,s,',')){
      vals.push_back(s);
    }


    // getting timestamps from preamble which is rather imprecise
    timestamp = DSO9254AEvent2StdEventConverter::timeConverter( vals[15], vals[16] );


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
    if( np.at(nch)%chann_words/sgmnt_count ) EUDAQ_WARN("incomplete waveform");


    // need once per channel
    std::vector<double> ped;
    std::vector<double> amp;
    std::vector<std::vector<double>> waves;


    for( int s=0; s<sgmnt_count; s++){ // loop semgents

      // container for one waveform
      std::vector<double> wave;

      // prepare histogram with corresponding binning if root file is created
      TH1D* hist = nullptr;
      if( m_generateRoot ){
        TH1D* hist_init = new TH1D(
              Form( "waveform_run%i_ev%i_ch%i_s%i", ev->GetRunN(), ev->GetEventN(), nch, s ),
              Form( "waveform_run%i_ev%i_ch%i_s%i", ev->GetRunN(), ev->GetEventN(), nch, s ),
              np.at(nch),
              x0.at(nch) - dx.at(nch)/2.,
              x0.at(nch) - dx.at(nch)/2. + dx.at(nch)*np.at(nch)
        );
        hist = hist_init;
        hist->GetXaxis()->SetTitle("time [ns]");
        hist->GetYaxis()->SetTitle("signal [V]");
      }

      // read channel data
      std::vector<int16_t> words;
      words.resize(points_per_words); // rawdata contains 8 byte words, scope sends 2 byte words
      int16_t wfi = 0;
      for(int i=block_posit + 3 + pream_words + (s+0)*chann_words/sgmnt_count;
          i<block_posit + 3 + pream_words + (s+1)*chann_words/sgmnt_count;
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
          if( m_generateRoot ){
            hist->SetBinContent( hist->FindBin( wfi * dx.at(nch) + x0.at(nch) ),
                                 (double)word * dy.at(nch) + y0.at(nch) );
          }

          wfi++;

        } // words

      } // waveform


      // store waveform vector and create vector entries for pedestal and amplitude
      waves.push_back( wave );
      ped.push_back( 0. );
      amp.push_back( 0. );

      // write and delete
      if( m_generateRoot ){
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


  // process waveform data
  if( time.size() > 0 ){ // only if there are waveform data


    // re-iterate waveforms
    // wavess.at(channel).at(segment).at(point)
    // ped   .at(channel).at(segment)
    for( int s = 0; s<peds.at(0).size(); s++ ){
      for( int c = 0; c<peds.size(); c++ ){

        // calculate pedestal
        int i_pedStart = (m_pedStartTime - time.at(0))/dx.at(c);
        int i_pedEnd = (m_pedEndTime - time.at(0))/dx.at(c);
        int n_ped = (m_pedEndTime-m_pedStartTime)/dx.at(c);
        for( int p = i_pedStart; p<i_pedEnd; p++ ){
          peds.at(c).at(s) += wavess.at(c).at(s).at(p) / n_ped;
        }

        // calculate maximum amplitude in range
        int i_ampStart = (m_ampStartTime - time.at(0))/dx.at(c);
        int i_ampEnd = (m_ampEndTime - time.at(0))/dx.at(c);
        for( int p = i_ampStart; p<i_ampEnd; p++ ){
          if( amps.at(c).at(s) < wavess.at(c).at(s).at(p) - peds.at(c).at(s) ){
            amps.at(c).at(s) = wavess.at(c).at(s).at(p) - peds.at(c).at(s);
          }
        }

      } // channels

      // check:
      for( int c = 0; c<peds.size(); c++ ){
        EUDAQ_DEBUG("CHECK: channel "+ to_string(c) + " segment " + to_string(s));
        EUDAQ_DEBUG("  points  " + to_string(wavess.at(c).at(s).size()));
        EUDAQ_DEBUG("  pedestl " + to_string(peds.at(c).at(s)));
        EUDAQ_DEBUG("  ampli   " + to_string(amps.at(c).at(s)));
        EUDAQ_DEBUG("  timestp " + to_string(timestamp));
      }


    } // segments


    // Create a StandardPlane representing one sensor plane
    // FIXME For now pushing just the first segment... need to decide how to treat several of them
    eudaq::StandardPlane plane(0, "Caribou", "DSO9254A");

    // this scope has only 4 channels
    std::map<int,std::vector<int>> chanToPix;
    chanToPix[0] = {1,1};
    chanToPix[1] = {1,0};
    chanToPix[2] = {0,0};
    chanToPix[3] = {0,1};

    // fill plane
    plane.SetSizeZS(2, 2, 0);
    for( int c = 0; c<peds.size(); c++ ){
      if( amps.at(c).at(0)*m_chargeScale > m_chargeCut ){
        plane.PushPixel( chanToPix[c].at(0), chanToPix[c].at(1), amps.at(c).at(0)*m_chargeScale, timestamp );
      }
    }
    d2->AddPlane(plane);
    // Identify the detetor type
    d2->SetDetectorType("DSO9254A");


    // forcing corry to fall back on trigger IDs
    // FIXME is this needed? the right way to do it?
    d2->SetTimeBegin(0);
    d2->SetTimeEnd(0);
    d2->SetTriggerN(ev->GetEventN());


    // close rootfile
    if( m_generateRoot ){
      histoFile->Close();
      EUDAQ_DEBUG("Histograms written to " + to_string(histoFile->GetName()));
    }

  }
  else{

    // close rootfile anyway
    if( m_generateRoot ){
      histoFile->Close();
      EUDAQ_DEBUG("Histograms written to " + to_string(histoFile->GetName()));
    }


    EUDAQ_WARN("Warning: No scope data in event " + to_string(ev->GetEventN()));
    return false;
  }


  // Indicate that data were successfully converted
  return true;
}


uint64_t DSO9254AEvent2StdEventConverter::timeConverter( std::string date, std::string time ){

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

  // fill time structure
  struct std::tm build_time = {0};
  build_time.tm_mday = stoi( parts.at(0) );       // day in month 1-31
  build_time.tm_year = stoi( parts.at(2) )-1900;  // years since 1900
  build_time.tm_hour = stoi( parts.at(3) );       // hours since midnight 0-23
  build_time.tm_min  = stoi( parts.at(4) );       // minutes 0-59
  build_time.tm_sec  = stoi( parts.at(5) );       // seconds 0-59
  build_time.tm_mon  = monthToNum[parts.at(1)]-1; // month 0-11
  if( monthToNum[parts.at(1)]==0 ){ // failed conversion
    EUDAQ_ERROR(" in timeConverter, month " + to_string(parts.at(1)) + " not defined!");
  }

  // now convert to unix timestamp, to pico seconds and add sub-second information from scope
  std::time_t tunix;
  tunix = std::mktime( &build_time );
  uint64_t result = tunix * 1e9;                // [   s->ps]
  uint64_t mill10 = stoi( parts.at(6) ) * 1e7 ; // [10ms->ps]
  result += mill10;

  return result;
}
