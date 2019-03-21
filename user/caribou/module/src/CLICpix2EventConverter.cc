#include "CaribouEvent2StdEventConverter.hh"

#include "framedecoder/clicpix2_frameDecoder.hpp"
#include "log.hpp"
#include "clicpix2_pixels.hpp"

using namespace eudaq;

namespace{
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::
  Register<CLICpix2Event2StdEventConverter>(CLICpix2Event2StdEventConverter::m_id_factory);
}

bool CLICpix2Event2StdEventConverter::t0_seen_(false);
double CLICpix2Event2StdEventConverter::last_shutter_open_(0);
bool CLICpix2Event2StdEventConverter::Converting(eudaq::EventSPC d1, eudaq::StandardEventSP d2, eudaq::ConfigurationSPC conf) const{
  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);

  // Prepare matrix decoder:
  static auto matrix_config = []() {
    std::map<std::pair<uint8_t, uint8_t>, caribou::pixelConfig> matrix;
    for(uint8_t x = 0; x < 128; x++) {
      for(uint8_t y = 0; y < 128; y++) {
        // FIXME hard-coded matrix configuration for CLICpix2 - needs to be read from a configuration!
        matrix[std::make_pair(y,x)] = caribou::pixelConfig(true, 3, true, false, false);
      }
    }
    return matrix;
  };

  static caribou::clicpix2_frameDecoder decoder(true, true, matrix_config());
  // No event
  if(!ev) {
    return false;
  }

  // Data containers:
  std::vector<uint64_t> timestamps;
  std::vector<uint32_t> rawdata;

  // Retrieve data from event
  if(ev->NumBlocks() == 1) {
    // New data format - timestamps and pixel data are combined in one data block

    // Block 0 contains all data, split it into timestamps and pixel data, returned as std::vector<uint8_t>
    auto datablock = ev->GetBlock(0);
    LOG(DEBUG) << "CLICpix2 frame with";

    // Number of timestamps: first word of data
    uint32_t timestamp_words;
    memcpy(&timestamp_words, &datablock[0], sizeof(uint32_t));
    LOG(DEBUG) << "        " << timestamp_words / 2 << " timestamps from header";

    // Calulate positions and length of data blocks:
    auto timestamp_pos = sizeof(uint32_t);
    auto timestamp_length = timestamp_words * sizeof(uint32_t);
    auto data_pos = timestamp_pos + timestamp_length;
    auto data_length = datablock.size() - timestamp_length - timestamp_pos;

    // Timestamps:
    timestamps.resize(timestamp_length / sizeof(uint64_t));
    memcpy(&timestamps[0], &datablock[0] + timestamp_pos, timestamp_length);
    LOG(DEBUG) << "        " << timestamps.size() << " timestamps retrieved";

    // Pixel data:
    rawdata.resize(data_length / sizeof(uint32_t));
    memcpy(&rawdata[0], &datablock[0] + data_pos, data_length);
    LOG(DEBUG) << "        " << rawdata.size() << " words of frame data";
  } else if(ev->NumBlocks() == 2) {
    // Old data format - timestamps in block 0, pixel data in block 1

    // Block 0 is timestamps:
    auto time = ev->GetBlock(0);
    timestamps.resize(time.size() / sizeof(uint64_t));
    memcpy(&timestamps[0], &time[0],time.size());

    // Block 1 is pixel data:
    auto tmp = ev->GetBlock(1);
    rawdata.resize(tmp.size() / sizeof(unsigned int));
    memcpy(&rawdata[0], &tmp[0],tmp.size());
  } else {
    EUDAQ_WARN("Ignoring bad frame " + std::to_string(ev->GetEventNumber()));
    return false;
  }

  // Calculate time stamps, CLICpix2 runs on 100MHz clock:
  bool shutterOpen = false;
  bool full_shutter = false;
  double shutter_open = 0, shutter_close = 0;
  for(auto& timestamp : timestamps) {
    // Remove first bit (end marker):
    timestamp &= 0x7ffffffffffff;

    if((timestamp >> 48) == 3) {
      shutter_open = static_cast<double>(timestamp & 0xffffffffffff) * 10.;
      shutterOpen = true;
    } else if((timestamp >> 48) == 1 && shutterOpen == true) {
      shutter_close = static_cast<double>(timestamp & 0xffffffffffff) * 10.;
      shutterOpen = false;
      full_shutter = true;
    }
  }

  // If we never saw a shutter open we have a problem:
  if(!full_shutter) {
    EUDAQ_WARN("Frame without shutter timestamps " + std::to_string(ev->GetEventNumber()));
    return false;
  }

  // Check if there was a T0:
  if(!t0_seen_) {
    // Last shutter open had higher timestamp than this one:
    t0_seen_ = (last_shutter_open_ > shutter_open);
  }

  // FIXME - hardcoded configuration:
  bool drop_before_t0 = true;
  // No T0 signal seen yet, dropping frame:
  if(drop_before_t0 && !t0_seen_) {
    return false;
  }

  // Decode the data:
  decoder.decode(rawdata);
  auto data = decoder.getZerosuppressedFrame();

  // Create a StandardPlane representing one sensor plane
  eudaq::StandardPlane plane(0, "Caribou", "CLICpix2");

  int i = 0;
  plane.SetSizeZS(128, 128, data.size());
  for(const auto& px : data) {
    auto cp2_pixel = dynamic_cast<caribou::pixelReadout*>(px.second.get());
    int col = px.first.first;
    int row = px.first.second;

    // Disentangle data types from pixel:
    int tot = -1;

    // ToT will throw if longcounter is enabled:
    try {
      tot = cp2_pixel->GetTOT();
    } catch(caribou::DataException&) {
      // Set ToT to one if not defined.
      tot = 1;
    }

    plane.SetPixel(i, col, row, tot, shutter_open);
    i++;
  }

  // Add the plane to the StandardEvent
  d2->AddPlane(plane);

  // Store frame begin and end:
  d2->SetTimeBegin(shutter_open);
  d2->SetTimeEnd(shutter_close);

  // Indicate that data was successfully converted
  return true;
}
