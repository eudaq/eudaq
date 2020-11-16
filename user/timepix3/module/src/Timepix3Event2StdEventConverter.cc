#include "Timepix3Event2StdEventConverter.hh"
#include <cmath> // for sqrt()

using namespace eudaq;

namespace {
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::
  Register<Timepix3RawEvent2StdEventConverter>(Timepix3RawEvent2StdEventConverter::m_id_factory);
  auto dummy1 = eudaq::Factory<eudaq::StdEventConverter>::
  Register<Timepix3TrigEvent2StdEventConverter>(Timepix3TrigEvent2StdEventConverter::m_id_factory);
}

long long int Timepix3TrigEvent2StdEventConverter::m_syncTimeTDC(0);
int Timepix3TrigEvent2StdEventConverter::m_TDCoverflowCounter(0);
bool Timepix3TrigEvent2StdEventConverter::Converting(eudaq::EventSPC ev, eudaq::StandardEventSP d2, eudaq::ConfigurationSPC conf) const{

  // Bad event
  if(ev->NumBlocks() != 1) {
    EUDAQ_WARN("Ignoring bad packet " + std::to_string(ev->GetEventNumber()));
    return false;
  }

  // Retrieve data from Block 0:
  uint64_t trigdata;
  auto data = ev->GetBlock(0);
  if(data.size() / sizeof(uint64_t) > 1) {
    EUDAQ_WARN("Ignoring packet " + std::to_string(ev->GetEventNumber()) + " with unexpected data");
    return false;
  }
  memcpy(&trigdata, &data[0],data.size());

  // Get the header (first 4 bits): 0x4 is the "heartbeat" signal, 0xA and 0xB are pixel data
  const uint8_t header = static_cast<uint8_t>((trigdata & 0xF000000000000000) >> 60) & 0xF;
  const uint8_t header2 = ((trigdata & 0x0F00000000000000) >> 56) & 0xF;

  // Header 0x6 indicate trigger data
  if(header != 0x6 || header2 != 0xF) {
    EUDAQ_WARN("Wrong data header found in packet " + std::to_string(ev->GetEventNumber()));
    return false;
  }

  // format of TDC data from SPIDR:
  // 63   59   55   51   47   43   39   35   31   27   23   19   15   11   7    3  0
  // 0110 1111 EEEE EEEE EEEE TTTT TTTT TTTT TTTT TTTT TTTT TTTT TTTT TTTF FFF0 0000
  // where: E=event_no, T=coarse_timestamp, F=fine_timestamp (0x0 to 0xc)

  unsigned int stamp = (trigdata & 0x1E0) >> 5;
  long long int timestamp_raw = static_cast<long long int>(trigdata & 0xFFFFFFFFE00) >> 9;
  long long int timestamp = 0;

  // Intermediate bits need to be zero:Header 0x6 indicate trigger data
  if((trigdata & 0x1F) != 0) {
    EUDAQ_WARN("Invalid data found in packet " + std::to_string(ev->GetEventNumber()));
    return false;
  }
  if(stamp == 0) {
      // stamp value = 0 indicates a TDC error
      // stamp value ranges from 1 to 12 correspond to coarse timestamp + <0-11>
      EUDAQ_WARN("Invalid TDC stamp received in packet " + std::to_string(ev->GetEventNumber()));
  }

  // if jump back in time is larger than 1 sec, overflow detected...
  if((m_syncTimeTDC - timestamp_raw) > 0x1312d000) {
    m_TDCoverflowCounter++;
  }
  m_syncTimeTDC = timestamp_raw;
  timestamp = timestamp_raw + (static_cast<long long int>(m_TDCoverflowCounter) << 35);

  // Calculate timestamp in picoseconds assuming 320 MHz clock:
  uint64_t triggerTime = timestamp * 3125 +(stamp * 3125) / 12;

  // Set timestamps for StdEvent in nanoseconds (timestamps are picoseconds):
  d2->SetTimeBegin(triggerTime);
  d2->SetTimeEnd(triggerTime);

  // Identify the detetor type
  d2->SetDetectorType("SpidrTrigger");

  return true;
}

