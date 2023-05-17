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


bool    DSO9254AEvent2StdEventConverter::m_configured(0);

int64_t DSO9254AEvent2StdEventConverter::m_runStartTime(-1);
double DSO9254AEvent2StdEventConverter::m_pedStartTime(0);
double DSO9254AEvent2StdEventConverter::m_pedEndTime(0);
double DSO9254AEvent2StdEventConverter::m_ampStartTime(0);
double DSO9254AEvent2StdEventConverter::m_ampEndTime(0);
double DSO9254AEvent2StdEventConverter::m_chargeScale(0);
double DSO9254AEvent2StdEventConverter::m_chargeCut(0);

bool DSO9254AEvent2StdEventConverter::m_generateRoot(0);

bool DSO9254AEvent2StdEventConverter::m_printTimeStamps(0);
std::string DSO9254AEvent2StdEventConverter::m_timeStampFileName("");

std::string DSO9254AEvent2StdEventConverter::m_fileNameEventTimesExt("");
std::string DSO9254AEvent2StdEventConverter::m_fileNameEventTimesInt("");
std::set<EventTime> DSO9254AEvent2StdEventConverter::m_eventTimesExt;
std::set<EventTime> DSO9254AEvent2StdEventConverter::m_eventTimesInt;
int DSO9254AEvent2StdEventConverter::m_nMissedEvents(0);
int DSO9254AEvent2StdEventConverter::m_nMissedBlocks(0);

