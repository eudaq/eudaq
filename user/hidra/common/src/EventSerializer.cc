#include "EventSerializer.hh"
#include "HidraUtils.hh"

#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <sys/types.h>

using hidra::utils::getTagOr;

namespace hidra {

namespace {

// LITTLE ENDIAN
template <typename T> void appendLE(std::vector<std::uint8_t>& buffer, T value) {
  static_assert(std::is_integral<T>::value, "appendIntegerLE requires an integer type");

  using UnsignedT = typename std::make_unsigned<T>::type;
  const UnsignedT v = static_cast<UnsignedT>(value);

  for (std::size_t i = 0; i < sizeof(T); ++i) {
    buffer.push_back(static_cast<std::uint8_t>((v >> (8 * i)) & 0xFF));
  }
}

template <typename T> void writeLE(std::vector<std::uint8_t>& buffer, std::size_t offset, T value) {
  using UnsignedT = typename std::make_unsigned<T>::type;
  const UnsignedT v = static_cast<UnsignedT>(value);

  for (std::size_t i = 0; i < sizeof(T); ++i) {
    buffer[offset + i] = static_cast<std::uint8_t>((v >> (8 * i)) & 0xFF);
  }
}

// BIG ENDIAN
template <typename T> void appendBE(std::vector<std::uint8_t>& buffer, T value) {
  static_assert(std::is_integral<T>::value, "appendIntegerBE requires an integer type");

  using UnsignedT = typename std::make_unsigned<T>::type;
  const UnsignedT v = static_cast<UnsignedT>(value);

  for (std::size_t i = 0; i < sizeof(T); ++i) {
    const std::size_t shift = 8 * (sizeof(T) - 1 - i);
    buffer.push_back(static_cast<std::uint8_t>((v >> shift) & 0xFF));
  }
}

template <typename T> void writeBE(std::vector<std::uint8_t>& buffer, std::size_t offset, T value) {
  using UnsignedT = typename std::make_unsigned<T>::type;
  const UnsignedT v = static_cast<UnsignedT>(value);

  for (std::size_t i = 0; i < sizeof(T); ++i) {
    buffer[offset + i] = static_cast<std::uint8_t>((v >> (8 * (sizeof(T) - 1 - i))) & 0xFF);
  }
}

} // end of anonymous namespace

std::vector<std::uint8_t> EventSerializer::Serialize(const eudaq::Event& event) {

  std::vector<std::uint8_t> buffer;

  /*
EVENT HEADER:
marker (16 bit) [0, 1]
data version (8 bit) [2]
header size (32 bit) [3..6]
trailer size (32 bit) [7..10]
event size (including header and trailer) (32 bit) [11..14]
run number (16 bit) [15,16]
event number (32 bit) [17..20]
spill number (32 bit) [21..24]
eventTime (64 bit) [25..32]
triggerMask (8 bit) [33]
reserved (32 bit) [34..37]
eventFlags (32 bit) [38..45]
timeSpread_ns (32 bit) [42..45]
DetectorMask (8 bit) [46]
sizeDet0 (16 bit) [47,48]
sizeDet1 (16 bit) [49,50]
...
sizeDet7 (16 bit) [61,62]
end marker (16 bit) [63,64]

FOR EACH SUBDETECTOR (IF PRESENT):
detEvent marker (16 bit) [0,1]
DetID (8 bit) [2]
event number (32 bit) [3,4,5,6]
spill number (0xFFFFFFFF if not applicable) (32 bit) [7,8,9,10]
eventTime1 (64 bit) [11..18]
eventTime2 (64 bit) [19..16]
reserved (16 bit) [17..18]
trigger mask (8 bit) [19]
Endianness (8 bit) [20]
Blocks (payload)
detEventEndMarker (16 bit)

EVENT TRAILER
marker (16 bit)
  */

  const uint8_t DataFormatVersion = 11;

  const std::uint16_t EVENT_MARKER = 0xB0BF;
  const std::uint16_t EVENT_HEADER_ENDMARKER = 0xBBBB;
  const std::uint16_t EVENT_TRAILER = 0xD04E;
  const std::uint16_t DETECTOR_EVENT_MARKER = 0xDEDE;
  const std::uint16_t DETECTOR_EVENT_ENDMARKER = 0xDDDD;
  uint8_t placeholder8 = 0xFF;
  uint16_t placeholder16 = 0xFFFF;
  uint32_t placeholder32 = std::numeric_limits<uint32_t>::max();
  uint64_t placeholder64 = std::numeric_limits<uint64_t>::max();
  uint8_t reserved8 = 0x0;
  uint16_t reserved16 = 0x0;
  uint32_t reserved32 = 0x0;
  uint64_t reserved64 = 0x0;

  // specific for data format
  const int MAX_N_DETECTORS = 8;

  const uint32_t TrailerSize = 2;

  // TODO: implement missing tags
  appendLE(buffer, EVENT_MARKER);
  appendLE(buffer, DataFormatVersion);
  int anchorpoint_headersize = buffer.size();
  appendLE(buffer, placeholder32); // header size
  appendLE(buffer, TrailerSize);
  int anchorpoint_eventsize = buffer.size();
  appendLE(buffer, placeholder32); // event size
  appendLE(buffer, static_cast<std::uint16_t>(event.GetRunN()));
  appendLE(buffer, static_cast<std::uint32_t>(event.GetTriggerN()));
  appendLE(buffer, getTagOr<std::uint32_t>(event, "spillNumber", 0xFFFFFFFF));
  appendLE(buffer, static_cast<std::uint64_t>(event.GetTimestampBegin()));
  appendLE(buffer, getTagOr<std::uint8_t>(event, "triggerMask", 0xFF));
  appendLE(buffer, reserved32); // reserved
  appendLE(buffer, static_cast<uint32_t>(hidra::utils::GetEventFlagMask(event))); // event flags
  uint64_t timeSpread_ns = event.GetTimestampEnd() > event.GetTimestampBegin() ? (event.GetTimestampEnd() - event.GetTimestampBegin()) : placeholder64;
  if (timeSpread_ns < placeholder32) {
    timeSpread_ns = static_cast<uint32_t>(timeSpread_ns);
  } else {
    timeSpread_ns = placeholder32;
  }
  appendLE(buffer, static_cast<std::uint32_t>(timeSpread_ns)); // time spread
  int anchorpoint_detmask = buffer.size();
  appendLE(buffer, reserved8); // detector mask

  uint32_t NSources = getTagOr<std::uint32_t>(event, "N_SOURCES", 0);
 
  int anchorpoint_detsize = buffer.size();
  for (int is = 0; is < MAX_N_DETECTORS; is++) {
    appendLE(buffer, reserved16); // data size for the subdetector
  }

  appendLE(buffer, EVENT_HEADER_ENDMARKER);

  uint32_t HeaderSize = buffer.size();
  writeLE(buffer, anchorpoint_headersize, HeaderSize);

  uint8_t detMask = 0x00;

  // SERIALIZING SUB-EVENTS
  for (int is = 0; is < NSources; is++) {

    int anchorpoint_subdetector_payload_1 = buffer.size();

    auto sub_ev = event.GetSubEvent(is);
    if (!sub_ev) {
      HIDRA_ERROR("Sub event index {} does not exist for trigger {}. "
                  "N_SOURCES is supposed to be {}. Skipping",
                  is,
                  event.GetTriggerN(),
                  NSources);
      continue;
    }

    std::string producerName = sub_ev->GetTag("Producer");
    std::string detIDs = sub_ev->GetTag("detID");
    int detID = std::stoi(detIDs);

    if (detID >= MAX_N_DETECTORS) {
      HIDRA_ERROR("Detector ID {} exceeds the limit {} for this data format (v.{})."
                  "Skipping",
                  detID,
                  MAX_N_DETECTORS,
                  DataFormatVersion);
      continue;
    }

    detMask |= (1 << detID);

    appendLE(buffer, DETECTOR_EVENT_MARKER);
    appendLE(buffer, static_cast<std::uint8_t>(detID));
    appendLE(buffer, static_cast<std::uint32_t>(sub_ev->GetTriggerN()));
    appendLE(buffer, getTagOr<std::uint32_t>(*sub_ev, "spillNumber", placeholder32, false));
    appendLE(buffer, static_cast<std::uint64_t>(sub_ev->GetTimestampBegin()));
    appendLE(buffer, getTagOr<std::uint64_t>(*sub_ev, "nativeTimestampBegin", placeholder64));
    appendLE(buffer, reserved16);
    appendLE(buffer, getTagOr<std::uint8_t>(*sub_ev, "triggerMask", 0xFF, false));
    std::string endiannessTag = getTagOr(*sub_ev, "endianness", std::string("unknown"));
    uint8_t endianness = 0xFF;
    if (endiannessTag.find("LE") != std::string::npos) {
      endianness &= 0x01;
    }
    if (endiannessTag.find("BE") != std::string::npos) {
      endianness &= 0x02;
    }
    appendLE(buffer, endianness);
    std::vector<uint32_t> block_ids = sub_ev->GetBlockNumList();

    // appending real payload blocks
    for (uint32_t ib : block_ids) {
      auto block = sub_ev->GetBlock(ib);
      buffer.insert(buffer.end(), block.begin(), block.end());
    }

    appendLE(buffer, DETECTOR_EVENT_ENDMARKER);

    int anchorpoint_subdetector_payload_2 = buffer.size();

    int subdetector_payload_size = anchorpoint_subdetector_payload_2 - anchorpoint_subdetector_payload_1;
    uint16_t ev_size_1 = getTagOr<std::uint16_t>(*sub_ev, "detectorDataSize", 0xFEDE);
    if (subdetector_payload_size > 0xFFFF || static_cast<int16_t>(subdetector_payload_size) != ev_size_1 + 33) {
      // TODO: do not hardcode "33"
      HIDRA_ERROR("Subdetector payload size {} don't match the detectorDataSize tag {} for detID {}. ",
                  subdetector_payload_size,
                  ev_size_1,
                  detID);
      // TODO: handle this
    }
    writeLE(buffer, anchorpoint_detsize + 2 * detID, static_cast<std::uint16_t>(subdetector_payload_size & 0xFFFF));
  }

  // updating detector mask
  if (detMask != getTagOr<std::uint8_t>(event, "detectorMask", 0xD5)) { // 0xD5 just an improbable vale
    HIDRA_ERROR("Event trig {}: writing detector mask {} but at source was {}",
                event.GetTriggerN(),
                detMask,
                getTagOr<std::uint8_t>(event, "detectorMask", 0xD5));
  }
  writeLE(buffer, anchorpoint_detmask, detMask);

  // Event trailer

  appendLE(buffer, EVENT_TRAILER);

  uint32_t EventSize = buffer.size();
  writeLE(buffer, anchorpoint_eventsize, EventSize);

  return buffer;
}

int EventSerializer::WriteToStream(const eudaq::Event& event, std::ostream& out) {
  const auto buffer = Serialize(event);

  out.write(reinterpret_cast<const char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));

  if (!out) {
    throw std::runtime_error("Failed to write event to stream");
  }

  return buffer.size();
}

int EventSerializer::WriteToFile(const eudaq::Event& event, const std::string& filename) {

  const auto buffer = Serialize(event);

  std::ofstream output(filename, std::ios::binary);
  if (!output) {
    throw std::runtime_error("Cannot open output file: " + filename);
  }

  output.write(reinterpret_cast<const char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));

  return buffer.size();
}

} // namespace hidra
