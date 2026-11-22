#include <eudaq/Configuration.hh>
#include <eudaq/Event.hh>
#include <eudaq/Factory.hh>
#include <eudaq/Logger.hh>
#include <eudaq/Producer.hh>

#include <CAENVMElib.h>
#include <CAENVMEtypes.h>
#include "HidraUtils.hh"

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <set>
#include <thread>
#include <type_traits>
#include <vector>
#include <mutex>

namespace {
using namespace std::chrono_literals;

// SET THESE ACCORDING TO HARDWARE SIGNALS
enum V977IN {
  cFastGate = 0,
  cPhy = 1,
  cPed = 2,
  cSpillStart = 3,
  cSpillEnd = 4
};
enum V977OUT {
  cVeto = static_cast<int>(V977IN::cFastGate),
  cPedVeto = 5,
  cResetSignal = 15
};

enum V560CHAN {
  sFastGate = 0,
  sIsPhys = 1,
  sIsPed = 2,
  sWW = 3,
  sEndOfSpill = 4
};
////////////////////

constexpr std::size_t MAX_BLT_SIZE = 1024 * 4; // TODO check this
constexpr int32_t INVALID_HANDLE = -1;
constexpr uint32_t DATATYPE_FILLER = 0x06000000;
constexpr uint32_t BLT_READ_ADDRESS = 0xAA000000;

// V977 registers.
constexpr uint16_t V977_INPUT_SET_REG = 0x0000;
constexpr uint16_t V977_INPUT_MASK_REG = 0x0002;
constexpr uint16_t V977_INPUT_READ_REG = 0x0004;
constexpr uint16_t V977_OUTPUT_MASK_REG = 0x000C;
constexpr uint16_t V977_OUTPUT_SET_REG = 0x000A;
constexpr uint16_t V977_OUTPUT_CLEAR_REG = 0x0010; // A dummy write access to this register clears all the channels FLIP-FLOP.
constexpr uint16_t V977_SINGLE_READ_REG = 0x0006;


// CAEN V7xx registers used by this producer.
constexpr uint16_t V792_GEO_ADDRESS_REG = 0x1002;
constexpr uint16_t V792_BIT_SET_1_REG = 0x1006;
constexpr uint16_t V792_BIT_CLEAR_1_REG = 0x1008;
constexpr uint16_t V792_BLT_EVENT_NUMBER_REG = 0x1004;
constexpr uint16_t V792_CRATE_SELECT_REG = 0x103C;
constexpr uint16_t V792_IPED_REG = 0x1060;
constexpr uint16_t V775_FULL_SCALE_RANGE_REG = 0x1060;
constexpr uint16_t V792_CONTROL_1_REG = 0x1010;
constexpr uint16_t V792_BIT_SET_2_REG = 0x1032;
constexpr uint16_t V792_BIT_CLEAR_2_REG = 0x1034;
constexpr uint16_t V792_EVENT_COUNTER_RESET_REG = 0x1040;
constexpr uint16_t V792_MCST_CBLT_ADDRESS_REG = 0x101A;
constexpr uint16_t V792_MODEL_HIGH_REG = 0x803A;
constexpr uint16_t V792_MODEL_LOW_REG = 0x803E;
constexpr uint16_t V792_STATUS_1_REG = 0x100E;

constexpr uint16_t V792_MCST_FIRST = 0x02;
constexpr uint16_t V792_MCST_MIDDLE = 0x03;
constexpr uint16_t V792_MCST_LAST = 0x01;

// V775/V775N TDC mode bit in Bit Set/Clear 2. Set = common stop, clear = common start.
constexpr uint16_t V775_COMMON_STOP_BIT = 0x0400;

// CAEN V560 registers used by this producer
constexpr uint16_t V560_STATUS_REG = 0x58;
constexpr uint16_t V560_CLEAR_REG  = 0x50;
// REGISTER FOR CHANNEL READING IS  0x10 + 0x4 * channel;


/// TRACKER SYNC MODULE
// Lower 16 bits are written to base + 0xD000.
// Optional readbacks:
//   base + 0xD000 : loopback/cable readback, if cable is present
//   base + 0xA000 : board readback register
constexpr uint16_t EVSYNC_LOW_WRITE_REG = 0xD000;
constexpr uint16_t EVSYNC_LOW_READ_REG = 0xD000;
constexpr uint16_t EVSYNC_LOW_LOOPBACK_REG = 0xD000;
constexpr uint16_t EVSYNC_LOW_READBACK_REG = 0xA000;

// Optional/debug high word, matching the old debug routine.
// Not needed for the trigger number itself.
constexpr uint16_t EVSYNC_HIGH_WRITE_REG = 0xE000;
constexpr uint16_t EVSYNC_HIGH_LOOPBACK_REG = 0xE000;
constexpr uint16_t EVSYNC_HIGH_READBACK_REG = 0xB000;
constexpr uint16_t EVSYNC_HIGH_READ_REG = 0xE000;



struct BoardConfig {
  std::size_t configIndex;
  uint32_t baseAddr;
  uint16_t geoAddr;
  uint16_t crateNr;
  std::string type;
  uint16_t tdcFullScale;
  std::string tdcMode;
  bool debugRaw;
  uint16_t multicastRole;
  std::string multicastRoleName;
};

struct V977Pattern {
  uint16_t raw = 0;
  bool trigger = false;
  bool physics = false;
  bool pedestal = false;
  bool spillStart = false;
  bool spillEnd = false;
};

std::string hex32(uint32_t value) {
  std::ostringstream oss;
  oss << "0x" << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << value;
  return oss.str();
}

std::string hex16(uint16_t value) {
  std::ostringstream oss;
  oss << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << value;
  return oss.str();
}

uint32_t parse_u32(const std::string& value) {
  std::size_t parsed = 0;
  const unsigned long result = std::stoul(value, &parsed, 0);
  if (parsed != value.size()) {
    throw std::runtime_error("Cannot parse uint32 from: " + value);
  }
  return static_cast<uint32_t>(result);
}

uint16_t parse_u16(const std::string& value) {
  std::size_t parsed = 0;
  const unsigned long result = std::stoul(value, &parsed, 0);
  if (parsed != value.size()) {
    throw std::runtime_error("Cannot parse uint16 from: " + value);
  }
  return static_cast<uint16_t>(result);
}

bool is_disabled(const std::string& value) {
  return value == "0" || value == "false" || value == "False";
}

std::string board_prefix(std::size_t index) {
  return "Board" + std::to_string(index) + ".";
}

std::string uppercase_ascii(std::string value) {
  for (auto& ch : value) {
    if (ch >= 'a' && ch <= 'z') {
      ch = static_cast<char>(ch - 'a' + 'A');
    }
  }
  return value;
}

bool is_qdc_board_type(const std::string& type) {
  return type == "V792" || type == "V792N" || type == "V862";
}

bool is_tdc_board_type(const std::string& type) {
  return type == "V775" || type == "V775N";
}

bool board_enable_key_index(const std::string& key, std::size_t* index) {
  // looks at a config key string and answers: “Is this key shaped like BoardN.Enable, and if yes, what is N?”

  const std::string prefix = "Board";
  const std::string suffix = ".Enable";

  if (key.compare(0, prefix.size(), prefix) != 0 ||
      key.size() <= prefix.size() + suffix.size() ||
      key.compare(key.size() - suffix.size(), suffix.size(), suffix) != 0) {
    return false;
  }

  const std::string indexString = key.substr(prefix.size(), key.size() - prefix.size() - suffix.size());
  if (indexString.empty()) {
    return false;
  }

  for (const auto ch : indexString) {
    if (ch < '0' || ch > '9') {
      return false;
    }
  }

  std::size_t parsed = 0;
  const unsigned long result = std::stoul(indexString, &parsed, 10);
  if (parsed != indexString.size()) {
    return false;
  }

  *index = static_cast<std::size_t>(result);
  return true;
}

} // namespace

