#include "Timepix3Event2StdEventConverter.hh"

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

  unsigned long long int stamp = (trigdata & 0x1E0) >> 5;
  long long int timestamp_raw = static_cast<long long int>(trigdata & 0xFFFFFFFFE00) >> 9;
  long long int timestamp = 0;

  // Intermediate bits need to be zero:Header 0x6 indicate trigger data
  if((trigdata & 0x1F) != 0) {
    EUDAQ_WARN("Invalid data found in packet " + std::to_string(ev->GetEventNumber()));
    return false;
  }

  // if jump back in time is larger than 1 sec, overflow detected...
  if((m_syncTimeTDC - timestamp_raw) > 0x1312d000) {
    m_TDCoverflowCounter++;
  }
  m_syncTimeTDC = timestamp_raw;
  timestamp = timestamp_raw + (static_cast<long long int>(m_TDCoverflowCounter) << 35);

  // Calculate timestamp in picoseconds assuming 320 MHz clock:
  uint64_t triggerTime = (timestamp + static_cast<long long int>(stamp) / 12) * 3125;

  // Set timestamps for StdEvent in nanoseconds (timestamps are picoseconds):
  d2->SetTimeBegin(triggerTime);
  d2->SetTimeEnd(triggerTime);
  return true;
}

uint64_t Timepix3RawEvent2StdEventConverter::m_syncTime(0);
bool Timepix3RawEvent2StdEventConverter::m_clearedHeader(false);
bool Timepix3RawEvent2StdEventConverter::Converting(eudaq::EventSPC ev, eudaq::StandardEventSP d2, eudaq::ConfigurationSPC conf) const{

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

        if(!m_clearedHeader && (m_syncTime / 4096 / 40) < 6000000) {
          m_clearedHeader = true;
        }
      }
    }

    // Sometimes there is still data left in the buffers at the start of a run. For that reason we keep skipping data until
    // this "header" data has been cleared, when the heart beat signal starts from a low number (~few seconds max)
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
      const uint64_t timestamp = time * 1000 / 4096 * 25;

      // Set event start/stop:
      event_begin = (timestamp < event_begin) ? timestamp : event_begin;
      event_end = (timestamp > event_end) ? timestamp : event_end;

      // creating new pixel object with non-calibrated values of tot and toa
      plane.PushPixel(col, row, tot, timestamp);
    }
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
