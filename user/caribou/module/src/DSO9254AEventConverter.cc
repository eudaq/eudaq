#include "CaribouEvent2StdEventConverter.hh"
#include "utils/log.hpp"
#include "dSiPMFrameDecoder.hpp"
#include "TFile.h"
#include "TH1D.h"
#include "time.h"

using namespace eudaq;

namespace {
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::Register<
      DSO9254AEvent2StdEventConverter>(
      DSO9254AEvent2StdEventConverter::m_id_factory);
}


bool DSO9254AEvent2StdEventConverter::m_configured(0);
bool DSO9254AEvent2StdEventConverter::m_hitbus(0);
uint64_t DSO9254AEvent2StdEventConverter::m_trigger(0);
int64_t DSO9254AEvent2StdEventConverter::m_runStartTime(-1);
double DSO9254AEvent2StdEventConverter::m_pedStartTime(0);
double DSO9254AEvent2StdEventConverter::m_pedEndTime(0);
double DSO9254AEvent2StdEventConverter::m_ampStartTime(0);
double DSO9254AEvent2StdEventConverter::m_ampEndTime(0);
double DSO9254AEvent2StdEventConverter::m_chargeScale(0);
double DSO9254AEvent2StdEventConverter::m_chargeCut(0);

bool DSO9254AEvent2StdEventConverter::m_generateRoot(0);

