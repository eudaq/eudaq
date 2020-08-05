#include "CaribouEvent2StdEventConverter.hh"

#include "framedecoder/clicpix2_frameDecoder.hpp"
#include "utils/log.hpp"
#include "clicpix2_pixels.hpp"

using namespace eudaq;

namespace{
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::
  Register<CLICpix2Event2StdEventConverter>(CLICpix2Event2StdEventConverter::m_id_factory);
}

size_t CLICpix2Event2StdEventConverter::t0_seen_(0);
uint64_t CLICpix2Event2StdEventConverter::last_shutter_open_(0);
bool CLICpix2Event2StdEventConverter::Converting(eudaq::EventSPC d1, eudaq::StandardEventSP d2, eudaq::ConfigurationSPC conf) const{
  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);

  // Retrieve matrix configuration and compression status from config:
  auto counting = conf->Get("countingmode", true);
  auto longcnt = conf->Get("longcnt", false);
  auto comp = conf->Get("comp", true);
  auto sp_comp = conf->Get("sp_comp", true);

  // Integer to allow skipping pixels with certain ToT values directly when decoding
  auto discard_tot_below = conf->Get("discard_tot_below", -1);
  auto discard_toa_below = conf->Get("discard_toa_below", -1);

  // Prepare matrix decoder:
  static auto matrix_config = [counting, longcnt]() {
    std::map<std::pair<uint8_t, uint8_t>, caribou::pixelConfig> matrix;
    for(uint8_t x = 0; x < 128; x++) {
      for(uint8_t y = 0; y < 128; y++) {
        // FIXME hard-coded matrix configuration for CLICpix2 - needs to be read from a configuration!
        matrix[std::make_pair(y,x)] = caribou::pixelConfig(true, 3, counting, false, longcnt);
      }
    }
    return matrix;
  };

  static auto matrix = matrix_config();
  static caribou::clicpix2_frameDecoder decoder(comp, sp_comp, matrix);
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
    std::vector<uint32_t> ts_tmp;
    ts_tmp.resize(timestamp_words);
    memcpy(&ts_tmp[0], &datablock[0] + timestamp_pos, timestamp_length);

    bool msb = true;
    uint64_t ts64;
    for(const auto& ts : ts_tmp) {
      if(msb) {
        ts64 = (static_cast<uint64_t>(ts & 0x7ffff) << 32);
      } else {
        timestamps.push_back(ts64 | ts);
      }
      msb = !msb;
    }
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
  uint64_t shutter_open = 0, shutter_close = 0;
  for(auto& timestamp : timestamps) {
    auto signal_pattern = (timestamp >> 48) & 0x3F;

    if(signal_pattern == 3) {
      shutter_open = (timestamp & 0xffffffffffff) * 10.;
      shutterOpen = true;
    } else if(signal_pattern == 1 && shutterOpen == true) {
      shutter_close = (timestamp & 0xffffffffffff) * 10.;
      shutterOpen = false;
      full_shutter = true;
    }
  }

  // If we never saw a shutter open we have a problem:
  if(!full_shutter) {
    EUDAQ_WARN("Frame without shutter timestamps: " + std::to_string(ev->GetEventNumber()));
    return false;
  }

  // Check if there was a T0:
  if(last_shutter_open_ > shutter_open) {
      t0_seen_++;
  }
  last_shutter_open_ = shutter_open;

  // Check for a sane shutter:
  if(shutter_open > shutter_close) {
    EUDAQ_WARN("Frame with shutter close before shutter open: " + std::to_string(ev->GetEventNumber()));
    return false;
  }

  // FIXME - hardcoded configuration:
  bool drop_before_t0 = true;
  // No T0 signal seen yet, dropping frame:
  if(drop_before_t0 && (t0_seen_==0)) {
    return false;
  }
  // throw exception when T0 occurs more than once:
  if(t0_seen_>1) {
      throw DataInvalid("Detected T0 " + std::to_string(t0_seen_) + " times.");
  }

  // Decode the data:
  decoder.decode(rawdata);
  auto data = decoder.getZerosuppressedFrame();

  // Create a StandardPlane representing one sensor plane
  eudaq::StandardPlane plane(0, "Caribou", "CLICpix2");

  plane.SetSizeZS(128, 128, 0);
  for(const auto& px : data) {
    auto cp2_pixel = dynamic_cast<caribou::pixelReadout*>(px.second.get());
    int col = px.first.first;
    int row = px.first.second;

    // Disentangle data types from pixel:
    int tot = -1;

    // ToT will throw if longcounter is enabled:
    try {
      tot = cp2_pixel->GetTOT();

      // Check if we want to skip low ToT values. Only skip if we actually read a ToT value
      if(tot < discard_tot_below) {
        continue;
      }
    } catch(caribou::DataException&) {
      // Set ToT to one if not defined.
      tot = 1;
    }

    // Time defaults to center between rising and falling shutter edge:
    auto timestamp = (shutter_open + shutter_close) / 2;

    // Decide whether information is counter of ToA
    if(matrix[std::make_pair(row, col)].GetCountingMode()) {
      // FIXME currently we don't use counting mode at all
      // cnt = cp2_pixel->GetCounter();
    } else {
      auto toa = cp2_pixel->GetTOA();

      // Check if we want to skip low ToA values. Only skip if we actually read a ToA value and are not on counting mode
      if(toa < discard_toa_below) {
        continue;
      }

      // Convert ToA form 100MHz clk into ns and sutract from shutterStopTime
      timestamp = shutter_close - toa * 10;
    }

    // Timestamp is stored in picoseconds
    plane.PushPixel(col, row, tot, timestamp * 1000);
  }

  // Add the plane to the StandardEvent
  d2->AddPlane(plane);

  // Store frame begin and end in picoseconds
  d2->SetTimeBegin(shutter_open * 1000);
  d2->SetTimeEnd(shutter_close * 1000);

  // Identify the detetor type
  d2->SetDetectorType("CLICpix2");
  // Indicate that data was successfully converted
  return true;
}
