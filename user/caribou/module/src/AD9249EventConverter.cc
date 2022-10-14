#include "CaribouEvent2StdEventConverter.hh"

#include "utils/log.hpp"

#include <algorithm>
#include <fstream>

using namespace eudaq;

namespace {
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::Register<
      AD9249Event2StdEventConverter>(
      AD9249Event2StdEventConverter::m_id_factory);
}

size_t AD9249Event2StdEventConverter::trig_(0);
bool AD9249Event2StdEventConverter::m_configured(0);
bool AD9249Event2StdEventConverter::m_useTime(0);
int64_t AD9249Event2StdEventConverter::m_runStartTime(-1);
int AD9249Event2StdEventConverter::threshold_trig(1000);
int AD9249Event2StdEventConverter::threshold_low(101);
std::string AD9249Event2StdEventConverter::m_waveform_filename("");
std::ofstream AD9249Event2StdEventConverter::m_outfile_waveforms;
//baseline evaluation
int AD9249Event2StdEventConverter::m_blStart(200);
int AD9249Event2StdEventConverter::m_blEnd(100);
// calibration functions
double AD9249Event2StdEventConverter::m_calib_range_min( 1000);
double AD9249Event2StdEventConverter::m_calib_range_max(10000);
std::vector<std::string> AD9249Event2StdEventConverter::m_calib_strings(16,"x");
std::vector<TF1*> AD9249Event2StdEventConverter::m_calib_functions(16, new TF1("name","x"));

void AD9249Event2StdEventConverter::decodeChannel(
    const size_t adc, const std::vector<uint8_t> &data, size_t size,
    size_t offset, std::vector<std::vector<uint16_t>> &waveforms,
    uint64_t &timestamp) const {

  // Timestamp index
  size_t ts_i = 0;

  for (size_t i = offset; i < offset + size; i += 2) {
    // Channel is ADC half times channels plus channel number within data block
    size_t ch = adc * 8 + ((i - offset) / 2) % 8;

    // Get waveform data
    uint16_t val =
        data.at(i) + ((static_cast<uint16_t>(data.at(i + 1)) & 0x3F) << 8);
    waveforms.at(ch).push_back(val);

    // If we have a full timestamp, skip the rest:
    if (ts_i >= 28) {
      continue;
    }

    // Treat timestamp data
    uint64_t ts = (data.at(i + 1) >> 6);

    // Channel 7 (or 15) have status bits only:
    if (ch == adc * 8 + 7) {
      // Check if this is a timestamp start - if not, reset timestamp index to
      // zero:
      if (ts_i < 8 && (ts & 0x1) == 0) {
        ts_i = 0;
      }
    } else {
      timestamp += (ts << 2 * ts_i);
      ts_i++;
    }
  }

  // Convert timestamp to picoseconds from the 65MHz clock (~15ns cycle):
  //  timestamp *= static_cast<uint64_t>(1. / 65. * 1e6);
  timestamp = static_cast<uint64_t>(timestamp * 1e6 / 65.);
}