class HidraQTPDProducer : public eudaq::Producer {
public:
  HidraQTPDProducer(const std::string& name, const std::string& runcontrol)
      : eudaq::Producer(name, runcontrol),
        m_handle(INVALID_HANDLE),
        m_vmeError(false),
        m_running(false),
        m_runNumber(0),
        m_evt(0),
        m_evt_ped(0),
        m_evt_phy(0),
        m_spillCount(0),
        m_iped(100),
        m_controllerType(cvV2718),
        m_pid(0) {
    ResetReadoutBuffers();
  }

  ~HidraQTPDProducer() override {
    try {
      StopAcquisitionThread();
      CloseController();
    } catch (...) {
    }
  }

  static const uint32_t m_id_factory = eudaq::cstr2hash("HidraQTPDProducer");

private:
  void DoInitialise() override {
    const auto ini = GetInitConfiguration();
    if (!ini) {
      EUDAQ_THROW("Init configuration is missing");
    }

    ConfigureController(*ini);
    OpenController();
    EUDAQ_INFO("CAEN controller initialized");
  }

  void DoConfigure() override {
    const auto conf = GetConfiguration();
    if (!conf) {
      EUDAQ_THROW("Run configuration is missing");
    }

    EUDAQ_LOG_LEVEL((int)(conf->Get("HIDRA_MUTE_DEBUG", 1)));
    ResetRunState();
    LoadRunConfiguration(*conf);


    ConfigureBoards();
    ConfigureV977andVeto();
    ConfigureV560();
    ConfigureEventSyncBoard(); // FOR TRACKER SYNC MODULE

    EUDAQ_INFO("Producer configured");
  }

  void DoStartRun() override {
    StopAcquisitionThread();

    m_runNumber = GetRunNumber();
    ResetRunState();

    for(const auto& board : m_boards) {
      ClearBoardData(board);
    }

    m_running = true;

    SendBORE();
    ResetV977ForRun();
    ClearV560();
    
    m_thread = std::thread(&HidraQTPDProducer::MainLoop, this);
    EUDAQ_INFO("Starting run " + std::to_string(m_runNumber));

    ReleaseTriggerVeto(m_pedestal_run);
    SendStatus();
  }

  void DoStopRun() override {
    m_running = false;
    StopAcquisitionThread();
    if (m_v560Enabled) {
      m_fastGateCount_560 = ReadV560FastGate();
      m_physicsCount_560 = ReadV560Physics();
      m_pedestalCount_560 = ReadV560Pedestal();
      m_wwCount_560 = ReadV560WW();
      m_endOfSpillCount_560 = ReadV560EndOfSpill();
    }
    SendEORE();
    HIDRA_INFO("Stopping run {}", m_runNumber);
    VetoTrigger();
    m_last_status_log = std::chrono::steady_clock::now();
  }

  void DoReset() override {
    StopAcquisitionThread();
    ResetRunState();
    EUDAQ_INFO("Producer reset");
  }

  void DoTerminate() override {
    StopAcquisitionThread();
    CloseController();
    EUDAQ_INFO("Producer terminated");
  }

  void RunLoop() override {
    // This producer owns its acquisition thread so the EUDAQ default loop stays idle.
  }

  void ConfigureController(const eudaq::Configuration& ini) {
    const std::string controller = ini.Get("ControllerType", std::string("V2718"));
    if (controller != "V2718") {
      EUDAQ_THROW("Unsupported ControllerType: " + controller);
    }

    m_controllerType = cvV2718;
    // Use LinkOrPid=49086 for the USB controller path used by older setups.
    m_pid = parse_u32(ini.Get("LinkOrPid", std::string("0")));
  }

  void LoadRunConfiguration(const eudaq::Configuration& conf) {
    m_iped = static_cast<int>(parse_u16(conf.Get("Iped", std::string("100"))));
    m_pedestal_run = conf.Get("PEDESTAL_ONLY", std::string("0")) == "1";
    if (m_pedestal_run) {
      HIDRA_WARN("This run is configured as pedestal-only.");
    }
    m_v977Base = parse_u32(conf.Get("V977_BASE", std::string("0x01000000")));
    m_v560Enabled = conf.Get("V560_ENABLE", conf.Has("V560_BASE") ? std::string("1") : std::string("0")) == "1"; // if V560_BASE is set, enable V560 by default
    m_v560Base = parse_u32(conf.Get("V560_BASE", std::string("0x00200000")));

    // TRACKER SYNC MODULE 
    m_eventSyncEnabled = conf.Get("TRACKER_SYNC_ENABLE", std::string("1")) == "1";
    m_eventSyncBase = parse_u32(conf.Get("TRACKER_SYNC_BASE", std::string("0x00D00000")));
    m_eventSyncReadback = conf.Get("TRACKER_SYNC_READBACK", std::string("0")) == "1";
    m_eventSyncDebug = conf.Get("TRACKER_SYNC_DEBUG", std::string("0")) == "1";
    m_eventSyncWriteHighMarker = conf.Get("TRACKER_SYNC_WRITE_HIGH_MARKER", std::string("0")) == "1";
    m_eventSyncHighMarker = parse_u16(conf.Get("TRACKER_SYNC_HIGH_MARKER", std::string("0xB0B0")));

    m_boards.clear();
    for (const auto index : ConfiguredBoardIndices(conf)) {
      AddBoardFromConf(conf, index);
    }

    if (m_boards.empty()) {
      EUDAQ_THROW("No boards configured");
    }

    AssignMulticastRoles(conf);
  }

