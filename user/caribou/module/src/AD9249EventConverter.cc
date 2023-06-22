#include "CaribouEvent2StdEventConverter.hh"

#include "utils/log.hpp"

#include <algorithm>
#include <fstream>
#include "TF1.h"

using namespace eudaq;

namespace {
auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::Register<
    AD9249Event2StdEventConverter>(AD9249Event2StdEventConverter::m_id_factory);
}

size_t AD9249Event2StdEventConverter::trig_(0);
bool AD9249Event2StdEventConverter::m_configured(0);
std::string AD9249Event2StdEventConverter::m_waveform_filename("");
std::ofstream AD9249Event2StdEventConverter::m_outfile_waveforms;
// baseline evaluation
int AD9249Event2StdEventConverter::m_blStart(150);
int AD9249Event2StdEventConverter::m_blEnd(80);
// amplitude evaluation
int AD9249Event2StdEventConverter::m_ampStart(170);
int AD9249Event2StdEventConverter::m_ampEnd(270);
// calibration functions
double AD9249Event2StdEventConverter::m_calib_range_min(0);
double AD9249Event2StdEventConverter::m_calib_range_max(16384);
std::vector<std::string>
  AD9249Event2StdEventConverter::m_calib_strings(16,"x");
std::vector<TF1>
  AD9249Event2StdEventConverter::m_calib_functions(16, TF1());

void AD9249Event2StdEventConverter::decodeChannel(
    const size_t adc, const std::vector<uint8_t> &data, size_t size,
    size_t offset, std::vector<std::vector<uint16_t> > &waveforms,
    uint64_t &timestamp) const {

  // Timestamp index
  size_t ts_i = 0;
  size_t ts_lost = 0;

  for (size_t i = offset; i < offset + size; i += 2) {
    // Channel is ADC half times channels plus channel number within data block
    size_t ch = adc * 8 + ((i - offset) / 2) % 8;

    // Get waveform data from least significant 14bit of the 16bit word
    uint16_t val =
        data.at(i) + ((static_cast<uint16_t>(data.at(i + 1)) & 0x3F) << 8);
    waveforms.at(ch).push_back(val);

    // If we have a full timestamp (56 bit from 2bitx28), skip parsing the
    // timestamps for the rest of the burst, we only care about the first
    // fully-contained:
    if (ts_i >= 28) {
      continue;
    }

    // Treat timestamp data
    uint64_t ts = (data.at(i + 1) >> 6);

    // Channel 7 (or 15) have status bits only, let's check them:
    if (ch == adc * 8 + 7) {
      // Check if this channel has the marker for timestamp begin.
      // The timestamp spans multiple samplings and the sampling where it starts
      // is marked with 0x1 in the status bits. The status bits are carried in
      // Channel 7, i.e. the last one - which means only after reading and
      // storing 7x2 bits of the timestamp already, we know if they were at the
      // begin of a new timestamp. If not, we need to rewind the index and start
      // over again unti we find a timestamp start.
      if (ts_i < 8 && (ts & 0x1) == 0) {
        ts_i = 0;
        ts_lost++;
        timestamp = 0;
      }
    } else {
      timestamp += (ts << 2 * ts_i);
      ts_i++;
    }
  }

  // Convert timestamp to picoseconds from the 65MHz clock (~15ns cycle):
  timestamp = static_cast<uint64_t>((timestamp - ts_lost) * 1e6 / 65.);
}