bool DSO9254AEvent2StdEventConverter::Converting(eudaq::EventSPC d1, eudaq::StandardEventSP d2, eudaq::ConfigurationSPC conf) const{


  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);
  // No event
  if(!ev) {
    return false;
  }

  // load parameters from config file
  std::ofstream outfileTimestamps;

  if (!m_configured) {
    // clean the histogram once to not always append
    m_generateRoot = conf->Get("generateRoot", 0);
    TFile *histoFile = nullptr;
    if (m_generateRoot) {
      histoFile =
          new TFile(Form("waveforms_run%i.root", ev->GetRunN()), "RECREATE");
      if (!histoFile) {
        EUDAQ_ERROR(to_string(histoFile->GetName()) + " can not be opened");
        return false;
      }
      histoFile->Close();
    }
    m_hitbus = conf->Get("hitbus", 0);

    if (m_hitbus) {
      EUDAQ_WARN("Reading hitbus like data assuming <Ariannas order that I do "
                 "not know now :)>");
    }

    // read from config file
    m_pedStartTime = conf->Get("pedStartTime", 0); // integration windows in [ns]
    m_pedEndTime   = conf->Get("pedEndTime"  , 0);
    m_ampStartTime = conf->Get("ampStartTime", 0);
    m_ampEndTime   = conf->Get("ampEndTime"  , 0);
    m_chargeScale  = conf->Get("chargeScale" , 0);
    m_chargeCut    = conf->Get("chargeCut"   , 0);

    EUDAQ_DEBUG( "Loaded parameters from configuration file." );
    EUDAQ_DEBUG( "  pedStartTime = " + to_string( m_pedStartTime ) + " ns" );
    EUDAQ_DEBUG( "  pedEndTime   = " + to_string( m_pedEndTime ) + " ns" );
    EUDAQ_DEBUG( "  ampStartTime = " + to_string( m_ampStartTime ) + " ns" );
    EUDAQ_DEBUG( "  ampEndTime   = " + to_string( m_ampEndTime ) + " ns" );
    EUDAQ_DEBUG( "  chargeScale  = " + to_string( m_chargeScale ) + " a.u." );
    EUDAQ_DEBUG( "  chargeCut    = " + to_string( m_chargeCut ) + " a.u." );
    EUDAQ_DEBUG( "  generateRoot = " + to_string( m_generateRoot ) );

    // check configuration - only if we do not have hitbus data
    if(!m_hitbus){
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
  uint64_t block_position = 0;
  uint64_t sgmnt_count = 1;
  uint nChannels = 4;
  // loop 4 channels
  for( int nch = 0; nch < nChannels; nch++ ){

    EUDAQ_DEBUG( "Reading channel " + to_string(nch));

    // Obtaining Data Stucture
    block_words = rawdata[block_position + 0];
    pream_words = rawdata[block_position + 1];
    chann_words = rawdata[block_position + 2 + pream_words];
    EUDAQ_DEBUG( "  " + to_string(datablock.size()) + " 8 bit block size");
    EUDAQ_DEBUG( "  " + to_string(block_position) + " current position in block");
    EUDAQ_DEBUG( "  " + to_string(block_words) + " words per data block");
    EUDAQ_DEBUG( "  " + to_string(pream_words) + " words per preamble");
    EUDAQ_DEBUG( "  " + to_string(chann_words) + " words per channel data");
    EUDAQ_DEBUG( "  " + to_string(rawdata.size()) + " size of rawdata");

    // Check our own sixth-graders math: The block header minus peamble minus channel data minus the counters is zero
    if(block_words - pream_words - 2 - chann_words) {
      EUDAQ_WARN("Go back to school, 6th grade - math doesn't check out! :/");
    }

    // read preamble:
    std::string preamble = "";
    for(int i=block_position + 2;
        i<block_position + 2 + pream_words;
        i++ ){
      preamble += (char)rawdata[i];
    }
    EUDAQ_DEBUG("Preamble");
    EUDAQ_DEBUG("  " + preamble);
    // parse preamble to vector of strings
    std::string val;
    std::vector<std::string> vals;
    std::istringstream stream(preamble);
    while(std::getline(stream,val,',')){
      vals.push_back(val);
      EUDAQ_DEBUG( " " + to_string(vals.size()-1) + ": " + to_string(vals.back()));
    }

    // Pick needed preamble elements
    np.push_back( stoi( vals[2]) );
    dx.push_back( stod( vals[4]) * 1e9 );
    x0.push_back( stod( vals[5]) * 1e9 );
    dy.push_back( stod( vals[7]) );
    y0.push_back( stod( vals[8]) );

    EUDAQ_DEBUG("Acq mode (2=SEGM): " + to_string(vals[18]));
    if( vals.size() == 25 ) {// this is segmented mode, possibly more than one waveform in block
      EUDAQ_DEBUG( "Segments: " + to_string(vals[24]));
      sgmnt_count = stoi( vals[24] );
    }

    // Check our own seventh-graders math: total data size minus four times the data of a channel minus four block header words = 0,
    // Only if all block headers are the same size, which they do not seem to be for the 6 GHz scope - skip it for now
    if (!m_hitbus && (rawdata.size() - 4 * block_words - 4)) {
      for (int ii = 0; ii < 5; ++ii) {
      EUDAQ_DEBUG(to_string(ii)+": " +to_string(uint64_t(rawdata[block_position+ii])));
      }
      if(nch==3)
      EUDAQ_THROW("Go back to school 7th grade - math doesn't check out! :/ " + to_string(rawdata.size() - 4 * block_words-4) + " is not 0!");
    }


    int points_per_words = np.at(nch)/(chann_words/sgmnt_count);
    if(chann_words % sgmnt_count != 0) {
      EUDAQ_THROW("Segment count and channel words don't match: " + to_string(chann_words) + "/" + to_string(sgmnt_count));
    }
    if( np.at(nch) % (chann_words/sgmnt_count) ){ // check
      EUDAQ_THROW("incomplete waveform in block " + to_string(ev->GetEventN())
                 + ", channel " + to_string(nch) );
    }
    EUDAQ_DEBUG("WF points per data word: " + to_string(points_per_words));

    // Check our own eighth-graders math: the number of words in the channel times 4 points per word minus the number of points times number of segments is 0
    if(4 * chann_words - np.at(nch) * sgmnt_count) {
      EUDAQ_WARN("Go back to school 8th grade - math doesn't check out! :/ " + to_string(chann_words - np.at(nch) * sgmnt_count) + " is not 0!");
    }

    // set run start time to 0 ms, using  timestamps from preamble which are 10 ms precision
    timestamp = 0;

    // need once per channel
    std::vector<double> ped;
    std::vector<double> amp;
    std::vector<std::vector<double>> waves;


    for( int s=0; s<sgmnt_count; s++){ // loop semgents

      // container for one waveform, only amplitudes stored here
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
      words.resize(points_per_words); // rawdata contains 8 bit words, scope sends 2 8bit words
      int16_t wfi = 0;
      // Read from segment start until the next segment begins:
      for(int i=block_position + 3 + pream_words + (s+0)*chann_words/sgmnt_count;
          i<block_position + 3 + pream_words + (s+1)*chann_words/sgmnt_count;
          i++){

        // copy channel data from entire segment data block
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
    block_position += block_words+1;


  } // for channel


  // close rootfile
  if( m_generateRoot ){
    histoFile->Close();
    EUDAQ_DEBUG("Histograms written to " + to_string(histoFile->GetName()));
  }


  // declare map between scope channel number and 2D pixel index
  // TODO make this configurable
  std::map<int,std::vector<int>> chanToPix;
  chanToPix[0] = {0,0};
  chanToPix[1] = {1,0};
  chanToPix[2] = {2,0};
  chanToPix[3] = {3,0};

  // process waveform data
  if(time.size() > 0 ){      // only if there are waveform data

    // re-iterate waveforms
    // wavess.at(channel).at(segment).at(point)
    // ped   .at(channel).at(segment)
    for( int s = 0; s<peds.at(0).size(); s++ ){
      for( int c = 0; c<peds.size(); c++ ){

        // calculate index range for pedestal
        int i_pedStart = (m_pedStartTime - time.at(0))/dx.at(c);
        int i_pedEnd = (m_pedEndTime - time.at(0))/dx.at(c);
        int n_ped = (m_pedEndTime-m_pedStartTime)/dx.at(c);
        // check index range for pedestal
        if(i_pedStart < 0 || i_pedEnd >= wavess.at(c).at(s).size()){
          EUDAQ_WARN("Parameter pedStartTime [ns] " + to_string(m_pedStartTime) + " or pedEndTime " + to_string(m_pedEndTime));
          EUDAQ_WARN("  Yields invalid index " + to_string(i_pedStart) + " or " + to_string(i_pedEnd));
          EUDAQ_WARN("  Setting pedestal to 0!");
          peds.at(c).at(s) = 0;
          continue;
        }
        // calculate pedestal
        for( int p = i_pedStart; p<i_pedEnd; p++ ){
          peds.at(c).at(s) += wavess.at(c).at(s).at(p) / n_ped;
        }

        // calculate index range for amplitude
        int i_ampStart = (m_ampStartTime - time.at(0))/dx.at(c);
        int i_ampEnd = (m_ampEndTime - time.at(0))/dx.at(c);
        // check index range for amplitude
        if(i_ampStart < 0 || i_ampEnd >= wavess.at(c).at(s).size()){
          EUDAQ_WARN("Parameter ampStartTime [ns] " + to_string(m_ampStartTime) + " or ampEndTime " + to_string(m_ampEndTime));
          EUDAQ_WARN("  Yields invalid index " + to_string(i_ampStart) + " or " + to_string(i_ampEnd));
          EUDAQ_WARN("  Setting amplitude to 0!");
          amps.at(c).at(s) = 0;
          continue;
        }
        // calculate maximum amplitude in range
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


      // create sub-event for segments
      auto sub_event = eudaq::StandardEvent::MakeShared();

      // Create a StandardPlane representing one sensor plane
      eudaq::StandardPlane plane(0, "Caribou", "DSO9254A");

      // We need to get the trigger ID, which is encoded in the triggerID
      // channel 2 where the trigger and trigger ID are superimposed.
      // Need to calibrate the trigger ID - trigger delay. First rising edge on
      // channel is trigger, bit length is 25ns
      // Only the lower 16 bits are transmitted -> store the previous ID and add
      // bit 17++
      uint64_t trigger = 0;
      auto time_wf = wavess.at(2).at(s);
      bool start = false;
      uint8_t bit = 0;
      int steps_25ns = 25 / dx.at(2); // bit length of trigger ID
      int offset_30ns = 30 / dx.at(2); // the offset in datapoints between
                                       // trigger and trigger ID start
      auto it = time_wf.begin();
      // find the rising edge
      while (*it < 0.25) {
        it++;
      }
      // jump to center of first bit of trigger ID
      it += offset_30ns;
      // sample 16 numbers:
      for (int bit = 0; bit < 18; bit++) {
        if (*it > 0.25) {
          trigger += (0x1 << bit);
        }
        it += steps_25ns;
      }
      // add the upper bits of the trigger
      trigger += m_trigger & 0xFFFFFFFFFFFF0000;
      // check for bitflip of trigger ID MSB+1
      if (trigger < m_trigger) {
        std::cout << d1->GetEventN() << ": " << s << "\t"
                  << std::bitset<24>(trigger) << "\t" << m_trigger << std::endl;
        trigger += 0xFFFF;
      }
      // update trigger number
      m_trigger = trigger;
      // fill plane
      plane.SetSizeZS(4, 1, 0);
      for (int c = 0; c < peds.size(); c++) {
        // igitt
        if (m_hitbus/* && (c == 1 || c == 3)*/) { // need to make this in a more clever way...
          plane.PushPixel(chanToPix[c].at(0), chanToPix[c].at(1), 1, timestamp);
          plane.SetWaveform(c, wavess.at(c).at(s), x0.at(c), dx.at(c));
        }
        else if( amps.at(c).at(s)*m_chargeScale > m_chargeCut ){
          plane.PushPixel( chanToPix[c].at(0),
                           chanToPix[c].at(1),
                           amps.at(c).at(s)*m_chargeScale, timestamp );
        }
      }

     EUDAQ_INFO("Block number " + to_string(ev->GetEventN()) + " " +
                  " block size " + to_string(peds.at(0).size()) +
                  " segments, segment number " + to_string(s) +
                  " trigger number " + to_string(m_trigger));

      sub_event->AddPlane(plane);
      sub_event->SetDetectorType("DSO9254A");
      sub_event->SetTimeBegin(0); // forcing corry to fall back on trigger IDs
      sub_event->SetTimeEnd(0);
      sub_event->SetTriggerN(m_trigger);

      // add sub event to StandardEventSP for output to corry
      d2->AddSubEvent( sub_event );

    } // segments

    EUDAQ_DEBUG( "Number of sub events " + to_string(d2->GetNumSubEvent()) );

    d2->SetDetectorType("DSO9254A");
  }
  else{
    EUDAQ_WARN("No scope data in block " + to_string(ev->GetEventN()));
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

  // now convert to unix timestamp, to milli seconds and add sub-second information from scope
  std::time_t tunix;
  tunix = std::mktime( &build_time );
  uint64_t result = static_cast<uint64_t>( tunix ) * 1e3;               // [   s->ms]
  uint64_t mill10 = static_cast<uint64_t>( stoi( parts.at(6) ) ) * 1e1; // [10ms->ms]
  result += mill10;

  return result;
} // timeConverter