  void ConfigureBoards() {
    m_vmeError = false;
    for (const auto& board : m_boards) {
      InitBoard(board);
    }
    ThrowIfVmeError("Board initialization failed");

    ConfigureBlockTransfer();
    ConfigureMulticastChain();
    ThrowIfVmeError("Board readout-chain configuration failed");
  }



  void ConfigureBlockTransfer() {
    for (const auto& board : m_boards) {
      WriteReg(V792_BLT_EVENT_NUMBER_REG, 0xAA, board.baseAddr); 
    }
  }

  void ConfigureMulticastChain() {
    for (const auto& board : m_boards) {
      WriteReg(V792_MCST_CBLT_ADDRESS_REG, board.multicastRole, board.baseAddr);
      HIDRA_INFO("Board{} multicast role: {}", board.configIndex, board.multicastRoleName);
    }
  }


  void ConfigureEventSyncBoard() {  // TRACKER SYNC MODULE
  if (!m_eventSyncEnabled) {
    EUDAQ_INFO("Event-sync board disabled");
    return;
  }

  const uint16_t boardId = ReadReg(0x0000, m_eventSyncBase, cvA24_U_DATA); 

  ThrowIfVmeError("Event-sync board ID read failed");

  EUDAQ_INFO("Event-sync board enabled at " + hex32(m_eventSyncBase) +
             ", ID = 0x" + hex16(boardId));

  if (m_eventSyncDebug) {
    HIDRA_INFO("Event-sync config: base {}, readback {}, writeHighMarker {}, highMarker 0x{}",
               hex32(m_eventSyncBase),
               m_eventSyncReadback,
               m_eventSyncWriteHighMarker,
               hex16(m_eventSyncHighMarker));
  }
}

uint32_t ReadEventSyncTstamp24(){
  if (!m_eventSyncEnabled) return std::numeric_limits<uint32_t>::max();

    uint16_t xhigh = ReadRegSyncModule(EVSYNC_HIGH_READ_REG, m_eventSyncBase);
    uint16_t xlow = ReadRegSyncModule(EVSYNC_LOW_READ_REG, m_eventSyncBase);
    uint32_t tstamp = (static_cast<uint32_t>(xhigh << 16)) | xlow;
    HIDRA_DEBUG("SYNC READ TSTAMP {} -- valid {} (should be 0)", tstamp, (xhigh >> 15));
    return tstamp; 
}

void WriteEventSyncTrigger16(uint64_t triggerNumber) {
  if (!m_eventSyncEnabled) {
    return;
  }

  // THIS IS THE ESSENTIAL OPERATION:
  const uint16_t trigger16 = static_cast<uint16_t>(triggerNumber & 0xFFFFu);
  WriteReg(EVSYNC_LOW_WRITE_REG, trigger16, m_eventSyncBase, cvA24_U_DATA);
  ThrowIfVmeError("Event-sync low trigger write failed");
  //////

  HIDRA_DEBUG("SYNC WRITTEN TRIG {}", trigger16);

  if (m_eventSyncWriteHighMarker) {
    WriteReg(EVSYNC_HIGH_WRITE_REG, m_eventSyncHighMarker, m_eventSyncBase, cvA24_U_DATA);
    ThrowIfVmeError("Event-sync high marker write failed");
  }

  if (m_eventSyncReadback || m_eventSyncDebug) {
    const uint16_t lowLoopback = ReadReg(EVSYNC_LOW_LOOPBACK_REG, m_eventSyncBase, cvA24_U_DATA);
    const uint16_t lowReg = ReadReg(EVSYNC_LOW_READBACK_REG, m_eventSyncBase, cvA24_U_DATA);
    ThrowIfVmeError("Event-sync low trigger readback failed");

    if (m_eventSyncDebug) {
      HIDRA_INFO("Event-sync trigger: evt {} trigger16 0x{} loopback 0x{} reg 0x{}",
                 triggerNumber,
                 hex16(trigger16),
                 hex16(lowLoopback),
                 hex16(lowReg));
    }

    if (m_eventSyncReadback && lowReg != trigger16) {
      HIDRA_WARN("Event-sync readback mismatch for evt {}: wrote 0x{}, register reads 0x{}",
                 triggerNumber,
                 hex16(trigger16),
                 hex16(lowReg));
    }
  }
}

  /// V977 handlers
  void SetSingleV977OutputReg(bool isHigh, int chan) { // chan starts from 0
    std::lock_guard<std::mutex> lock(m_v977OutputSetMutex);
    uint16_t outputSet = ReadReg(V977_OUTPUT_SET_REG, m_v977Base);
    ThrowIfVmeError("V977 output set read failed");

    uint16_t setBitmask = static_cast<uint16_t>(1u << chan);

    if (isHigh) {
      outputSet = outputSet | setBitmask;
    } else {
      outputSet = outputSet & ~setBitmask;
    }

    WriteReg(V977_OUTPUT_SET_REG, outputSet, m_v977Base);
    ThrowIfVmeError("V977 output set write failed");
  }

  void SetAllV977OutputReg(bool isHigh){
    uint16_t setBitMask = isHigh ? 0xFFFF : 0x0000;
    WriteReg(V977_OUTPUT_SET_REG, setBitMask, m_v977Base);
    ThrowIfVmeError("V977 output set write failed");
  }


