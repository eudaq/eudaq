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

int DSO9254AEvent2StdEventConverter::m_channels(0);
bool DSO9254AEvent2StdEventConverter::m_configured(0);
bool DSO9254AEvent2StdEventConverter::m_hitbus(0);
uint64_t DSO9254AEvent2StdEventConverter::m_trigger(0);
int64_t DSO9254AEvent2StdEventConverter::m_runStartTime(-1);
bool DSO9254AEvent2StdEventConverter::m_generateRoot(0);
TFile *DSO9254AEvent2StdEventConverter::m_rootFile(nullptr);
std::map<int, std::pair<int, int>> DSO9254AEvent2StdEventConverter::m_chanToPix{};

bool DSO9254AEvent2StdEventConverter::Converting(eudaq::EventSPC d1, eudaq::StandardEventSP d2, eudaq::ConfigurationSPC conf) const{


  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);
  // No event
  if(!ev) {
    return false;
  }

  // load parameters from config file
  std::ofstream outfileTimestamps;

  if (!m_configured) {

   // open the root file once
    m_generateRoot = conf->Get("generateRoot", 0);
    m_channels = conf->Get("nChannels", 4);
    if (m_generateRoot) {
      m_rootFile =
          new TFile(Form("waveforms_run%i.root", ev->GetRunN()), "RECREATE");
      if (!m_rootFile) {
        EUDAQ_THROW(to_string(m_rootFile->GetName()) + " can not be opened");
        return false;
      }
    }
    m_hitbus = conf->Get("hitbus", 0);

    if (m_hitbus) {
      EUDAQ_WARN("Reading hitbus like data assuming <Ariannas order that I do "
                 "not know now :)>");
    }

    EUDAQ_DEBUG( "  generateRoot = " + to_string( m_generateRoot )) ;

    m_configured = true;

  } // configure



  // internal containers