bool AD9249Event2StdEventConverter::Converting(
    eudaq::EventSPC d1, eudaq::StandardEventSP d2,
    eudaq::ConfigurationSPC conf) const {

  if( !m_configured ){
    threshold_trig = conf->Get("threshold_trig", 1000);
    threshold_low = conf->Get("threshold_low", 101);
    m_blStart = conf->Get("blStart", 200);
    m_blEnd = conf->Get("blEnd", 100);
    m_calib_range_min = conf->Get("calib_range_min", m_calib_range_min);
    m_calib_range_max = conf->Get("calib_range_max", m_calib_range_max);
    m_useTime = conf->Get("use_time_stamp", false);
    m_waveform_filename = conf->Get("waveform_filename", "");

    // read calibration functions
    delete m_calib_functions.at(0); // since there is only one 'name' this destroys them all
    for(unsigned int i = 0; i < m_calib_strings.size(); i++){
      unsigned int col = i/4;
      unsigned int row = i%4;
      std::string name = "calibration_px" + to_string(i/4) + to_string(i%4);
      m_calib_strings.at(i) = conf->Get(name, m_calib_strings.at(i));
      m_calib_functions.at(i) = new TF1(name.c_str(), m_calib_strings.at(i).c_str(), m_calib_range_min, m_calib_range_max);
    }

    EUDAQ_DEBUG( "Using configuration:" );
    EUDAQ_DEBUG( " threshold_low  = " + to_string( threshold_low ));
    EUDAQ_DEBUG( " threshold_trig  = " + to_string( threshold_trig ));
    EUDAQ_DEBUG( " use_time_stamp  = " + to_string( m_useTime ));
    EUDAQ_DEBUG( "Calibration functions: ");
    for(unsigned int i = 0; i < m_calib_strings.size(); i++){
      EUDAQ_DEBUG( to_string( m_calib_functions.at(i)->GetName() ) + " " +
                   to_string( m_calib_functions.at(i)->GetExpFormula() ));
    }
    EUDAQ_DEBUG( " calib_range_min  = " + to_string( m_calib_range_min ));
    EUDAQ_DEBUG( " calib_range_max  = " + to_string( m_calib_range_max ));

    m_configured = true;
  }
  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);
  EUDAQ_DEBUG("Decoding AD event " + to_string(ev->GetEventN()) + " trig " +
              to_string(trig_));

  const size_t header_offset = 8;
  auto datablock0 = ev->GetBlock(0);

  // std::ofstream out;
  // out.open("/tmp/out.dat", std::ofstream::out | std::ofstream::binary |
  // std::ofstream::app); out.write(reinterpret_cast<char*>(datablock0.data()),
  // datablock0.size()); out.close();

  // Get configured burst length from header:
  uint32_t burst_length =
      (static_cast<uint32_t>(datablock0.at(3)) << 8) | datablock0.at(2);

  // Check total available data against expected event size:
  const size_t evt_length = burst_length * 128 * 2 * 16 + 16;
  if (datablock0.size() < evt_length) {
    // FIXME throw something at someone?
    // std::cout << "Event length " << datablock0.size() << " not enough for
    // full event, requires " << evt_length << std::endl;
    return false;
  }

  EUDAQ_DEBUG("Burst: " + to_string(burst_length));

  // Read waveforms
  std::vector<std::vector<uint16_t>> waveforms;
  waveforms.resize(16);

  uint32_t size_ADC0 = (static_cast<uint32_t>(datablock0.at(7)) << 24) +
                       (static_cast<uint32_t>(datablock0.at(6)) << 16) +
                       (static_cast<uint32_t>(datablock0.at(5)) << 8) +
                       (datablock0.at(4) << 0);
  uint32_t size_ADC1 =
      (static_cast<uint32_t>(datablock0.at(header_offset + size_ADC0 + 7))
       << 24) +
      (static_cast<uint32_t>(datablock0.at(header_offset + size_ADC0 + 6))
       << 16) +
      (static_cast<uint32_t>(datablock0.at(header_offset + size_ADC0 + 5))
       << 8) +
      (datablock0.at(header_offset + size_ADC0 + 4) << 0);

  // Decode channels:
  uint64_t timestamp0 = 0;
  uint64_t timestamp1 = 0;
  decodeChannel(0, datablock0, size_ADC0, header_offset, waveforms, timestamp0);
  decodeChannel(1, datablock0, size_ADC1, 2 * header_offset + size_ADC0,
                waveforms, timestamp1);

  // store time of the run start
  if(trig_ <= 1){
    m_runStartTime = timestamp0; // just use one of them for now
  }

  // Prepare output plane:
  eudaq::StandardPlane plane(0, "Caribou", "AD9249");
  plane.SetSizeZS(4, 4, 0);

  // Channels are sorted like ADC0: A1 C1 E1 ...
  //                          ADC1: B1 D1 F1 ...
  std::vector<std::pair<int, int>> mapping = {
      {1, 2}, {0, 2}, {1, 1}, {1, 0}, {0, 3}, {0, 1}, {0, 0}, {2, 0},
      {2, 1}, {3, 0}, {3, 2}, {3, 3}, {3, 1}, {2, 2}, {2, 3}, {1, 3}};

  // AD9249 channels to pixel matrix map:
  // A2, H2, F2, H1
  // C1, A1, D2, F1
  // C2, E1, B1, B2
  // E2, G1, G2, D1


  // print waveforms to file, if a filename is given
  // this returns false! If you want to change that that remove `trig_++`!!!
  if(!m_waveform_filename.empty()){

    // Open
    m_outfile_waveforms.open(m_waveform_filename, std::ios_base::app); // append

    // Print to file
    for(size_t ch = 0; ch < waveforms.size(); ch++) {
      m_outfile_waveforms << trig_ << " " << ch << " " << mapping.at(ch).first << " " << mapping.at(ch).second << " : ";
      auto const& waveform = waveforms.at(ch);
      for(auto const& sample : waveform) {
        m_outfile_waveforms << sample << " ";
      }
      m_outfile_waveforms << std::endl;
    }

    m_outfile_waveforms.close();
    trig_++;
    return false;
  }

  EUDAQ_DEBUG("_______________ Event " + to_string(ev->GetEventN()) + " trig " +
              to_string(trig_) + " __________");

  std::map<std::pair<int, int>, std::pair<int, bool>> amplitudes;
  for (size_t ch = 0; ch < waveforms.size(); ch++) {

    auto max = *std::max_element(waveforms[ch].begin(), waveforms[ch].end());
    auto max_posizion = std::distance(waveforms[ch].begin(),std::max_element(waveforms[ch].begin(), waveforms[ch].end()));

    if((max_posizion - m_blStart) < 0){
      //skip if the max is too early
      continue;
    }

    double baseline = 0.;

    for(int i=max_posizion-m_blStart; i<max_posizion-m_blEnd; i++){
      baseline += waveforms[ch][i];
    }
    baseline /= m_blStart-m_blEnd;

    bool hit = false;
    amplitudes[mapping.at(ch)] = std::pair<int, bool>(max - baseline, false);
    //    if(max - min > threshold_trig) {
    //  auto p = mapping.at(ch);
    //  plane.PushPixel(p.first,p.second, max - min, timestamp0);
    //  hit = true;
    // }
    EUDAQ_DEBUG("------------------------------" + to_string(ch) + ": " +
                to_string(waveforms[ch].size()) + "-----" + '\t' +
                to_string(max) + (hit ? " HIT" : " --"));
  }

  for (auto &p : amplitudes) {
    if (p.second.first > threshold_trig) {
      // add the seed pixel if not added
      if (!p.second.second) {
        plane.PushPixel(p.first.first, p.first.second, p.second.first,
                        timestamp0);
        p.second.second = true;
      }
      // loop over all surrounding once
      for (auto &pp : amplitudes) {
        if ((std::abs(pp.first.first - p.first.first) <= 1) &&
            (std::abs(pp.first.second - p.first.second) <= 1)) {
          if (pp.second.first > threshold_low) {
            if (!pp.second.second) {
              plane.PushPixel(pp.first.first, pp.first.second, pp.second.first,
                              timestamp0);
              pp.second.second = true;
            }
          }
        }
      }
    }
  }

  // Add the plane to the StandardEvent
  d2->AddPlane(plane);

  // use timestamps
  if( m_useTime ){
    d2->SetTimeBegin(timestamp0 - m_runStartTime);
    d2->SetTimeEnd(timestamp0 - m_runStartTime);
  }
  // forcing corry to fall back on trigger IDs
  else{
    d2->SetTimeBegin(0);
    d2->SetTimeEnd(0);
  }
  d2->SetTriggerN(trig_);
  trig_++;

  // Identify the detetor type
  d2->SetDetectorType("AD9249");
  // Indicate that data was successfully converted
  return true;
}