  void ConfigureV977andVeto() {


    ClearV977FlipFlops();

    
    // Why masking the unused channels?
    /*
    uint16_t output_mask = 0xFFFF;
    output_mask &= ~(1u << V977OUT::cVeto);

    uint16_t input_mask = 0xFFFF;
    input_mask &= ~(1u << V977IN::cFastGate);
    input_mask &= ~(1u << V977IN::cPed);
    input_mask &= ~(1u << V977IN::cPhy);
    WriteReg(V977_INPUT_MASK_REG, input_mask, m_v977Base); // not needed?
    WriteReg(
        V977_OUTPUT_MASK_REG,
        output_mask,
        m_v977Base); // the relevant output is “masked” and no output signal is produced regardless the FLIP FLOPs
                     // status. The output signal can be produced anyway via the relevant bit in the OUTPUT SET register
    */
    
    WriteReg(V977_INPUT_SET_REG, 0x0000, m_v977Base);
    VetoTrigger();
    ThrowIfVmeError("V977 configuration failed");

    EUDAQ_INFO("Initialized I/O at address: " + hex32(m_v977Base));
  }


  void ResetV977ForRun() {
    ClearV977FlipFlops();
    WriteReg(V977_INPUT_SET_REG, 0x0000, m_v977Base);                       // all inputs set to 0
    SetSingleV977OutputReg(true, V977OUT::cResetSignal);
    std::this_thread::sleep_for(std::chrono::microseconds(10));
    SetSingleV977OutputReg(false, V977OUT::cResetSignal);
  }

  //V560 functions

  void ConfigureV560() {
    if (!m_v560Enabled) {
      EUDAQ_INFO("V560 scaler disabled");
      return;
    }
    
    const uint16_t status = ReadReg(V560_STATUS_REG, m_v560Base);
    ThrowIfVmeError("V560 status read failed");

    EUDAQ_INFO("V560 status at " + hex32(m_v560Base) + ": 0x" + hex16(status));
    EUDAQ_INFO("V560 configured at address: " + hex32(m_v560Base));
  }

  void ClearV560() {
    if (!m_v560Enabled) {
      return;
    }
    ReadReg(V560_CLEAR_REG, m_v560Base); // this is a clear register, so we just need to read it to clear the scaler
    ThrowIfVmeError("V560 clear failed");
  }

  uint32_t ReadV560Counter(uint16_t channel) {
    if (!m_v560Enabled) {
      return 0;
    }
    const uint16_t reg = 0x10 + 0x4 * channel;
    uint32_t data = 0;
    const CVErrorCodes ret = CAENVME_ReadCycle(m_handle, m_v560Base + reg, &data, cvA32_U_DATA, cvD32);
    if (ret != cvSuccess) {
      m_errorString = "Cannot read V560 channel at address " + hex32(m_v560Base + reg);
      m_vmeError = true;
    }
    ThrowIfVmeError("V560 channel read failed");
    return data;
  }

  uint32_t ReadV560FastGate() {
    if (!m_v560Enabled) {
      return 0;
    }
    const uint32_t fastgate_count = ReadV560Counter(V560CHAN::sFastGate);
    HIDRA_DEBUG("V560 FastGate count: {}", fastgate_count);
    return fastgate_count;
  }

  uint32_t ReadV560Physics() {
    if (!m_v560Enabled) {
      return 0;
    }
    const uint32_t physics_count = ReadV560Counter(V560CHAN::sIsPhys);
    HIDRA_DEBUG("V560 Physics count: {}", physics_count);
    return physics_count;
  }

  uint32_t ReadV560Pedestal() {
    if (!m_v560Enabled) {
      return 0;
    }
    const uint32_t pedestal_count = ReadV560Counter(V560CHAN::sIsPed);
    HIDRA_DEBUG("V560 Pedestal count: {}", pedestal_count);
    return pedestal_count;
  }

  uint32_t ReadV560WW() {
    if (!m_v560Enabled) {
      return 0;
    }
    const uint32_t ww_count = ReadV560Counter(V560CHAN::sWW);
    HIDRA_DEBUG("V560 WW count: {}", ww_count);
    return ww_count;
  }

  uint32_t ReadV560EndOfSpill() {
    if (!m_v560Enabled) {
      return 0;
    }
    const uint32_t eos_count = ReadV560Counter(V560CHAN::sEndOfSpill);
    HIDRA_DEBUG("V560 End of Spill count: {}", eos_count);
    return eos_count;
  }  

  void VetoTrigger() {
    uint16_t setBitMask = 0x0000 | (1u << V977OUT::cVeto) | (1u << V977OUT::cPedVeto);
    WriteReg(V977_OUTPUT_SET_REG, setBitMask, m_v977Base);
    ThrowIfVmeError("V977 output set write failed while calling VetoTrigger");
    HIDRA_DEBUG("trigger vetoed");
  }

  void ReleaseTriggerVeto(bool releasePedVeto = false) {
    uint16_t setBitMask = 0x0000 | (1u << V977OUT::cPedVeto);
    if (releasePedVeto) setBitMask = 0x0000;
    WriteReg(V977_OUTPUT_SET_REG, setBitMask, m_v977Base);
    ThrowIfVmeError("V977 output set write failed while calling VetoTrigger");
    HIDRA_DEBUG("trigger veto released");
  }

  bool requestPedestalNext(){
    if (m_pedestal_run) return true;
    if (m_evt_ped == 0 && m_evt_phy < 10) return false;
    else if (m_evt_ped == 0) return true;
    else return ((double)m_evt_phy / (double)m_evt_ped) > 10;
  }

  bool CheckBoardsReady(int timeout_us, int sleep_cycle_time_us, int sleep_begin_time_ns = 0) {

    if (sleep_begin_time_ns > 0) {
      std::this_thread::sleep_for(std::chrono::microseconds(sleep_begin_time_ns));
    }

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::microseconds(timeout_us);
    while (std::chrono::steady_clock::now() < deadline) {

      /*
      // ----- Option 1 ----
      // TODO: this would try BLT address, but V792 manual says that 0x100E doesn't work like this for V792N
      const uint16_t statusAll = ReadReg(V792_STATUS_1_REG, BLT_READ_ADDRESS);
      bool atLeastOneReady = (statusAll & (1 << 1)) != 0; // Accessing GLOBAL READY
      bool atLeastOneBusy = (statusAll & (1 << 3)) != 0;  // Accessing GLOBAL BUSY
      if (atLeastOneReady && !atLeastOneBusy) {
        return true;
      }
      // ------------------
      */

      // ----- Option 2 -----
      bool allReady = true;
      bool anyBusy = false;
      for (const auto& board : m_boards) {
        const uint16_t statusb = ReadReg(V792_STATUS_1_REG, board.baseAddr);
        bool thisReady = (statusb & 1) != 0;       // Accessing READY
        bool thisBusy = (statusb & (1 << 2)) != 0; // Accessing BUSY
        allReady &= thisReady;
        anyBusy |= thisBusy;
      }
      if (allReady && !anyBusy) {
        return true;
      }
      // --------------------

      std::this_thread::sleep_for(std::chrono::microseconds(sleep_cycle_time_us));
    }
    return false;
  }