bool DSO9254AEvent2StdEventConverter::Converting(eudaq::EventSPC d1, eudaq::StandardEventSP d2, eudaq::ConfigurationSPC conf) const{


  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);
  // No event
  if(!ev) {
    return false;
  }

  // load parameters from config file
  std::ofstream outfileTimestamps;
  if( !m_configured ){

    // read from config file
    m_pedStartTime = conf->Get("pedStartTime", 0); // integration windows in [ns]
    m_pedEndTime   = conf->Get("pedEndTime"  , 0);
    m_ampStartTime = conf->Get("ampStartTime", 0);
    m_ampEndTime   = conf->Get("ampEndTime"  , 0);
    m_chargeScale  = conf->Get("chargeScale" , 0);
    m_chargeCut    = conf->Get("chargeCut"   , 0);
    m_generateRoot = conf->Get("generateRoot", 0);
    m_timeStampFileName = conf->Get("timestamp_file_name", "empty");
    m_fileNameEventTimesExt = conf->Get("fileNameEventTimesExt", "empty");
    m_fileNameEventTimesInt = conf->Get("fileNameEventTimesInt", "empty");

    EUDAQ_DEBUG( "Loaded parameters from configuration file." );
    EUDAQ_DEBUG( "  pedStartTime = " + to_string( m_pedStartTime ) + " ns" );
    EUDAQ_DEBUG( "  pedEndTime   = " + to_string( m_pedEndTime ) + " ns" );
    EUDAQ_DEBUG( "  ampStartTime = " + to_string( m_ampStartTime ) + " ns" );
    EUDAQ_DEBUG( "  ampEndTime   = " + to_string( m_ampEndTime ) + " ns" );
    EUDAQ_DEBUG( "  chargeScale  = " + to_string( m_chargeScale ) + " a.u." );
    EUDAQ_DEBUG( "  chargeCut    = " + to_string( m_chargeCut ) + " a.u." );
    EUDAQ_DEBUG( "  generateRoot = " + to_string( m_generateRoot ) );
    EUDAQ_DEBUG( "  timestamp_file_name = " + m_timeStampFileName);
    EUDAQ_DEBUG( "  fileNameEventTimesExt = " + m_fileNameEventTimesExt);
    EUDAQ_DEBUG( "  fileNameEventTimesInt = " + m_fileNameEventTimesInt);

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

    // set flag and prepare output file if file name is given
    if(m_timeStampFileName.compare("empty")){
      m_printTimeStamps = true;
      // need to strip quotation marks?
      if(!m_timeStampFileName.substr(0,1).compare("\"")){
        m_timeStampFileName = m_timeStampFileName.substr( 1, m_timeStampFileName.length()-2 );
      }
      outfileTimestamps.open(m_timeStampFileName.c_str(), std::ios_base::trunc); // recreate
      if(outfileTimestamps.is_open()){
        EUDAQ_DEBUG( "writing time stamps to " + m_timeStampFileName );
      }
      else{
        EUDAQ_ERROR( "Failed to open " + m_timeStampFileName );
      }
      outfileTimestamps.close();
    }

    // read event numbers and time stamps from filename if filename is given
    // this needs to be done twice
    // for this detector (Int)
    // for reference detector (Ext)
    if(m_fileNameEventTimesExt.compare("empty")){
      // need to strip quotation marks?
      if(!m_fileNameEventTimesExt.substr(0,1).compare("\"")){
        m_fileNameEventTimesExt =
          m_fileNameEventTimesExt.substr(1, m_fileNameEventTimesExt.length()-2);
      }
      readEventTimeFile(m_fileNameEventTimesExt, &m_eventTimesExt);
    }
    if(m_fileNameEventTimesInt.compare("empty")){
      // need to strip quotation marks?
      if(!m_fileNameEventTimesInt.substr(0,1).compare("\"")){
        m_fileNameEventTimesInt =
          m_fileNameEventTimesInt.substr(1, m_fileNameEventTimesInt.length()-2);
      }
      readEventTimeFile(m_fileNameEventTimesInt, &m_eventTimesInt);
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

  // number of missed events for re-sync
  int thisBlockMissed = 0;


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


  // loop 4 channels
  for( int nch = 0; nch < 4; nch++ ){

    EUDAQ_DEBUG( "Reading channel " + to_string(nch));

    // Obtaining Data Stucture
    block_words = rawdata[block_position + 0];
    pream_words = rawdata[block_position + 1];
    chann_words = rawdata[block_position + 2 + pream_words];
    EUDAQ_DEBUG( "  " + to_string(block_position) + " current position in block");
    EUDAQ_DEBUG( "  " + to_string(block_words) + " words per segment");
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

    // Check our own seventh-graders math: total data size minus four times the data of a channel minus four block header words = 0
    if(rawdata.size() - 4 * block_words - 4) {
      EUDAQ_WARN("Go back to school 7th grade - math doesn't check out! :/ " + to_string(rawdata.size() - 4 * block_words) + " is not 0!");
    }

    int points_per_words = np.at(nch)/(chann_words/sgmnt_count);
    if(chann_words % sgmnt_count != 0) {
      EUDAQ_WARN("Segment count and channel words don't match: " + to_string(chann_words) + "/" + to_string(sgmnt_count));
    }
    if( np.at(nch) % (chann_words/sgmnt_count) ){ // check
      EUDAQ_WARN("incomplete waveform in block " + to_string(ev->GetEventN())
                 + ", channel " + to_string(nch) );
    }
    EUDAQ_DEBUG("WF points per data word: " + to_string(points_per_words));

    // Check our own eighth-graders math: the number of words in the channel times 4 points per word minus the number of points times number of segments is 0
    if(4 * chann_words - np.at(nch) * sgmnt_count) {
      EUDAQ_WARN("Go back to school 8th grade - math doesn't check out! :/ " + to_string(chann_words - np.at(nch) * sgmnt_count) + " is not 0!");
    }

    // set run start time to 0 ms, using  timestamps from preamble which are 10 ms precision
    if( m_runStartTime < 0 ){
      m_runStartTime = DSO9254AEvent2StdEventConverter::timeConverter( vals[15], vals[16] );
    }
    timestamp = ( DSO9254AEvent2StdEventConverter::timeConverter( vals[15], vals[16] )
                  - m_runStartTime ) * 1e9; // ms to ps


    // print time stamps to file
    if(m_printTimeStamps && nch == 0){ // once per event
      // need to re-open, each time we want to fill
      outfileTimestamps.open(m_timeStampFileName.c_str(), std::ios_base::app);
      outfileTimestamps << ev->GetEventN() << " " << timestamp << std::endl;
      outfileTimestamps.close();
    }

    // if there are two time stamp files we can try to resync
    if(m_eventTimesInt.size() > 0 && m_eventTimesExt.size() > 0 &&
       nch == 0){ // time stamps are identical for all channels

      EUDAQ_DEBUG("Compare time stamps event " + to_string(ev->GetEventN()) +
                  " timestamp " + to_string(timestamp));
      EventTime findme(ev->GetEventN(),timestamp);

      // get scope time stamps for this and the next block
      auto thisBlockTimeInt = m_eventTimesInt.find(findme);
      auto nextBlockTimeInt = thisBlockTimeInt;
      nextBlockTimeInt++;

      // we have no scope timestamp to mark the end of the last block
      // we can not know if it is complete or not -> discard
      if(nextBlockTimeInt==m_eventTimesInt.end()){
        thisBlockMissed = 1;
      }
      else{
        // get next larger time stamps from reference detector
        auto thisBlockStart = m_eventTimesExt.upper_bound(*thisBlockTimeInt);
        auto thisBlockEnd   = m_eventTimesExt.upper_bound(*nextBlockTimeInt);

        // using getBlockEnd, trying to find the gap in cases where the time stamps are off
        // see commit e5a3193e for version without
        if(thisBlockTimeInt->time > 0){ // do not go ealier than run start
          thisBlockStart = DSO9254AEvent2StdEventConverter::getBlockEnd(thisBlockStart,
                                                                        thisBlockTimeInt);
          thisBlockStart++; // blockend++ ~ blockstart
        }
        else{ // first
          thisBlockStart = m_eventTimesExt.begin();
        }
        thisBlockEnd = DSO9254AEvent2StdEventConverter::getBlockEnd(thisBlockEnd,
                                                                    nextBlockTimeInt);

        // calculate number of missed events
        uint64_t deltaIev = thisBlockEnd->iev - thisBlockStart->iev;
        thisBlockMissed = sgmnt_count - (deltaIev+1);
        m_nMissedEvents += thisBlockMissed;

        EUDAQ_DEBUG("  Reference event numbers range" + to_string(thisBlockStart->iev) +
                    " " + to_string(thisBlockEnd->iev) +
                    " this block misses " + to_string(thisBlockMissed));

      }

    } // have time stamp files


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
  chanToPix[0] = {1,0};
  chanToPix[1] = {0,0};
  chanToPix[2] = {1,1};
  chanToPix[3] = {0,1};

  // process waveform data
  if(time.size() > 0 &&      // only if there are waveform data
     thisBlockMissed == 0 ){ // and if we are not missing events in external detector

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

      // fill plane
      plane.SetSizeZS(2, 2, 0);
      for( int c = 0; c<peds.size(); c++ ){
        if( amps.at(c).at(s)*m_chargeScale > m_chargeCut ){
          plane.PushPixel( chanToPix[c].at(0),
                           chanToPix[c].at(1),
                           amps.at(c).at(s)*m_chargeScale, timestamp );
        }
      }

      // derive trigger number from block number
      // take missed events into account
      int triggerN = (ev->GetEventN()-1) * peds.at(0).size() + s - m_nMissedEvents;
      // why is this necessary?
      if(ev->GetEventN()==1 && to_string(peds.at(0).size()) > 1){
        triggerN++;
      }

      EUDAQ_DEBUG("Block number " + to_string(ev->GetEventN()) + " " +
                  " block size " + to_string(peds.at(0).size()) +
                  " segments, segment number " + to_string(s) +
                  " subtracting missed " + to_string(m_nMissedEvents) +
                  " trigger number " + to_string(triggerN));

      sub_event->AddPlane(plane);
      sub_event->SetDetectorType("DSO9254A");
      sub_event->SetTimeBegin(0); // forcing corry to fall back on trigger IDs
      sub_event->SetTimeEnd(0);
      sub_event->SetTriggerN(triggerN);

      // add sub event to StandardEventSP for output to corry
      d2->AddSubEvent( sub_event );

    } // segments

    EUDAQ_DEBUG( "Number of sub events " + to_string(d2->GetNumSubEvent()) );

    d2->SetDetectorType("DSO9254A");
  }
  else if(thisBlockMissed != 0){
    EUDAQ_DEBUG("Missed " + to_string(thisBlockMissed) +
                " events in block " + to_string(ev->GetEventN()));
    m_nMissedBlocks++;
    return false;
    }
  else{
    EUDAQ_WARN("No scope data in block " + to_string(ev->GetEventN()));
    return false;
  }

  // print resync yield
  if(!m_eventTimesExt.empty() && !m_eventTimesInt.empty()){ // are we even trying
    EUDAQ_INFO("Resync yield: " + to_string(1-(double)m_nMissedBlocks/(double)ev->GetEventN()));
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

void DSO9254AEvent2StdEventConverter::readEventTimeFile(std::string filename,
                                                        std::set<EventTime>* eventtime){

  // open
  std::ifstream infileTimestamps;
  infileTimestamps.open(filename.c_str(), std::ios_base::in);
  if(infileTimestamps.is_open()){
    EUDAQ_DEBUG("Reading time stamps from " + filename);
  }
  else{
    EUDAQ_ERROR("Failed to open " + filename);
    return;
  }

  // parse to EventTime set
  while(!infileTimestamps.eof()){

    uint64_t iev, tev;

    std::string line_string;
    std::getline(infileTimestamps, line_string);
    std::stringstream line_stream(line_string);
    line_stream >> iev;
    line_stream >> tev;
    EventTime et(iev, tev);

    // check if there is already a time stamp with the same value and event number
    if(!eventtime->empty() && eventtime->find(et)->iev==iev){
      continue;
    }

    // make entry otherwise
    eventtime->insert(et);

  } // file good

  // close
  infileTimestamps.close();

  return;
} // readEventTimeFile

// FIXME not using internal
// this might need some fine tuning. right now it is searching for the largest gap
// within +- 5 events around the external event.
// might want to try something like closest gap larger than e.g. 0.5 s
std::set<EventTime>::iterator DSO9254AEvent2StdEventConverter::getBlockEnd(
  std::set<EventTime>::iterator external, std::set<EventTime>::iterator internal){

  Long64_t largeGap = 0;
  std::set<EventTime>::iterator blockEnd;

  // check gaps in preceeding alpide time stamps
  for(int prev = 0; prev < 5; prev++){
    Long64_t gap = (external)->time - (--external)->time;
    // update gap lenght and event
    if(gap > largeGap){
      largeGap = gap;
      blockEnd = external;
    }
  }
  std::advance(external,5);// reset iterator

  // check gaps in following alpide time stamps
  for(int next = 0; next < 5; next++){
    Long64_t gap = -(external->time) + (++external)->time;
    // update gap length and event
    if(gap > largeGap){
      largeGap = gap;
      external--; // blockEnd should be the one with the lower timestamp
      blockEnd = external;
      external++;
    }
  }
  std::advance(external,-5); // reset iterator

  EUDAQ_DEBUG("  getBlockEnd: Internal event " + to_string(internal->iev) +
              " time " + to_string(internal->time));
  EUDAQ_DEBUG("  getBlockEnd: External event " + to_string(blockEnd->iev) +
              " time " + to_string(blockEnd->time));


  return blockEnd;
} // getBlockEnd