bool
AD9249Event2StdEventConverter::Converting(eudaq::EventSPC d1,
                                          eudaq::StandardEventSP d2,
                                          eudaq::ConfigurationSPC conf) const {

  if (!m_configured) {
    m_blStart = conf->Get("blStart", m_blStart);
    m_blEnd = conf->Get("blEnd", m_blEnd);
    m_ampStart = conf->Get("ampStart", m_ampStart);
    m_ampEnd = conf->Get("ampEnd", m_ampEnd);
    m_calib_range_min = conf->Get("calib_range_min", m_calib_range_min);
    m_calib_range_max = conf->Get("calib_range_max", m_calib_range_max);
    m_waveform_filename = conf->Get("waveform_filename", "");

    // read calibration functions
    m_calib_functions.clear();
    for (unsigned int i = 0; i < m_calib_strings.size(); i++) {
      std::string name = "calibration_px" + to_string(mapping.at(i).first) +
                         to_string(mapping.at(i).second);
      m_calib_strings.at(i) = conf->Get(name, m_calib_strings.at(i));
      m_calib_functions.emplace_back(name.c_str(),
                                     m_calib_strings.at(i).c_str(),
                                     m_calib_range_min, m_calib_range_max);
    }

    EUDAQ_DEBUG("Using configuration:");
    EUDAQ_DEBUG(" blStart   = " + to_string(m_blStart));
    EUDAQ_DEBUG(" blEnd     = " + to_string(m_blEnd));
    EUDAQ_DEBUG(" ampStart  = " + to_string(m_ampStart));
    EUDAQ_DEBUG(" ampEnd    = " + to_string(m_ampStart));
    EUDAQ_DEBUG(" calib_range_min = " + to_string(m_calib_range_min));
    EUDAQ_DEBUG(" calib_range_max = " + to_string(m_calib_range_max));
    EUDAQ_DEBUG("Calibration functions: ");
    if (EUDAQ_IS_LOGGED("DEBUG")) {
      for (unsigned int i = 0; i < m_calib_strings.size(); i++) {
        EUDAQ_DEBUG(to_string(m_calib_functions.at(i).GetName()) + " " +
                    to_string(m_calib_functions.at(i).GetExpFormula()));
      }
    }
    EUDAQ_DEBUG(" calib_range_min  = " + to_string(m_calib_range_min));
    EUDAQ_DEBUG(" calib_range_max  = " + to_string(m_calib_range_max));

    m_configured = true;
  }
  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);
  EUDAQ_DEBUG("Decoding AD event " + to_string(ev->GetEventN() / 2) + " trig " +
              to_string(trig_));

  const size_t header_offset = 8;
  auto datablock0 = ev->GetBlock(0);

  // Get configured burst length from header:
  uint32_t burst_length =
      (static_cast<uint32_t>(datablock0.at(3)) << 8) | datablock0.at(2);

  // Check total available data against expected event size:
  const size_t evt_length = burst_length * 128 * 2 * 16 + 16;
  if (datablock0.size() < evt_length) {
    EUDAQ_WARN("Available date does not match expected event length in event " +
               std::to_string(ev->GetEventNumber()));
    return false;
  }

  // Read waveforms
  std::vector<std::vector<uint16_t> > waveforms;
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

  EUDAQ_DEBUG("Bursts, size_ADC0, size_ADC1 " + to_string(burst_length) + ", " +
              to_string(size_ADC0) + ", " + to_string(size_ADC1));

  // Decode channels:
  uint64_t timestamp0 = 0;
  uint64_t timestamp1 = 0;
  decodeChannel(0, datablock0, size_ADC0, header_offset, waveforms, timestamp0);
  decodeChannel(1, datablock0, size_ADC1, 2 * header_offset + size_ADC0,
                waveforms, timestamp1);

  if (timestamp0 != timestamp1) {
    EUDAQ_WARN("Timestamp inconsistency in event " +
               std::to_string(ev->GetEventNumber()));
    return false;
  }

  // Prepare output plane:
  eudaq::StandardPlane plane(0, "Caribou", "AD9249");
  plane.SetSizeZS(4, 4, 0);

  // print waveforms to file, if a filename is given
  // this returns false! If you want to change that that remove `trig_++`!!!
  if (!m_waveform_filename.empty()) {

    // Open
    m_outfile_waveforms.open(m_waveform_filename, std::ios_base::app); // append

    // Print to file
    for (size_t ch = 0; ch < waveforms.size(); ch++) {
      m_outfile_waveforms << trig_ << " " << ch << " " << mapping.at(ch).first
                          << " " << mapping.at(ch).second << " : ";
      auto const &waveform = waveforms.at(ch);
      for (auto const &sample : waveform) {
        m_outfile_waveforms << sample << " ";
      }
      m_outfile_waveforms << std::endl;
    }

    m_outfile_waveforms.close();
    trig_++;
    return false;
  }

  for (size_t ch = 0; ch < waveforms.size(); ch++) {

    // find waveform maximum
    auto max = std::max_element(waveforms[ch].begin() + m_ampStart,
                                waveforms[ch].begin() + m_ampEnd);
    auto max_posizion = std::distance(waveforms[ch].begin(), max);

    // this means that we will not have an amplitude for some noise events.
    if ((max_posizion - m_blStart) < 0) {
      EUDAQ_DEBUG("  Skipping channel " + to_string(ch) + " max too early");
      continue;
    }

    // calculate waveform baseline
    double baseline = 0.;
    for (int i = max_posizion - m_blStart; i < max_posizion - m_blEnd; i++) {
      baseline += waveforms[ch][i];
    }
    baseline /= m_blStart - m_blEnd;

    // calculate amplitude and apply calibration
    double amplitude = m_calib_functions.at(ch).Eval(*max - baseline);
    if (amplitude > m_calib_range_max)
      amplitude = m_calib_range_max;
    if (amplitude < m_calib_range_min)
      amplitude = 0;

    plane.PushPixel(mapping.at(ch).first, mapping.at(ch).second, amplitude,
                    timestamp0);
  }

  // Add the plane to the StandardEvent
  d2->AddPlane(plane);

  d2->SetTimeBegin(timestamp0);
  d2->SetTimeEnd(timestamp0);
  // This does not work with the TLU, as it also reads this data and discards it
  // later. In any case, if we are using the TLU, we can also sync by timestamp.
  // Also this is probably fixed in corry merge requests !598.
  d2->SetTriggerN(trig_);
  trig_++;

  // Identify the detetor type
  d2->SetDetectorType("AD9249");

  // Indicate that data was successfully converted
  return true;
}