uint64_t Timepix3RawEvent2StdEventConverter::m_syncTime(0);
uint64_t Timepix3RawEvent2StdEventConverter::m_syncTime_prev(0);
bool Timepix3RawEvent2StdEventConverter::m_clearedHeader(false);
bool Timepix3RawEvent2StdEventConverter::m_first_time(true);
std::vector<std::vector<float>> Timepix3RawEvent2StdEventConverter::vtot;
std::vector<std::vector<float>> Timepix3RawEvent2StdEventConverter::vtoa;

bool Timepix3RawEvent2StdEventConverter::Converting(eudaq::EventSPC ev, eudaq::StandardEventSP d2, eudaq::ConfigurationSPC conf) const{
  uint64_t delta_t0 = 1e6; // default: 1sec
  // Read from configuration:
  if(m_first_time) {
      delta_t0 = conf->Get("delta_t0", 1e6); // default: 1sec
      EUDAQ_INFO("Will detect 2nd T0 indirectly if timestamp jumps back by more than " + to_string(delta_t0) + "ns.");
      m_first_time = false;

      if(conf->Has("calibration_path_tot") && conf->Has("calibration_path_toa")) {
          std::string calibrationPathToT = conf->Get("calibration_path_tot","");
          std::string calibrationPathToA = conf->Get("calibration_path_toa","");

          if(calibrationPathToT.find("toa") != std::string::npos) {
              throw DataInvalid("Timepix3: Parameter calibration_path_tot contains substring \"toa\", please update your configuration file!");
          }
          if(calibrationPathToA.find("tot") != std::string::npos) {
              throw DataInvalid("Timepix3: Parameter calibration_path_toa contains substring \"tot\", please update your configuration file!");
          }

          EUDAQ_INFO("Applying ToT calibration from " + calibrationPathToT);
          EUDAQ_INFO("Applying ToA calibration from " + calibrationPathToA);

          std::string tmp;
          loadCalibration(calibrationPathToT, ' ', vtot);
          loadCalibration(calibrationPathToA, ' ', vtoa);

            for(size_t row = 0; row < 256; row++) {
                for(size_t col = 0; col < 256; col++) {
                    float a = vtot.at(256 * row + col).at(2);
                    float b = vtot.at(256 * row + col).at(3);
                    float c = vtot.at(256 * row + col).at(4);
                    float t = vtot.at(256 * row + col).at(5);
                    float toa_c = vtoa.at(256 * row + col).at(2);
                    float toa_t = vtoa.at(256 * row + col).at(3);
                    float toa_d = vtoa.at(256 * row + col).at(4);

                    double cold = static_cast<double>(col);
                    double rowd = static_cast<double>(row);
                }
            }
        } else {
            EUDAQ_INFO("No calibration file path for ToT or ToA; data will be uncalibrated.");
        }
    }

  bool data_found = false;

  // No event
  if(!ev || ev->NumBlocks() < 1) {
    return false;
  }

  // Retrieve data from Block 0:
  std::vector<uint64_t> vpixdata;
  auto data = ev->GetBlock(0);
  vpixdata.resize(data.size() / sizeof(uint64_t));
  memcpy(&vpixdata[0], &data[0], data.size());

  // Create a StandardPlane representing one sensor plane
  eudaq::StandardPlane plane(0, "SPIDR", "Timepix3");
  plane.SetSizeZS(256, 256, 0);

  // Event time stamps, defined by first and last pixel timestamp found in the data block:
  uint64_t event_begin = std::numeric_limits<uint64_t>::max(), event_end = std::numeric_limits<uint64_t>::lowest();

  for(const auto& pixdata : vpixdata) {
    // Get the header (first 4 bits): 0x4 is the "heartbeat" signal, 0xA and 0xB are pixel data
    const uint8_t header = static_cast<uint8_t>((pixdata & 0xF000000000000000) >> 60) & 0xF;

    // Use header 0x4 to get the long timestamps (syncTime)
    if(header == 0x4) {
      // The 0x4 header tells us that it is part of the timestamp, there is a second 4-bit header that says if it is the most
      // or least significant part of the timestamp
      const uint8_t header2 = ((pixdata & 0x0F00000000000000) >> 56) & 0xF;

      // This is a bug fix. There appear to be errant packets with garbage data - source to be tracked down.
      // Between the data and the header the intervening bits should all be 0, check if this is the case
      const uint8_t intermediateBits = ((pixdata & 0x00FF000000000000) >> 48) & 0xFF;
      if(intermediateBits != 0x00) {
        continue;
      }

      // 0x4 is the least significant part of the timestamp
      if(header2 == 0x4) {
        // The data is shifted 16 bits to the right, then 12 to the left in order to match the timestamp format (net 4 right)
        m_syncTime = (m_syncTime & 0xFFFFF00000000000) + ((pixdata & 0x0000FFFFFFFF0000) >> 4);
      }
      // 0x5 is the most significant part of the timestamp
      if(header2 == 0x5) {
        // The data is shifted 16 bits to the right, then 44 to the left in order to match the timestamp format (net 28 left)
        m_syncTime = (m_syncTime & 0x00000FFFFFFFFFFF) + ((pixdata & 0x00000000FFFF0000) << 28);

        if(!m_clearedHeader && (m_syncTime / 4096 / 40) < 6000000) { // < 6sec
          EUDAQ_INFO("Timepix3: Detected T0 signal. Header cleared.");
          m_clearedHeader = true;

        // From SPS data we know that even though pixel timestamps are not perfectly chronological, they are not more
        // than "mixed up by -20us". At DESY, this is hardly (ever?) the case due to the lower occupancies.
        // Hence, if the current timestamp is more than 20us earlier than the previous timestamp, we can assume that
        // a 2nd T0 has occured. With some safety margin, set delta_t0 = 1e6 (1s, default).
        // This implies we cannot detect a 2nd T0 within the first "delta_t0" microseconds after the initial T0.
        } else if ((m_syncTime + delta_t0 * 4096 * 40) < m_syncTime_prev) { // delta_t0 on left side to avoid neg. difference between uint64_t
          throw DataInvalid("Timepix3: Detected second T0 signal. Time jumps back by " + to_string((m_syncTime_prev - m_syncTime) / 4096 / 40) + "us.");
        }
        m_syncTime_prev = m_syncTime;
      }
    }

    // Sometimes there is still data left in the buffers at the start of a run. For that reason we keep skipping data until
    // this "header" data has been cleared, when the heart beat signal starts from a low number (~few seconds max).
    // To detect a possible second T0, we have no better gauge than the same criterion:
    // Comparing the timestamp to the previous timestamp (see above).
    if(!m_clearedHeader) {
        continue;
    }

    // Header 0xA and 0xB indicate pixel data
    if(header == 0xA || header == 0xB) {
      data_found = true;
      // Decode the pixel information from the relevant bits
      const uint16_t dcol = static_cast<uint16_t>((pixdata & 0x0FE0000000000000) >> 52);
      const uint16_t spix = static_cast<uint16_t>((pixdata & 0x001F800000000000) >> 45);
      const uint16_t pix = static_cast<uint16_t>((pixdata & 0x0000700000000000) >> 44);
      const uint16_t col = static_cast<uint16_t>(dcol + pix / 4);
      const uint16_t row = static_cast<uint16_t>(spix + (pix & 0x3));

      // Get the rest of the data from the pixel
      // const UShort_t pixno = col * 256 + row;
      const uint32_t data = static_cast<uint32_t>((pixdata & 0x00000FFFFFFF0000) >> 16);
      const uint32_t tot = (data & 0x00003FF0) >> 4;
      const uint64_t spidrTime(pixdata & 0x000000000000FFFF);
      const uint64_t ftoa(data & 0x0000000F);
      const uint64_t toa((data & 0x0FFFC000) >> 14);

      // Calculate the timestamp.
      uint64_t time = (((spidrTime << 18) + (toa << 4) + (15 - ftoa)) << 8) + (m_syncTime & 0xFFFFFC0000000000);

      // Adjusting phases for double column shift
      time += ((static_cast<uint64_t>(col) / 2 - 1) % 16) * 256;

      // The time from the pixels has a maximum value of ~26 seconds. We compare the pixel time to the "heartbeat"
      // signal (which has an overflow of ~4 years) and check if the pixel time has wrapped back around to 0
      while(static_cast<long long>(m_syncTime) - static_cast<long long>(time) > 0x0000020000000000) {
        time += 0x0000040000000000;
      }

      // Convert final timestamp into picoseconds
      uint64_t timestamp = time * 1000 / 4096 * 25;

      // best guess for charge is ToT if no calibration is available
      double charge = static_cast<float>(tot);

      // Apply calibration if both vtot and vtoa are not empty
      // (copied over from Corryvreckan EventLoaderTimepix3)
      if(!vtot.empty() && !vtoa.empty()) {
        EUDAQ_DEBUG("Applying calibration to DUT");
        size_t scol = static_cast<size_t>(col);
        size_t srow = static_cast<size_t>(row);
        float a = vtot.at(256 * srow + scol).at(2);
        float b = vtot.at(256 * srow + scol).at(3);
        float c = vtot.at(256 * srow + scol).at(4);
        float t = vtot.at(256 * srow + scol).at(5);

        float toa_c = vtoa.at(256 * srow + scol).at(2);
        float toa_t = vtoa.at(256 * srow + scol).at(3);
        float toa_d = vtoa.at(256 * srow + scol).at(4);

        // Calculating calibrated tot and toa
        float fvolts = (sqrt(a * a * t * t + 2 * a * b * t + 4 * a * c - 2 * a * t * static_cast<float>(tot) +
                             b * b - 2 * b * static_cast<float>(tot) + static_cast<float>(tot * tot)) +
                        a * t - b + static_cast<float>(tot)) /
                       (2 * a);
        charge = fvolts * 1e-3 * 3e-15 * 6241.509 * 1e15; // capacitance is 3 fF or 18.7 e-/mV // overwrite fcharge variable (was ToT before)

        /* Note 1: fvolts is the inverse to f(x) = a*x + b - c/(x-t). Note the +/- signs! */
        /* Note 2: The capacitance is actually smaller than 3 fC, more like 2.5 fC. But there is an offset when when
         * using testpulses. Multiplying the voltage value with 20 [e-/mV] is a good approximation but means one is
         * over estimating the input capacitance to compensate the missing information of the offset. */

        uint64_t t_shift = (toa_c / (fvolts - toa_t) + toa_d) * 1000; // convert to ps
        timestamp -= t_shift; // apply correction
        EUDAQ_DEBUG("Time shift = " + to_string(t_shift) + "ps");
        EUDAQ_DEBUG("Timestamp calibrated = " + to_string(timestamp) + "ps");

        if(col >= 256 || row >= 256) {
            EUDAQ_WARN("Pixel address " + std::to_string(col) + ", " + std::to_string(row) + " is outside of pixel matrix.");
        }
      }  // end applyCalibration

      // Set event start/stop:
      event_begin = (timestamp < event_begin) ? timestamp : event_begin;
      event_end = (timestamp > event_end) ? timestamp : event_end;

      // creating new pixel object
      plane.PushPixel(col, row, charge, timestamp);
    } // end header 0xA and 0xB indicate pixel data
  }

  // Add the plane to the StandardEvent
  d2->AddPlane(plane);

  // Store event begin and end:
  d2->SetTimeBegin(event_begin);
  d2->SetTimeEnd(event_end);

  // Identify the detetor type
  d2->SetDetectorType("Timepix3");

  return data_found;
}

void Timepix3RawEvent2StdEventConverter::loadCalibration(std::string path, char delim, std::vector<std::vector<float>>& dat) const {
    // copied from Corryvreckan EventLoaderTimepix3
    std::ifstream f;
    f.open(path);
    dat.clear();

    // check if file is open
    if(!f.is_open()) {
        throw DataInvalid("Cannot open calibration file:\n\t" + path);
        return;
    }

    // read file line by line
    int i = 0;
    std::string line;
    while(!f.eof()) {
        std::getline(f, line);

        // check if line is empty or a comment
        // if not write to output vector
        if(line.size() > 0 && isdigit(line.at(0))) {
            std::stringstream ss(line);
            std::string word;
            std::vector<float> row;
            while(std::getline(ss, word, delim)) {
                i += 1;
                row.push_back(stof(word));
            }
            dat.push_back(row);
        }
    }

    // warn if too few entries
    if(dat.size() != 256 * 256) {
        throw DataInvalid("Something went wrong. Found only " + to_string(i) + " entries. Not enough for TPX3.\n\t");
    }

    f.close();
}