  void MainLoop() {
    uint64_t spill_evt_cnt;
    while (m_running) {
      if (!ControllerIsReady()) {
        continue;
      }

      const V977Pattern pattern = ReadV977FlipFlopPattern();

      if (pattern.spillStart && pattern.spillEnd && !pattern.trigger) {
        // Every events clears the flip flops. So, if here, there was a spill without events
        HIDRA_WARN("Passed through spill {} with no events", m_spillCount);
        m_spillCount++;
        ClearV977FlipFlops();
        spill_evt_cnt = 0;
      }

      if (pattern.trigger){
        spill_evt_cnt++;
        if (pattern.spillStart) {
          m_spillCount++;
        }

        m_evtTimeNs = hidra::utils::getTimens();
        m_TriggerMask = 0x0;
        if (pattern.physics) m_TriggerMask |= 0b01;
        if (pattern.pedestal) m_TriggerMask |= 0b10;
        if (m_TriggerMask == 0b11) {
          HIDRA_WARN("Both ped and phy signals were latched for this evt {}. This will be reported in the trigger mask", m_evt);
        }

	      if (m_v560Enabled){
          m_triggerCount_560 = ReadV560FastGate();
	      }

        HIDRA_DEBUG("Trigger count 16 lsb. Read from V560: {}. Read from event pattern: {}", m_triggerCount_560, m_evt & 0xFFFF);

        if (m_v560Enabled && m_triggerCount_560 != (m_evt & 0xFFFF)) {
          HIDRA_ERROR("Mismatch between trigger count from V560 ({}) and expected from event pattern ({}). You are probably loosing events.", m_triggerCount_560, m_evt & 0xFFFF);
        }

        bool eventHandlingOk = ReadOneBlockAndSendEvent();
        m_evt++;
        if (m_TriggerMask == 0b01) m_evt_phy++;
        if (m_TriggerMask == 0b10) m_evt_ped++;

        WriteEventSyncTrigger16(m_evt); // PROPAGATE NEXT TRIGGER (COUNTER IS ALREADY INCREMENTED) NUMBER TO TRACKER SYNC MODULE

        // TODO: veto is still active and we can do what we want.. but slowing down. Remove and create a dedicated thread
        SetStatusTag("PhyTrigN", std::to_string(m_evt_phy));
        SetStatusTag("PedTrigN", std::to_string(m_evt_ped));
        SetStatusTag("SpillN", std::to_string(m_spillCount));
        SendStatus();
        if (std::chrono::steady_clock::now() - m_last_status_log > 1000ms) {
          m_last_status_log = std::chrono::steady_clock::now();
          HIDRA_INFO("Evt {} mask {}. Sent? {} So far: phy {} ped {} spill {}",
                     m_evt,
                     m_TriggerMask,
                     eventHandlingOk,
                     m_evt_phy,
                     m_evt_ped,
                     m_spillCount);
        }
        HIDRA_DEBUG("Evt {} mask {}. Sent? {} So far: phy {} ped {} spill {}",
                    m_evt,
                    m_TriggerMask,
                    eventHandlingOk,
                    m_evt_phy,
                    m_evt_ped,
                    m_spillCount);
        /////////////////

        // Set pedestal veto if we want pedestal next;
        SetSingleV977OutputReg(!requestPedestalNext(),
                               V977OUT::cPedVeto); // TODO r-m-w not needed if this is the only controlled output

        ClearV977FlipFlops(); // this will release the Trigger veto and clear the spill pattern as well
      } else {
        if(pattern.spillEnd) {
          ClearV977FlipFlops(); // Last clear at end of spill
        }
      }
      if(pattern.spillEnd) {
        HIDRA_INFO("Spill {} ended with {} events", m_spillCount, spill_evt_cnt);
        spill_evt_cnt=0;
      }
      // std::this_thread::sleep_for(std::chrono::microseconds(5)); // TODO: add a sleep here?
      
    }
  }

