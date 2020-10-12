#include "CaribouEvent2StdEventConverter.hh"

#include "CLICTDFrameDecoder.hpp"
#include "utils/log.hpp"
#include "CLICTDPixels.hpp"

using namespace eudaq;

namespace{
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::
  Register<CLICTDEvent2StdEventConverter>(CLICTDEvent2StdEventConverter::m_id_factory);
}

bool CLICTDEvent2StdEventConverter::t0_is_high_(false);
size_t CLICTDEvent2StdEventConverter::t0_seen_(0);
uint64_t CLICTDEvent2StdEventConverter::last_shutter_open_(0);
bool CLICTDEvent2StdEventConverter::Converting(eudaq::EventSPC d1, eudaq::StandardEventSP d2, eudaq::ConfigurationSPC conf) const{
  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);

  // Retrieve matrix configuration from config:
  auto counting = conf->Get("countingmode", true);
  auto longcnt = conf->Get("longcnt", false);

  auto pxvalue = conf->Get("pixel_value_toa", false);

  // Integer to allow skipping pixels with certain ToT values directly when decoding
  auto discard_tot_below = conf->Get("discard_tot_below", -1);
  auto discard_toa_below = conf->Get("discard_toa_below", -1);

  static caribou::CLICTDFrameDecoder decoder(longcnt);
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
    LOG(DEBUG) << "CLICTD frame with";

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
        ts64 = (static_cast<uint64_t>(ts) << 32);
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

  // Print all timestamps for first event:
  if(ev->GetEventNumber() == 1) {
    for(auto& timestamp : timestamps) {
      EUDAQ_INFO("TS " + eudaq::to_bit_string((timestamp >> 48), 16, true) + " " + std::to_string(timestamp & 0xffffffffffff));
    }
  }

  // Calculate time stamps, CLICTD runs on 100MHz clock:
  bool shutterIsOpen = false;
  bool full_shutter = false;
  uint64_t shutter_open = 0, shutter_close = 0;
  for(auto& timestamp : timestamps) {
    // LSB 8bit: signal status after the trigger
    // MSB 8bit: which signals triggered
    uint8_t signals = (timestamp >> 48) & 0xff;
    uint8_t triggers = (timestamp >> 56) & 0xff;
    // 48bit of timestamp in 10ns clk
    uint64_t time = (timestamp & 0xffffffffffff) * 10;

    // Old format: MSB all zero
    bool legacy_format = (triggers == 0);

    if(legacy_format) {
      // Find first appearance and first disappearance of SHUTTER signal
      if(signals & 0x4) {
        shutter_open = time;
        shutterIsOpen = true;
      } else if(!(signals & 0x4) && shutterIsOpen == true) {
        shutter_close = time;
        shutterIsOpen = false;
        full_shutter = true;
      }

      // Check for T0 signal going high:
      if(signals & 0x1) {
        t0_is_high_ = true;
      }

      // Check for T0 signal going from high to low
      if(!(signals & 0x1) && t0_is_high_) {
        t0_seen_++;
        t0_is_high_ = false;

        if(t0_seen_ == 1) {
            EUDAQ_INFO("CLIDTD: Detected 1st T0 signal in event: " + std::to_string(ev->GetEventNumber()) + " (ts signal)");
            // Discard this event:
            return false;
        } else {
            // throw exception and interrupt analysis:
            throw DataInvalid("CLICTD: Detected 2nd T0 signal in event: " + std::to_string(ev->GetEventNumber()) + " (ts signal)");
        }
      }
    } else {
      // New format:

      // Find shutter
      if((triggers & 0x4) && (signals & 0x4)) {
        // Trigger plus signal is there: shutter opened
        shutter_open = time;
        shutterIsOpen = true;
      } else if((triggers & 0x4) && !(signals & 0x4) && shutterIsOpen) {
        // Trigger and no signal: shutter closed
        shutter_close = time;
        shutterIsOpen = false;
        full_shutter = true;
      }

      // Check for T0 signal going from high to low
      if((triggers & 0x1) && !(signals & 0x1)) {
        if (time <= 10) {
          t0_seen_++;
          if(t0_seen_ == 1) {
              EUDAQ_INFO("CLICTD: Detected 1st T0 signal directly: T0 flag at " + to_string(time) + "ns");
              // Discard this event:
              return false;
          } else {
              // throw exception and interrupt analysis:
              throw DataInvalid("CLICTD: Detected 2nd T0 signal directly: T0 flag at " + to_string(time) + "ns");
          }
        } else {
          EUDAQ_INFO("CLIDTD: Detected T0 signal directly: T0 flag at " + to_string(time) + "ns. This did not reset the counter and was ignored in the analysis.");
        }
      }
    }
  }

  // If we never saw a shutter open we have a problem:
  if(!full_shutter) {
    EUDAQ_WARN("Frame without shutter timestamps: " + std::to_string(ev->GetEventNumber()));
    return false;
  }

  // Check for a sane shutter, else T0 during shutter open:
  if(shutter_open > shutter_close) {
    EUDAQ_WARN("Frame with shutter close before shutter open: " + std::to_string(ev->GetEventNumber()));
    t0_seen_++;
    if(t0_seen_==1) {
        EUDAQ_INFO("CLICTD: Detected 1st T0 signal indirectly: shutter_close earlier than shutter_open in event " + std::to_string(ev->GetEventNumber()) + " (ts jump)");
        return false;
    } else {
        // throw exception and interrupt analysis:
        throw DataInvalid("CLICTD: Detected 2nd T0 signal indirectly: shutter_close earlier than shutter_open in event " + std::to_string(ev->GetEventNumber()) + " (ts signal)");
    }
  }

  // Check if there was a T0 between shutters:
  // Last shutter open had higher timestamp than this one:
  if (last_shutter_open_ > shutter_open) {
      t0_seen_++;
      // Log when we have it detector:
      if(t0_seen_==1) {
          EUDAQ_INFO("CLICTD: Detected 1st T0 signal indirectly: shutter_open ("
            + to_string(shutter_open) + "ns) earlier than previous shutter_open ("
            + to_string(last_shutter_open_) + "ns), time difference: " + to_string(last_shutter_open_ - shutter_open) + "ns");
          // Discard this event:
          return false;
      } else if (t0_seen_ > 1) {
          // throw exception and interrupt analysis:
          throw DataInvalid("CLICTD: Detected 2nd T0 signal indirectly: shutter_open ("
            + to_string(shutter_open) + "ns) earlier than previous shutter_open ("
            + to_string(last_shutter_open_) + "ns), time difference: " + to_string(last_shutter_open_ - shutter_open) + "ns");
      }
  }

  last_shutter_open_ = shutter_open;

  // FIXME - hardcoded configuration:
  bool drop_before_t0 = true;
  // No T0 signal seen yet, dropping frame:
  if(drop_before_t0 && t0_seen_==0) {
    return false;
  }

  // Decode the data:
  auto data = decoder.decodeFrame(rawdata);

  // Create a StandardPlane representing one sensor plane
  eudaq::StandardPlane plane(0, "Caribou", "CLICTD");

  plane.SetSizeZS(128, 128, 0);
  for(const auto& px : data) {
    auto pixel = dynamic_cast<caribou::CLICTDPixelReadout*>(px.second.get());

    // Disentangle data types from pixel:
    int tot = -1;

    // ToT will throw if longcounter is enabled:
    try {
      tot = pixel->GetTOT();

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
    int rawvalue = tot;

    // Decide whether information is counter of ToA
    if(counting) {
      // FIXME currently we don't use counting mode at all
    } else {
      auto toa = pixel->GetTOA();

      // Check if we want to skip low ToA values. Only skip if we actually read a ToA value and are not on counting mode
      if(toa < discard_toa_below) {
        continue;
      }

      // Select which value to return as pixel raw value
      rawvalue = (pxvalue ? toa : tot);

      // Convert ToA form 100MHz clk into ns and sutract from shutterStopTime
      timestamp = shutter_close - toa * 10;
    }

    // Resolve the eight sub-pixels
    int row = px.first.second;
    for(int sub = 0; sub < 8; sub++) {
      // This pixel hasn't fired:
      if(!pixel->isHit(sub)) {
        continue;
      }

      // "column" times eight plus sub-pixel column
      // LSB of subpixels is the left-most in the mastrix
      int col = px.first.first*8 + sub;

      // Timestamp is stored in picoseconds
      plane.PushPixel(col, row, rawvalue, timestamp * 1000);
    }
  }

  // Add the plane to the StandardEvent
  d2->AddPlane(plane);

  // Store frame begin and end in picoseconds
  d2->SetTimeBegin(shutter_open * 1000);
  d2->SetTimeEnd(shutter_close * 1000);

  // Identify the detetor type
  d2->SetDetectorType("CLICTD");
  // Indicate that data was successfully converted
  return true;
}
