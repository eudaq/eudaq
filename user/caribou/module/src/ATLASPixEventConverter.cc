#include "CaribouEvent2StdEventConverter.hh"
#include "utils/log.hpp"

using namespace eudaq;

namespace{
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::
  Register<ATLASPixEvent2StdEventConverter>(ATLASPixEvent2StdEventConverter::m_id_factory);
}

uint32_t ATLASPixEvent2StdEventConverter::gray_decode(uint32_t gray) const {
  uint32_t bin = gray;
  while(gray >>= 1) {
    bin ^= gray;
  }
  return bin;
}

uint64_t ATLASPixEvent2StdEventConverter::readout_ts_(0);
uint64_t ATLASPixEvent2StdEventConverter::fpga_ts_(0);
uint64_t ATLASPixEvent2StdEventConverter::fpga_ts1_(0);
uint64_t ATLASPixEvent2StdEventConverter::fpga_ts2_(0);
uint64_t ATLASPixEvent2StdEventConverter::fpga_ts3_(0);
bool ATLASPixEvent2StdEventConverter::new_ts1_(false);
bool ATLASPixEvent2StdEventConverter::new_ts2_(false);
bool ATLASPixEvent2StdEventConverter::timestamps_cleared_(false);

bool ATLASPixEvent2StdEventConverter::Converting(eudaq::EventSPC d1, eudaq::StandardEventSP d2, eudaq::ConfigurationSPC conf) const{
  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);

    // Retrieve chip configuration from config:
  auto clkdivend2 = conf->Get("clkdivend2", 7) + 1;
  auto clockcycle = conf->Get("clock_cycle", 8); // value in [ns]

  // No event
  if(!ev || ev->NumBlocks() < 1) {
    return false;
  }

  // Read file and load data
  auto datablock = ev->GetBlock(0);
  uint32_t datain;
  memcpy(&datain, &datablock[0], sizeof(uint32_t));

  // Check if current word is a pixel data:
  if(datain & 0x80000000) {
    // Do not return and decode pixel data before T0 arrived
    if(!timestamps_cleared_) {
      return false;
    }
    // Structure: {1'b1, column_addr[5:0], row_addr[8:0], rise_timestamp[9:0], fall_timestamp[5:0]}
    // Extract pixel data
    long ts2 = gray_decode((datain)&0x003F);
    // long ts2 = gray_decode((datain>>6)&0x003F);
    // TS1 counter is by default half speed of TS2. By multiplying with 2 we make it equal.
    long ts1 = (gray_decode((datain >> 6) & 0x03FF)) << 1;
    // long ts1 = (gray_decode(((datain << 4) & 0x3F0) | ((datain >> 12) & 0xF)))<<1;
    int row = ((datain >> (6 + 10)) & 0x01FF);
    int col = ((datain >> (6 + 10 + 9)) & 0x003F);
    // long tot = 0;

    long long ts_diff = ts1 - static_cast<long long>(readout_ts_ & 0x07FF);

    if(ts_diff > 0) {
      // Hit probably came before readout started and meanwhile an OVF of TS1 happened
      if(ts_diff > 0x01FF) {
        ts_diff -= 0x0800;
      }
    } else {
      // Hit probably came after readout started and after OVF of TS1.
      if(ts_diff < (0x01FF - 0x0800)) {
        ts_diff += 0x0800;
      }
    }

    long long hit_ts = static_cast<long long>(readout_ts_) + ts_diff;

    // Convert the timestamp to nanoseconds:
    uint64_t timestamp = clockcycle * hit_ts;

    // calculate ToT only when pixel is good for storing (division is time consuming)
    int tot = static_cast<int>(ts2 - ((hit_ts % static_cast<long long>(64 * clkdivend2)) / clkdivend2));
    if(tot < 0) {
      tot += 64;
    }

    LOG(DEBUG) << "HIT: TS1: " << ts1 << "\t0x" << std::hex << ts1 << "\tTS2: " << ts2 << "\t0x" << std::hex << ts2
    << "\tTS_FULL: " << hit_ts << "\t" << timestamp << "ns"
    << "\tTOT: " << tot;

    // Create a StandardPlane representing one sensor plane
    eudaq::StandardPlane plane(0, "Caribou", "ATLASPix");
    plane.SetSizeZS(25, 400, 1);
    // Timestamp is stored in picoseconds
    plane.SetPixel(0, col, row, tot, timestamp * 1000);

    // Add the plane to the StandardEvent
    d2->AddPlane(plane);

    // Store time in picoseconds
    d2->SetTimeBegin(timestamp * 1000);
    d2->SetTimeEnd(timestamp * 1000);

    // Identify the detetor type
    d2->SetDetectorType("ATLASPix");
    return true;
  } else {
    // data is not hit information

    // Decode the message content according to 8 MSBits
    unsigned int message_type = (datain >> 24);
    LOG(DEBUG) << "Message type " << std::hex << message_type << std::dec;
    if(message_type == 0b01000000) {
      uint64_t atp_ts = (datain >> 7) & 0x1FFFE;
      long long ts_diff = static_cast<long long>(atp_ts) - static_cast<long long>(fpga_ts_ & 0x1FFFF);

      if(ts_diff > 0) {
        if(ts_diff > 0x10000) {
          ts_diff -= 0x20000;
        }
      } else {
        if(ts_diff < -0x1000) {
          ts_diff += 0x20000;
        }
      }
      readout_ts_ = static_cast<unsigned long long>(static_cast<long long>(fpga_ts_) + ts_diff);
      LOG(DEBUG) << "RO_ts " << std::hex << readout_ts_ << " atp_ts " << atp_ts << std::dec;
    } else if(message_type == 0b00010000) {
      // Trigger counter from FPGA [23:0] (1/4)
    } else if(message_type == 0b00110000) {
      // Trigger counter from FPGA [31:24] and timestamp from FPGA [63:48] (2/4)
      fpga_ts1_ = ((static_cast<unsigned long long>(datain) << 48) & 0xFFFF000000000000);
      new_ts1_ = true;
    } else if(message_type == 0b00100000) {

      // Timestamp from FPGA [47:24] (3/4)
      uint64_t fpga_tsx = ((static_cast<unsigned long long>(datain) << 24) & 0x0000FFFFFF000000);
      if((!new_ts1_) && (fpga_tsx < fpga_ts2_)) {
        fpga_ts1_ += 0x0001000000000000;
        LOG(DEBUG) << "Missing TS_FPGA_1, adding one";
      }
      new_ts1_ = false;
      new_ts2_ = true;
      fpga_ts2_ = fpga_tsx;
    } else if(message_type == 0b01100000) {

      // Timestamp from FPGA [23:0] (4/4)
      uint64_t fpga_tsx = ((datain)&0xFFFFFF);
      if((!new_ts2_) && (fpga_tsx < fpga_ts3_)) {
        fpga_ts2_ += 0x0000000001000000;
        LOG(DEBUG) <<"Missing TS_FPGA_2, adding one";
      }
      new_ts2_ = false;
      fpga_ts3_ = fpga_tsx;
      fpga_ts_ = fpga_ts1_ | fpga_ts2_ | fpga_ts3_;
    } else if(message_type == 0b00000010) {
      // BUSY was asserted due to FIFO_FULL + 24 LSBs of FPGA timestamp when it happened
    } else if(message_type == 0b01110000) {
      // T0 received
      LOG(DEBUG) << "T0 event was found in the data";
      new_ts1_ = false;
      new_ts2_ = false;
      fpga_ts_ = 0;
      fpga_ts1_ = 0;
      fpga_ts2_ = 0;
      fpga_ts3_ = 0;
      timestamps_cleared_ = true;
    } else if(message_type == 0b00000000) {

      // Empty data - should not happen
      LOG(DEBUG) << "EMPTY_DATA";
    } else {

      // Other options...
      // LOG(DEBUG) << "...Other";
      // Unknown message identifier
      if(message_type & 0b11110010) {
        LOG(DEBUG) << "UNKNOWN_MESSAGE";
      } else {
        // Buffer for chip data overflow (data that came after this word were lost)
        if((message_type & 0b11110011) == 0b00000001) {
          LOG(DEBUG) << "BUFFER_OVERFLOW";
        }
        // SERDES lock established (after reset or after lock lost)
        if((message_type & 0b11111110) == 0b00001000) {
          LOG(DEBUG) << "SERDES_LOCK_ESTABLISHED";
        }
        // SERDES lock lost (data might be nonsense, including up to 2 previous messages)
        else if((message_type & 0b11111110) == 0b00001100) {
          LOG(DEBUG) << "SERDES_LOCK_LOST";
        }
        // Unexpected data came from the chip or there was a checksum error.
        else if((message_type & 0b11111110) == 0b00000100) {
          LOG(DEBUG) << "WEIRD_DATA";
        }
        // Unknown message identifier
        else {
          LOG(DEBUG) << "UNKNOWN_MESSAGE";
        }
      }
    }
  }
  return false;
}