//  std::vector<std::vector<std::vector<double>>> wavess;
//  std::vector<std::vector<double>> peds;
//  std::vector<std::vector<double>> amps;
//  std::vector<double> time;
//  uint64_t timestamp;


  // Bad event
  if(ev->NumBlocks() != 1) {
    EUDAQ_WARN("Ignoring bad packet " + std::to_string(ev->GetEventNumber()));
    return false;
  }

  // all four scope channels in one data block
  auto datablock = ev->GetBlock(0);
  EUDAQ_DEBUG("DSO9254A frame with " + to_string(datablock.size()) + " entries");

   // get the waveforms for all active channels and segments
  auto waves = read_data(datablock,ev->GetEventN());


  // declare map between scope channel number and 2D pixel index - should be configurable (will be lovely in eudaq configs)
  m_chanToPix[0] = {0,0};
  m_chanToPix[1] = {1,0};
  m_chanToPix[2] = {2,0};
  m_chanToPix[3] = {3,0};

    // loop over all segments and reshuffle them into subevents
    for( int s = 0; s<waves.front().size(); s++ ){

      // create sub-event for each segment
      auto sub_event = eudaq::StandardEvent::MakeShared();

      // Create a StandardPlane representing one sensor plane
      eudaq::StandardPlane plane(0, "Caribou", "DSO9254A");

      // if we take the trigger ID as input on one channel, fetch it. Ortherwise
      // eventnumber*segments+currentsegments
      m_trigger  = m_hitbus ? triggerID(waves.at(2).at(s)) : ev->GetEventN() * 100 + s;

      // fill plane - how to properly size this...
      plane.SetSizeZS(4, 1, 0);
      for (int c = 0; c < waves.size(); c++) {
      auto wf = waves.at(c).at(s);
      plane.PushPixel(m_chanToPix[c].first, m_chanToPix[c].second, 1,uint64_t(0x0));
      plane.SetWaveform(c, wf.data, waves.at(c).at(s).x0, waves.at(c).at(s).dx);
      if (m_generateRoot) {
        std::shared_ptr<TH1D>hist = std::make_shared<TH1D>(Form("waveform_run%i_ev%i_ch%i_s%i",
                                   ev->GetRunN(), ev->GetEventN(), c, s),
                              Form("waveform_run%i_ev%i_ch%i_s%i",
                                   ev->GetRunN(), ev->GetEventN(), c, s),
                              wf.points, wf.x0 - wf.dx / 2.,
                              wf.x0 - wf.dx / 2. + wf.dx * wf.points);
        hist->GetXaxis()->SetTitle("time [ns]");
        hist->GetYaxis()->SetTitle("signal [V]");
        for (int i = 0; i < wf.points; i++) {
          hist->SetBinContent(hist->FindBin(i * wf.dx + wf.x0), wf.data.at(i));
        }
        hist->Write();
      }
      }
     EUDAQ_DEBUG("EUDAQ Event number " + to_string(ev->GetEventN()) + " " +
                 " number of segments  " + to_string(waves.front().size()) +
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
}

std::vector<std::vector<waveform>>
DSO9254AEvent2StdEventConverter::read_data(std::vector<std::uint8_t> &datablock,
                                           int evt) {
  // Calulate positions and length of data blocks:
  // FIXME FIXME FIXME by Simon: this is prone to break since you are selecting
  // bits from a 64bit
  //  word - but there is no guarantee in the endianness here and you
  //  might end up having the wrong one.
  // Finn: I am not sure if I actually solved this issue! Lets discuss.
  std::vector<std::vector<waveform>> waves;
  waves.resize(m_channels);
  caribou::pearyRawData rawdata;
  rawdata.resize(sizeof(datablock[0]) * datablock.size() / sizeof(uintptr_t));
  std::memcpy(&rawdata[0], &datablock[0],
              sizeof(datablock[0]) * datablock.size());


  // needed per event
  uint64_t block_words;
  uint64_t pream_words;
  uint64_t chann_words;
  uint64_t block_position = 0;
  uint64_t sgmnt_count = 1;

  for (int nch = 0; nch < (m_channels); ++nch) {


    EUDAQ_DEBUG("Reading channel " + to_string(nch) + " of " +
                to_string(m_channels));

    // Obtaining Data Stucture
    block_words = rawdata[block_position + 0];
    pream_words = rawdata[block_position + 1];
    chann_words = rawdata[block_position + 2 + pream_words];
    EUDAQ_DEBUG("  " + to_string(datablock.size()) + " 8 bit block size");
    EUDAQ_DEBUG("  " + to_string(block_position) +
                " current position in block");
    EUDAQ_DEBUG("  " + to_string(block_words) + " words per data block");
    EUDAQ_DEBUG("  " + to_string(pream_words) + " words per preamble");
    EUDAQ_DEBUG("  " + to_string(chann_words) + " words per channel data");
    EUDAQ_DEBUG("  " + to_string(rawdata.size()) + " size of rawdata");

    // Data sanity check: Total block size should be:
    // pream_words+channel_data_words+2
    if (block_words - pream_words - 2 - chann_words) {
      EUDAQ_WARN("Total data block size does not match the sum of "
                 "preamble+channel data + 2 (header)");
    }

    // read preamble:
    std::string preamble = "";
    for (int i = block_position + 2; i < block_position + 2 + pream_words;
         i++) {
      preamble += (char)rawdata[i];
    }
    EUDAQ_DEBUG("Preamble " + preamble);
    // parse preamble to vector of strings
    std::string val;
    std::vector<std::string> vals;
    std::istringstream stream(preamble);
    while (std::getline(stream, val, ',')) {
      vals.push_back(val);
    }

    // Pick needed preamble elements - this is constant for all elements of
    // channel and segment
    waveform wave;
    wave.points = stoi(vals[2]);
    wave.dx = stod(vals[4]) * 1e9;
    wave.x0 = stod(vals[5]) * 1e9;
    wave.dy = stod(vals[7]);
    wave.y0 = stod(vals[8]);

    EUDAQ_INFO("Acq mode (2=SEGM): " + to_string(vals[18]));
    if (vals.size() == 25) { // this is segmented mode, possibly more than one
                             // waveform in block
      EUDAQ_DEBUG("Segments: " + to_string(vals[24]));
      sgmnt_count = stoi(vals[24]);
    }

    int points_per_words = wave.points / (chann_words / sgmnt_count);
    if (chann_words % sgmnt_count != 0) {
      EUDAQ_THROW("Segment count and channel words don't match: " +
                  to_string(chann_words) + "/" + to_string(sgmnt_count));
    }
    if (wave.points % (chann_words / sgmnt_count)) { // check
      EUDAQ_THROW("incomplete waveform in block " + to_string(evt) +
                  ", channel " + to_string(nch));
    }
    EUDAQ_DEBUG("WF points per data word: " + to_string(points_per_words));

    // Check our own eighth-graders math: the number of words in the channel
    // times 4 points per word minus the number of points times number of
    // segments is 0
    if (4 * chann_words - wave.points * sgmnt_count) {
      EUDAQ_WARN("Go back to school 8th grade - math doesn't check out! :/ " +
                 to_string(chann_words - wave.points * sgmnt_count) +
                 " is not 0!");
    }

    for (int s = 0; s < sgmnt_count; s++) { // loop semgents
      auto current_wave = wave;
      // read channel data
      std::vector<int16_t> words;
      words.resize(points_per_words); // rawdata contains 8 bit words, scope
                                      // sends 2 8bit words
      int16_t wfi = 0;
      // Read from segment start until the next segment begins:
      for (int i = block_position + 3 + pream_words +
                   (s + 0) * chann_words / sgmnt_count;
           i < block_position + 3 + pream_words +
                   (s + 1) * chann_words / sgmnt_count;
           i++) {

        // copy channel data from entire segment data block
        memcpy(&words.front(), &rawdata[i], points_per_words * sizeof(int16_t));

        for (auto word : words) {
          // fill vectors with time bins and waveform
          // Lennart: what are the time bins  good for?
          if (nch == 0 && s == 0) { // this we need only once
            //    time.push_back( wfi * dx.at(nch) + x0.at(nch) );
          }
          current_wave.data.push_back((word * wave.dy + wave.y0));
          wfi++;

        } // words

     } // waveform
      waves.at(nch).push_back(current_wave);

    } // segments


    // update position for next iteration
    block_position += block_words + 1;


  } // for channel

  // sanity check if everything has been read
  if (!(block_position == rawdata.size())) {
    EUDAQ_THROW(
        "Have not read all data at the end of data reading for event: " +
        to_string(evt) + ": last  data word read: " +
        to_string(block_position) + " of " + to_string(rawdata.size()));
  }

  return waves;
}

uint64_t DSO9254AEvent2StdEventConverter::triggerID(waveform &wf) {
  // We need to get the trigger ID, which is encoded in the triggerID
  // channel 2 where the trigger and trigger ID are superimposed.
  // Need to calibrate the trigger ID - trigger delay. First rising edge on
  // channel is trigger, bit length is 25ns
  // Only the lower 16 bits are transmitted -> store the previous ID and add
  // bit 17++
  uint64_t trigger = 0;
  bool start = false;
  uint8_t bit = 0;
  int steps_25ns = 25 / wf.dx; // bit length of trigger ID
  int offset_30ns = 30 / wf.dx; // the offset in datapoints between
                                // trigger and trigger ID start
  auto it = wf.data.begin();
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
    trigger += 0xFFFF;
  }
  // update trigger number
  return  trigger;
}