  bool ControllerIsReady() {
    if (m_handle >= 0) { // TODO: what is exactly m_handle ?
      return true;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    return false;
  }

  V977Pattern ReadV977FlipFlopPattern() {
    V977Pattern pattern;
    pattern.raw = ReadReg(V977_SINGLE_READ_REG, m_v977Base);
    pattern.trigger = (pattern.raw & (1u << V977IN::cFastGate)) != 0;
    pattern.physics = (pattern.raw & (1u << V977IN::cPhy)) != 0;
    pattern.pedestal = (pattern.raw & (1u << V977IN::cPed)) != 0;
    pattern.spillStart = (pattern.raw & (1u << V977IN::cSpillStart)) != 0;
    pattern.spillEnd = (pattern.raw & (1u << V977IN::cSpillEnd)) != 0;
    return pattern;
  }

 
  void ClearV977FlipFlops() {
    WriteReg(V977_OUTPUT_CLEAR_REG, 0xFFFF, m_v977Base);
    ThrowIfVmeError("V977 OUTPUT CLEAR write failed");
  }

  void ClearBoardData(const BoardConfig& board) {
    WriteReg(V792_EVENT_COUNTER_RESET_REG, 0x0, board.baseAddr);
    WriteReg(V792_BIT_SET_2_REG, 0x0004, board.baseAddr);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    WriteReg(V792_BIT_CLEAR_2_REG, 0x0004, board.baseAddr);
    ThrowIfVmeError("Board data clear failed");
  }

  void ConfigureQdcBoard(const BoardConfig& board) {
    WriteReg(V792_IPED_REG, static_cast<uint16_t>(m_iped), board.baseAddr);
    WriteReg(V792_CONTROL_1_REG, 0x60, board.baseAddr);
    WriteReg(V792_BIT_SET_2_REG, 0x0010, board.baseAddr); // disable zero suppression
    WriteReg(V792_BIT_SET_2_REG, 0x0008, board.baseAddr); // disable overrange suppression
    WriteReg(V792_BIT_SET_2_REG, 0x1000, board.baseAddr); // enable empty events
    WriteReg(V792_EVENT_COUNTER_RESET_REG, 0x0, board.baseAddr);
    ClearBoardData(board);
    ThrowIfVmeError("QDC board configuration failed");
    HIDRA_INFO("Configured QDC Board{} ({}) with Iped {}", board.configIndex, board.type, m_iped);
  }

  void ConfigureTdcBoard(const BoardConfig& board) {
    WriteReg(V775_FULL_SCALE_RANGE_REG, board.tdcFullScale, board.baseAddr);
    WriteReg(V792_CONTROL_1_REG, 0x60, board.baseAddr);
    WriteReg(V792_BIT_SET_2_REG, 0x0010, board.baseAddr); // disable zero suppression
    WriteReg(V792_BIT_SET_2_REG, 0x0008, board.baseAddr); // disable overflow suppression
    WriteReg(V792_BIT_SET_2_REG, 0x1000, board.baseAddr); // enable empty events

    if (board.tdcMode == "COMMON_STOP") {
      WriteReg(V792_BIT_SET_2_REG, V775_COMMON_STOP_BIT, board.baseAddr);
      HIDRA_INFO("Setting TDC Board to COMMON_STOP");
    } else {
      WriteReg(V792_BIT_CLEAR_2_REG, V775_COMMON_STOP_BIT, board.baseAddr);
    }

    WriteReg(V792_EVENT_COUNTER_RESET_REG, 0x0, board.baseAddr);
    ClearBoardData(board);
    ThrowIfVmeError("TDC board configuration failed");
    HIDRA_INFO("Configured TDC Board{} ({}) with full-scale register {} and mode {}",
               board.configIndex,
               board.type,
               board.tdcFullScale,
               board.tdcMode);
  }

  void OpenController() {
    if (m_handle >= 0) {
      return;
    }

    uint32_t pid = m_pid;
    const CVErrorCodes ret = CAENVME_Init2(m_controllerType, &pid, 0, &m_handle);
    if (ret != cvSuccess) {
      m_handle = INVALID_HANDLE;
      EUDAQ_THROW("Failed to open CAEN VME controller type " + std::to_string(m_controllerType) +
                  ", ret=" + std::to_string(ret));
    }
  }

  void CloseController() {
    if (m_handle < 0) {
      return;
    }

    int attempt = 0;
    while (CAENVME_End(m_handle) != cvSuccess) {
      if (++attempt > 20) {
        break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    m_handle = INVALID_HANDLE;
  }

  uint16_t ReadReg(uint16_t regAddr, uint32_t baseAddr, CVAddressModifier am = cvA32_U_DATA) {
    uint16_t data = 0;
    const CVErrorCodes ret = CAENVME_ReadCycle(m_handle, baseAddr + regAddr, &data, am, cvD16);
    if (ret != cvSuccess) {
      m_errorString = "Cannot read at address " + hex32(baseAddr + regAddr);
      m_vmeError = true;
    }
    return data;
  }

  uint16_t ReadRegSyncModule(uint16_t regAddr, uint32_t baseAddr) {
    uint16_t data = 0;
    const CVErrorCodes ret = CAENVME_ReadCycle(m_handle, baseAddr + regAddr, &data, cvA24_U_DATA, cvD16);
    if (ret != cvSuccess) {
      m_errorString = "Cannot read at address " + hex32(baseAddr + regAddr);
      m_vmeError = true;
    }
    return data;
  }

  void WriteReg(uint16_t regAddr, uint16_t data, uint32_t baseAddr, CVAddressModifier am = cvA32_U_DATA) {
    const CVErrorCodes ret = CAENVME_WriteCycle(m_handle, baseAddr + regAddr, &data, am, cvD16);
    if (ret != cvSuccess) {
      m_errorString = "Cannot write at address " + hex32(baseAddr + regAddr);
      m_vmeError = true;
    }
  }

  void ThrowIfVmeError(const std::string& context) {
    if (m_vmeError) {
      EUDAQ_THROW(context + ": " + m_errorString);
    }
  }

  void InitBoard(const BoardConfig& board) {
    EUDAQ_INFO("Initializing Board" + std::to_string(board.configIndex) + " (" + board.type +
               ") at " + hex32(board.baseAddr));

    WriteReg(V792_GEO_ADDRESS_REG, board.geoAddr, board.baseAddr);
    WriteReg(V792_BIT_SET_1_REG, 0x80, board.baseAddr);
    ThrowIfVmeError("Board reset failed");

    WriteReg(V792_BIT_CLEAR_1_REG, 0x80, board.baseAddr);

    const int model = (ReadReg(V792_MODEL_LOW_REG, board.baseAddr) & 0xFF) +
                      ((ReadReg(V792_MODEL_HIGH_REG, board.baseAddr) & 0xFF) << 8);
    EUDAQ_INFO("Board model at " + hex32(board.baseAddr) + ": " + std::to_string(model));

    WriteReg(V792_CRATE_SELECT_REG, board.crateNr, board.baseAddr);

    if (is_qdc_board_type(board.type)) {
      ConfigureQdcBoard(board);
    } else if (is_tdc_board_type(board.type)) {
      ConfigureTdcBoard(board);
    } else {
      EUDAQ_THROW("Unsupported Board" + std::to_string(board.configIndex) + ".Type: " + board.type);
    }

    const uint16_t bitSet2 = ReadReg(V792_BIT_SET_2_REG, board.baseAddr);
    EUDAQ_INFO("Board programmed 0x" + hex16(bitSet2));
  }

  bool ReadOneBlockAndSendEvent() { // return false if errors

    int ready_timeout_us = 100; 
    int ready_cycle_us = 10;

    // TODO: not used in 2025, check if needed
    /* 
    if (!CheckBoardsReady(ready_timeout_us, ready_cycle_us)) {
      HIDRA_ERROR("QDC data not ready after {} us", ready_timeout_us);
      return false;
    }
    */

    int byteCount = 0;
    const CVErrorCodes ret = CAENVME_FIFOMBLTReadCycle(m_handle,
                                                       BLT_READ_ADDRESS,
                                                       reinterpret_cast<char*>(m_buffer.data()),
                                                       static_cast<int>(MAX_BLT_SIZE),
                                                       cvA32_U_MBLT,
                                                       &byteCount);

    if (ret != cvSuccess && ret != cvBusError) {
      EUDAQ_ERROR("BLT Error: " + std::to_string(ret));
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    if (byteCount <= 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      HIDRA_ERROR("BCNT = 0, controller replied with 0 bytes. Ret code of BLT was {}",
                  static_cast<std::underlying_type_t<CVErrorCodes>>(ret));
      return false;
    }

    const int wordCount = byteCount / 4;
    if (wordCount <= 0) {
      HIDRA_ERROR("BCNT = 0, controller replied with less than 4 bytes. Ret code of BLT was {}",
                  static_cast<std::underlying_type_t<CVErrorCodes>>(ret));
      return false;
    }

    m_tracker_time = ReadEventSyncTstamp24();

    DumpDebugRawWords(byteCount);

    SendDataEvent(byteCount);
    return true;
  }

  void DumpDebugRawWords(int byteCount) const {
    const int wordCount = byteCount / 4;
    if (wordCount <= 0) {
      return;
    }

    for (const auto& board : m_boards) {
      if (!board.debugRaw) {
        continue;
      }

      std::ostringstream words;
      int matchedWords = 0;
      for (int index = 0; index < wordCount; ++index) {
        const uint32_t word = m_buffer[index];
        if ((word & 0xFE000000) == 0xFE000000) {
          continue;
        }

        const uint8_t geo = static_cast<uint8_t>((word >> 27) & 0x1F);
        if (geo != board.geoAddr) {
          continue;
        }

        if (matchedWords > 0) {
          words << " ";
        }
        words << hex32(word);
        ++matchedWords;
      }

      HIDRA_INFO("Raw XDC words for Board{} geo {} type {} evt {}: {}",
                 board.configIndex,
                 board.geoAddr,
                 board.type,
                 m_evt,
                 matchedWords > 0 ? words.str() : std::string("<none>"));
    }
  }

  void SendDataEvent(int byteCount) {
    auto event = eudaq::Event::MakeUnique("CAENQTPDRaw");
    event->SetTriggerN(static_cast<uint32_t>(m_evt));
    event->SetEventN(static_cast<uint32_t>(m_evt));
    event->SetRunN(static_cast<uint32_t>(m_runNumber));

    const uint8_t* rawBegin = reinterpret_cast<const uint8_t*>(m_buffer.data());
    const std::vector<uint8_t> raw(rawBegin, rawBegin + byteCount);
    if (byteCount != raw.size()) {
      HIDRA_ERROR("Event supposed to have {} bytes. Block with {} bytes", byteCount, raw.size());
    }
    event->AddBlock(0, raw);

    event->SetTag("spillNumber", std::to_string(m_spillCount));
    event->SetTag("triggerMask", std::to_string(m_TriggerMask));
    event->SetTag("endianness", "BE32");
    event->SetTimestamp(m_evtTimeNs, m_evtTimeNs + 100ULL);
    event->SetTag("nativeTimestampBegin", std::to_string(m_tracker_time));
    event->SetTag("detectorDataSize", std::to_string(raw.size()));
    SendEvent(std::move(event));
  }

  void SendBORE() {
    auto bore = eudaq::Event::MakeUnique("CAENQTPDRaw");
    bore->SetBORE();
    bore->SetRunN(static_cast<uint32_t>(m_runNumber));
    bore->SetTag("Producer", "HidraQTPDProducer");
    //bore->SetTag("Iped", std::to_string(m_iped));
    bore->SetTag("NumBoards", std::to_string(m_boards.size()));

    for (std::size_t i = 0; i < m_boards.size(); ++i) {
      bore->SetTag("Board" + std::to_string(i) + "_ConfigIndex", std::to_string(m_boards[i].configIndex));
      //bore->SetTag("Board" + std::to_string(i) + "_Base", hex32(m_boards[i].baseAddr));
      bore->SetTag("Board" + std::to_string(i) + "_Geo", std::to_string(m_boards[i].geoAddr));
      //bore->SetTag("Board" + std::to_string(i) + "_Crate", std::to_string(m_boards[i].crateNr));
      bore->SetTag("Board" + std::to_string(i) + "_Type", m_boards[i].type);
      if (is_tdc_board_type(m_boards[i].type)) {
        bore->SetTag("Board" + std::to_string(i) + "_TdcFullScale", std::to_string(m_boards[i].tdcFullScale));
        bore->SetTag("Board" + std::to_string(i) + "_TdcMode", m_boards[i].tdcMode);
      }
      //bore->SetTag("Board" + std::to_string(i) + "_MulticastRoleName", m_boards[i].multicastRoleName);
    }

    m_runStart = std::chrono::steady_clock::now();
    SendEvent(std::move(bore));
  }

  void SendEORE() {
    auto eore = eudaq::Event::MakeUnique("CAENQTPDRaw");
    eore->SetEORE();
    eore->SetRunN(static_cast<uint32_t>(m_runNumber));
    eore->SetTag("EventsSent", std::to_string(m_evt));
    if (m_v560Enabled) {
      eore->SetTag("V560FastGate", std::to_string(m_fastGateCount_560));
      eore->SetTag("V560Physics", std::to_string(m_physicsCount_560));
      eore->SetTag("V560Pedestal", std::to_string(m_pedestalCount_560));
      eore->SetTag("V560WW", std::to_string(m_wwCount_560));
      eore->SetTag("V560EndOfSpill", std::to_string(m_endOfSpillCount_560));
    }

    const auto runStop = std::chrono::steady_clock::now();
    const auto elapsed = runStop - m_runStart;
    const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
    EUDAQ_INFO("The run has lasted: " + std::to_string(seconds) + " seconds.");

    SendEvent(std::move(eore));
  }

  void StopAcquisitionThread() {
    m_running = false;
    if (m_thread.joinable()) {
      m_thread.join();
    }
  }

  std::set<std::size_t> ConfiguredBoardIndices(const eudaq::Configuration& conf) const {
    std::set<std::size_t> indices;
    for (const auto& key : conf.Keylist()) {
      std::size_t index = 0;
      if (board_enable_key_index(key, &index)) {
        indices.insert(index);
      }
    }
    return indices;
  }

  std::string RequiredConfigValue(const eudaq::Configuration& conf, const std::string& key) const {
    if (!conf.Has(key)) {
      EUDAQ_THROW("Missing required board configuration key: " + key);
    }
    return conf.Get(key, std::string(""));
  }

  void AddBoardFromConf(const eudaq::Configuration& conf, std::size_t index) {
    const std::string prefix = board_prefix(index);
    const std::string enable = conf.Get(prefix + "Enable", std::string("0"));
    if (is_disabled(enable)) {
      return;
    }

    BoardConfig board;
    board.configIndex = index;
    board.baseAddr = parse_u32(RequiredConfigValue(conf, prefix + "BaseAddress"));
    board.geoAddr = parse_u16(RequiredConfigValue(conf, prefix + "GeoAddress"));
    board.crateNr = parse_u16(RequiredConfigValue(conf, prefix + "CrateNumber"));
    board.type = uppercase_ascii(RequiredConfigValue(conf, prefix + "Type"));
    board.tdcFullScale = parse_u16(conf.Get(prefix + "TdcFullScale", std::string("100")));
    board.tdcMode = uppercase_ascii(conf.Get(prefix + "TdcMode", std::string("COMMON_STOP")));
    board.debugRaw = conf.Get(prefix + "DebugRaw", std::string("0")) == "1";

    if (!is_qdc_board_type(board.type) && !is_tdc_board_type(board.type)) {
      EUDAQ_THROW("Unsupported " + prefix + "Type: " + board.type);
    }

    if (is_tdc_board_type(board.type) && board.tdcMode != "COMMON_START" && board.tdcMode != "COMMON_STOP") {
      EUDAQ_THROW("Unsupported " + prefix + "TdcMode: " + board.tdcMode +
                  " (expected COMMON_START or COMMON_STOP)");
    }

    m_boards.push_back(board);
  }

  void AssignMulticastRoles(const eudaq::Configuration& conf) {
    for (std::size_t index = 0; index < m_boards.size(); ++index) {
      BoardConfig& board = m_boards[index];
      const std::string roleKey = board_prefix(board.configIndex) + "MulticastRole";
      if (conf.Has(roleKey)) {
        SetMulticastRole(board, conf.Get(roleKey, std::string("")));
      } else {
        SetMulticastRole(board, DefaultMulticastRoleName(index));
      }
    }
  }

  std::string DefaultMulticastRoleName(std::size_t enabledIndex) const {
    if (enabledIndex == 0) {
      return "FIRST";
    }
    if (enabledIndex + 1 == m_boards.size()) {
      return "LAST";
    }
    return "MIDDLE";
  }

  void SetMulticastRole(BoardConfig& board, const std::string& role) const {
    const std::string normalizedRole = uppercase_ascii(role);
    if (normalizedRole == "FIRST" || normalizedRole == "0X02" || normalizedRole == "2") {
      board.multicastRole = V792_MCST_FIRST;
      board.multicastRoleName = "FIRST";
      return;
    }
    if (normalizedRole == "MIDDLE" || normalizedRole == "0X03" || normalizedRole == "3") {
      board.multicastRole = V792_MCST_MIDDLE;
      board.multicastRoleName = "MIDDLE";
      return;
    }
    if (normalizedRole == "LAST" || normalizedRole == "0X01" || normalizedRole == "1") {
      board.multicastRole = V792_MCST_LAST;
      board.multicastRoleName = "LAST";
      return;
    }

    EUDAQ_THROW("Unsupported " + board_prefix(board.configIndex) + "MulticastRole: " + role);
  }

  void ResetRunState() {
    m_running = false;
    m_evt = m_evt_ped = m_evt_phy = 0;
    m_spillCount = 0;
    m_evtTimeNs = 0;
    m_triggerCount_560 = 0;
    m_spillCount_560 = 0;
    m_fastGateCount_560 = 0;
    m_physicsCount_560 = 0;
    m_pedestalCount_560 = 0;
    m_wwCount_560 = 0;
    m_endOfSpillCount_560 = 0;
  }

  void ResetReadoutBuffers() {
    m_buffer.fill(0);
    m_buffer[0] = DATATYPE_FILLER;
  }

private:
  int32_t m_handle;
  bool m_vmeError;
  std::string m_errorString;

  
  uint32_t m_v977Base = 0;
  uint32_t m_v560Base = 0;

  std::atomic<bool> m_running;
  bool m_pedestal_run = false;

  bool m_v560Enabled = false;

  uint32_t m_runNumber;
  uint64_t m_evt;
  uint64_t m_evt_phy;
  uint64_t m_evt_ped;
  uint32_t m_spillCount; 
  int m_iped;
  uint64_t m_evtTimeNs = 0;
  uint8_t m_TriggerMask = 0xFF; 

  // TRACKER SYNC MODULE
  uint32_t m_eventSyncBase = 0x00D00000;
  bool m_eventSyncEnabled = false;
  bool m_eventSyncReadback = false;
  bool m_eventSyncDebug = false;
  bool m_eventSyncWriteHighMarker = false;
  uint16_t m_eventSyncHighMarker = 0xB0B0;
  uint32_t m_tracker_time = std::numeric_limits<uint32_t>::max();

  //V560 scaler
  uint32_t m_triggerCount_560 = 0;
  uint32_t m_spillCount_560 = 0;
  uint32_t m_fastGateCount_560 = 0;
  uint32_t m_physicsCount_560 = 0;
  uint32_t m_pedestalCount_560 = 0;
  uint32_t m_wwCount_560 = 0;
  uint32_t m_endOfSpillCount_560 = 0;

  std::chrono::steady_clock::time_point m_runStart;
  std::chrono::steady_clock::time_point m_last_status_log = std::chrono::steady_clock::time_point::min();

  CVBoardTypes m_controllerType;
  uint32_t m_pid;

  std::vector<BoardConfig> m_boards;
  std::array<uint32_t, 256 * 1024 / 4> m_buffer;

  std::thread m_thread;
  std::mutex m_v977OutputSetMutex; // Without the mutex, two threads could both read the same old OUTPUT SET value,
                                   // modify different bits, and whichever writes last would accidentally erase the
                                   // other thread’s change.
};

namespace {
auto dummy0 = eudaq::Factory<eudaq::Producer>::Register<HidraQTPDProducer, const std::string&, const std::string&>(
    HidraQTPDProducer::m_id_factory);
}