/*
 *  Erics python reference
 *
channels = 8

while True:
    h = file.read(4)
    header = struct.unpack('HH', h)
    bursts = header[1]
    points = 128 * bursts
    print("Channel", header[0], "Burst", header[1])

    s = file.read(4)
    size = struct.unpack('I', s)[0]
    print("Block size", size)

    while size > 0:
        data = file.read(points*2*channels)
        print("Reading", points*2*channels, "bytes")
        size -= points*2*channels

        val = [(i[0] & 0x3FFF) for i in struct.iter_unpack('<H', data)]
        val2 = np.reshape(val, (channels, -1), order='F')

        aux = [(i[0] >> 14) for i in struct.iter_unpack('<H', data)]
        aux2 = np.reshape(aux, (-1, channels))

        foo = []

        for i in aux2:
            if i[-1] & 2:
                print('trigger')

            if i[-1] & 1:
                out = 0
                for j in foo[::-1]:
                    out <<= 2
                    out |= j
                print(out/65000000.0)
                foo = []
            foo.extend(i[:-1])


        #fig, ax = plt.subplots(2,4, figsize=(16,9), sharex='col', sharey='row')
        fig, ax = plt.subplots(2,4, figsize=(16,9), sharex='all', sharey='all')
        for x in range(0, 4):
            for y in range(0, 2):
                i = y*4+x
                channel = i + 8*header[0]
                ax[y][x].plot(np.arange(0, len(val2[i]))*(1.0/65), val2[i])
                ax[y][x].set_title('ch {}'.format(channel))

        plt.show()

 */
