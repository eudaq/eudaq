#include "eudaq/Event.hh"
#include "eudaq/Factory.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Producer.hh"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <deque>
#include <limits>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

#include "HidraUtils.hh"
#include "FERSBoardManager.h"
#include "FERSlib.h"
#undef max
#undef min
#include "FersException.h"

namespace {

using hidra::fers2::BoardMonitorStatus;
using hidra::fers2::FERSBoardManager;
using hidra::fers2::FERSConfiguration;
using hidra::fers2::FERSEvent;
using namespace std::chrono_literals;

std::string FormatSummary(const FERSEvent& event) {
  std::ostringstream oss;
  oss << "board=" << event.board_id << " dq=" << event.data_qualifier << " trig=" << event.trigger_id;
  if (!event.payload.empty()) {
    oss << " bytes=" << event.payload.size();
  }

  const int base_dq = event.data_qualifier & 0x0F;
  if (base_dq == DTQ_SPECT || event.data_qualifier == DTQ_TSPECT) {
    if (event.payload.size() >= sizeof(SpectEvent_t)) {
      const auto* spect = reinterpret_cast<const SpectEvent_t*>(event.payload.data());
      oss << " chmask=0x" << std::hex << spect->chmask << std::dec;
      oss << " nhits=";
      int nhits = 0;
      for (size_t channel = 0; channel < 64; ++channel) {
        if (spect->energyHG[channel] != 0 || spect->energyLG[channel] != 0) {
          ++nhits;
        }
      }
      oss << nhits;
      oss << " HG[0..3]=[" << spect->energyHG[0] << ',' << spect->energyHG[1] << ',' << spect->energyHG[2] << ','
          << spect->energyHG[3] << "]";
      oss << " LG[0..3]=[" << spect->energyLG[0] << ',' << spect->energyLG[1] << ',' << spect->energyLG[2] << ','
          << spect->energyLG[3] << "]";
      if (event.data_qualifier == DTQ_TSPECT) {
        oss << " ToT[0..3]=[" << spect->ToT[0] << ',' << spect->ToT[1] << ',' << spect->ToT[2] << ',' << spect->ToT[3]
            << "]";
      }
    }
  } else if (base_dq == DTQ_TIMING) {
    if (event.payload.size() >= sizeof(ListEvent_t)) {
      const auto* timing = reinterpret_cast<const ListEvent_t*>(event.payload.data());
      oss << " nhits=" << timing->nhits;
      const uint16_t preview = timing->nhits < 4 ? timing->nhits : 4;
      oss << " ch=[";
      for (uint16_t index = 0; index < preview; ++index) {
        if (index != 0) {
          oss << ',';
        }
        oss << static_cast<int>(timing->channel[index]);
      }
      oss << "]";
      oss << " ToA=[";
      for (uint16_t index = 0; index < preview; ++index) {
        if (index != 0) {
          oss << ',';
        }
        oss << timing->ToA[index];
      }
      oss << "]";
      oss << " ToT=[";
      for (uint16_t index = 0; index < preview; ++index) {
        if (index != 0) {
          oss << ',';
        }
        oss << timing->ToT[index];
      }
      oss << "]";
    }
  } else if (base_dq == DTQ_COUNT) {
    if (event.payload.size() >= sizeof(CountingEvent_t)) {
      const auto* counting = reinterpret_cast<const CountingEvent_t*>(event.payload.data());
      oss << " chmask=0x" << std::hex << counting->chmask << std::dec;
      oss << " counts[0..3]=[" << counting->counts[0] << ',' << counting->counts[1] << ',' << counting->counts[2] << ','
          << counting->counts[3] << "]";
      oss << " T_OR=" << counting->t_or_counts << " Q_OR=" << counting->q_or_counts;
    }
  } else if (base_dq == DTQ_SERVICE) {
    if (event.payload.size() >= sizeof(ServEvent_t)) {
      const auto* service = reinterpret_cast<const ServEvent_t*>(event.payload.data());
      oss << " service trig=" << service->TotTrg_cnt << " rej=" << service->RejTrg_cnt
          << " suppr=" << service->SupprTrg_cnt << " hv=" << service->hv_Vmon << "V"
          << " temp=" << service->tempBoard << "C";
    }
  }

  return oss.str();
}

int IsMonitorStatusOk(const BoardMonitorStatus& status) { // 0 means all good
  int retcode = 0;
  if (status.hv_vmon_valid == 0) {
    retcode = 1;
  }
  if (status.hv_vmon < 40) {
    retcode = 2;
  }
  if (status.hv_vmon > 46) {
    retcode = 3;
  }
  if (status.hv_imon_valid == 0) {
    retcode = 4;
  }
  if (status.hv_on == 0) {
    retcode = 10;
  }
  if (status.hv_ramping != 0) {
    retcode = 11;
  }
  if (status.hv_over_current != 0) {
    retcode = 12;
  }
  if (status.hv_over_voltage != 0) {
    retcode = 13;
  }

  if (retcode != 0) {
    return status.board_id * 1000 + retcode;
  } else {
    return 0;
  }
}

std::string FormatMonitorStatus(const BoardMonitorStatus& status) {
  std::ostringstream oss;
  oss << "board=" << status.board_id;
  if (status.hv_vmon_valid) {
    oss << " Vmon=" << status.hv_vmon << "V";
  }
  if (status.hv_imon_valid) {
    oss << " Imon=" << status.hv_imon << "mA";
  }
  if (status.detector_temp_valid) {
    oss << " Tdet=" << status.temp_detector << "C";
  }
  if (status.fpga_temp_valid) {
    oss << " Tfpga=" << status.temp_fpga << "C";
  }
  if (status.board_temp_valid) {
    oss << " Tboard=" << status.temp_board << "C";
  }
  if (status.hv_temp_valid) {
    oss << " Thv=" << status.temp_hv << "C";
  }
  if (status.tdc0_temp_valid) {
    oss << " Ttdc0=" << status.temp_tdc0 << "C";
  }
  if (status.tdc1_temp_valid) {
    oss << " Ttdc1=" << status.temp_tdc1 << "C";
  }
  if (status.hv_status_valid) {
    oss << " HV(on=" << status.hv_on << ",ramp=" << status.hv_ramping << ",ovc=" << status.hv_over_current
        << ",ovv=" << status.hv_over_voltage << ")";
  }
  return oss.str();
}

int ParseIntegerOrKeyword(const std::string& value, const std::map<std::string, int>& keywords, int fallback) {
  try {
    size_t consumed = 0;
    const int parsed = std::stoi(value, &consumed, 0);
    if (consumed == value.size()) {
      return parsed;
    }
  } catch (...) {
  }

  const auto it = keywords.find(value);
  if (it != keywords.end()) {
    return it->second;
  }

  return fallback;
}

std::string JoinBoardIds(const std::vector<int>& board_ids) {
  std::ostringstream oss;
  for (size_t index = 0; index < board_ids.size(); ++index) {
    if (index != 0) {
      oss << ",";
    }
    oss << board_ids[index];
  }
  return oss.str();
}

constexpr auto kAlignmentWaitIdleTimeoutUs = 4s;
constexpr auto kAlignmentWaitAheadTimeoutUs = 3s;

} // namespace

class HidraFERS2Producer : public eudaq::Producer {
public:
  HidraFERS2Producer(const std::string& name, const std::string& runcontrol)
      : eudaq::Producer(name.empty() ? "HidraFERS2Producer" : name, runcontrol) {
        HIDRA_INFO("FERS2 Producer instantiated with name: {}", name);
      }

  static const uint32_t m_id_factory_fers2 = eudaq::cstr2hash("HidraFERS2Producer");
  static const uint32_t m_id_factory_maxicc = eudaq::cstr2hash("HidraMAXICCProducer");

private:
  void DoInitialise() override {
    auto ini = GetInitConfiguration();
    (void)ini;
  }

  void DoConfigure() override {
    auto conf = GetConfiguration();
    m_evt_f = 0;
    if (!conf) {
      HIDRA_THROW("Run configuration is missing");
    }

    EUDAQ_LOG_LEVEL((int)(conf->Get("HIDRA_MUTE_DEBUG", 1)));
    m_config_file = conf->Get("FERS_CONF_FILE", std::string("/home/eudaq/daq/Janus_Config_EUDAQ_TB2026.txt"));
    if (m_config_file.empty()) {
      HIDRA_THROW("FERS_CONF_FILE is missing from the run configuration");
    }

    //m_readout_mode = conf->Get("FERS_READOUT_MODE", 0);
    m_readout_mode = 0; //Fix readout to disable event building
    m_configure_mode = ParseIntegerOrKeyword(conf->Get("FERS_CONFIGURE_MODE", std::string("CFG_HARD")),
                                             {{"CFG_HARD", CFG_HARD}, {"CFG_SOFT", CFG_SOFT}},
                                             CFG_HARD);
    m_start_mode = ParseIntegerOrKeyword(conf->Get("FERS_START_MODE", std::string("TDL")),
                                         {{"ASYNC", STARTRUN_ASYNC},
                                          {"CHAIN_T0", STARTRUN_CHAIN_T0},
                                          {"CHAIN_T1", STARTRUN_CHAIN_T1},
                                          {"TDL", STARTRUN_TDL},
                                          {"STARTRUN_ASYNC", STARTRUN_ASYNC},
                                          {"STARTRUN_CHAIN_T0", STARTRUN_CHAIN_T0},
                                          {"STARTRUN_CHAIN_T1", STARTRUN_CHAIN_T1},
                                          {"STARTRUN_TDL", STARTRUN_TDL}},
                                         STARTRUN_ASYNC);

    m_poll_sleep_us = conf->Get("FERS_POLL_SLEEP_US", 0);
    // `FERS_MAX_EVENTS_PER_BOARD` historically limited events per board. With
    // concentrator/TDL batch reads this parameter now acts as a global cap on
    // the total number of FERSEvents returned by `ReadAvailableEvents` in a
    // single call. A value of 0 means "no limit" (run until no data).
    m_max_total_events = conf->Get("FERS_MAX_EVENTS_PER_BOARD", 0);
    m_send_timestamp = conf->Get("FERS_SEND_TIMESTAMP", 1) != 0;
    m_status_poll_interval_s = conf->Get("FERS_STATUS_POLL_INTERVAL_S", 1);
    m_attach_status_tags = conf->Get("FERS_STATUS_ATTACH_TAGS", 0) != 0;
    if (m_status_poll_interval_s < 0) {
      HIDRA_WARN("FERS_STATUS_POLL_INTERVAL_S is negative; disabling FERS2 status polling");
      m_status_poll_interval_s = 0;
    }

    std::string error;
    if (!FERSConfiguration::FromFile(m_config_file, &m_config, &error)) {
      HIDRA_THROW("Cannot parse FERS configuration file: " + error);
    }

    try {
      m_board_manager = std::make_unique<FERSBoardManager>(m_config, 0, m_readout_mode, m_configure_mode);
    } catch (const hidra::fers2::FersError& e) {
      HIDRA_THROW(std::string("Failed to build FERS boards: ") + e.what());
    }

    // if (!m_board_manager.SetHighVoltageAll(false, &error)) {
    //   m_board_manager.DisconnectAll(nullptr);
    //   EUDAQ_THROW(error);
    // }

    m_board_ids.clear();
    m_board_ids.reserve(m_board_manager->boards().size());
    for (const auto& board : m_board_manager->boards()) {
      m_board_ids.push_back(board.board_id());
      m_event_queues[board.board_id()] = {};
    }

    HIDRA_INFO("Configured " + std::string(GetName()) + " boards: " + JoinBoardIds(m_board_ids));
    if (m_status_poll_interval_s > 0) {
      HIDRA_INFO(std::string(GetName()) + " status polling enabled every " +
                 std::to_string(m_status_poll_interval_s) + " s");
    }
  }

  void DoStartRun() override {
    m_exit_of_run = false;
    m_evt_f = 0;
    m_stamp_last_sent_ns = 0;
    m_evt_incomplete = 0;
    SetStatusTag("PartialEvts", std::to_string(m_evt_incomplete));
    m_run_number = GetRunNumber();
    m_event_queues.clear();
    m_monitor_status.clear();
    m_next_status_poll = std::chrono::steady_clock::time_point::min();
    m_last_event_read = std::chrono::steady_clock::time_point::min();
    m_last_status_log = std::chrono::steady_clock::now();
    m_alignment_wait_trigger = 0;
    m_alignment_wait_active = false;
    for (const auto& board : m_board_manager->boards()) {
      m_event_queues[board.board_id()] = {};
    }

    auto bore = eudaq::Event::MakeUnique(GetName());
    bore->SetBORE();
    bore->SetRunN(static_cast<uint32_t>(m_run_number));
    bore->SetTag("Producer", GetName());
    bore->SetTag("FERS_CONF_FILE", m_config_file);
    SendEvent(std::move(bore));

    std::string error;

    // if (!m_board_manager.SetHighVoltageAll(true, &error)) {
    //   EUDAQ_THROW(error);
    // }

    m_start_boards_call_ts_ns = hidra::utils::getTimens();
    if (!m_board_manager->StartAll(m_start_mode, m_run_number, &error)) {
      HIDRA_THROW(error);
    }

    HIDRA_INFO("Starting " + std::string(GetName()) + " run " + std::to_string(m_run_number));
    SendStatus();
  }

  void DoStopRun() override {
    m_exit_of_run = true;
    std::string error;
    if (m_board_manager && !m_board_manager->StopAll(m_start_mode, m_run_number, &error)) {
      HIDRA_WARN(error);
    }

    // if (!m_board_manager.SetHighVoltageAll(false, &error)) {
    //   EUDAQ_WARN(error);
    // }

    auto eore = eudaq::Event::MakeUnique(GetName());
    eore->SetEORE();
    eore->SetRunN(static_cast<uint32_t>(m_run_number));
    SendEvent(std::move(eore));

    HIDRA_INFO("Stopping " + std::string(GetName()) + " run " + std::to_string(m_run_number));
  }

  void DoReset() override {
    m_exit_of_run = true;
    m_evt_f = 0;
    std::string error;
    if (m_board_manager && !m_board_manager->StopAll(m_start_mode, m_run_number, &error)) {
      HIDRA_WARN(error);
    }

    // if (!m_board_manager.SetHighVoltageAll(false, &error)) {
    //   EUDAQ_WARN(error);
    // }

    m_board_manager.reset();
    m_board_ids.clear();
    m_event_queues.clear();
    m_monitor_status.clear();
    m_alignment_wait_trigger = 0;
    m_alignment_wait_active = false;
  }

  void DoTerminate() override {
    m_exit_of_run = true;
    std::string error;
    if (m_board_manager && !m_board_manager->StopAll(m_start_mode, m_run_number, &error)) {
      HIDRA_WARN(error);
    }

    // if (!m_board_manager.SetHighVoltageAll(false, &error)) {
    //   EUDAQ_WARN(error);
    // }

    m_board_manager.reset();
  }

  void RunLoop() override {
    while (!m_exit_of_run) {
      std::string error;

      PollMonitorStatusIfDue();

      uint64_t ts = hidra::utils::getTimens();
      const auto events = m_board_manager->ReadAvailableEvents(m_max_total_events, &error);
      if (events.size() > 0) {
        HIDRA_DEBUG(
            "ReadAvailableEvents took {} ns to read {} FERSEvents", hidra::utils::getTimens() - ts, events.size());
        m_last_event_read = std::chrono::steady_clock::now();
      }

      if (!error.empty()) {
        HIDRA_THROW(error);
      }

      for (const auto& event : events) {
        HIDRA_DEBUG("Read FERS event {}", FormatSummary(event));
        m_event_queues[event.board_id].push_back(event);
      }

      FlushAlignedEvents();

      if (m_poll_sleep_us > 0) {
        std::this_thread::sleep_for(std::chrono::microseconds(m_poll_sleep_us));
      }
    }

    FlushAlignedEvents();
  }

  void PollMonitorStatusIfDue() {
    const auto now = std::chrono::steady_clock::now();
    const bool interval_poll_due =
        m_status_poll_interval_s > 0 &&
        (m_next_status_poll == std::chrono::steady_clock::time_point::min() || now >= m_next_status_poll);
  
    if (!interval_poll_due) {
      return;
    }

    std::string error;
    auto statuses = m_board_manager->ReadMonitorStatuses(&error);
    if (!error.empty()) {
      HIDRA_WARN(error);
    }

    const uint64_t read_time_ns = hidra::utils::getTimens();
    int monitorstatus = 0;
    for (auto& status : statuses) {
      status.read_time_ns = read_time_ns;
      HIDRA_INFO(std::string(GetName()) + " monitor " + FormatMonitorStatus(status));
      m_monitor_status[status.board_id] = status;
      if (monitorstatus == 0) {
        monitorstatus = IsMonitorStatusOk(status);
      }
    }

    if (statuses.empty()) {
      SetStatusTag("BoardsMon", "NOK_READFAIL");
    }
    else{
      SetStatusTag("BoardsMon", monitorstatus == 0 ? "OK" : "NOK_" + std::to_string(monitorstatus));
    }
    
    SendStatus();

    if (m_status_poll_interval_s > 0) {
      m_next_status_poll = now + std::chrono::seconds(m_status_poll_interval_s);
    }
  }

  void AddMonitorStatusTags(eudaq::Event& event) const {
    if (!m_attach_status_tags) {
      return;
    }

    for (const auto& entry : m_monitor_status) {
      const auto& status = entry.second;
      const std::string prefix = "FERS_STATUS_B" + std::to_string(status.board_id) + "_";
      event.SetTag(prefix + "READ_TIME_NS", std::to_string(status.read_time_ns));
      if (status.hv_vmon_valid) {
        event.SetTag(prefix + "VMON_V", std::to_string(status.hv_vmon));
      }
      if (status.hv_imon_valid) {
        event.SetTag(prefix + "IMON_MA", std::to_string(status.hv_imon));
      }
      if (status.detector_temp_valid) {
        event.SetTag(prefix + "TEMP_DETECTOR_C", std::to_string(status.temp_detector));
      }
      if (status.fpga_temp_valid) {
        event.SetTag(prefix + "TEMP_FPGA_C", std::to_string(status.temp_fpga));
      }
      if (status.board_temp_valid) {
        event.SetTag(prefix + "TEMP_BOARD_C", std::to_string(status.temp_board));
      }
      if (status.hv_temp_valid) {
        event.SetTag(prefix + "TEMP_HV_C", std::to_string(status.temp_hv));
      }
      if (status.tdc0_temp_valid) {
        event.SetTag(prefix + "TEMP_TDC0_C", std::to_string(status.temp_tdc0));
      }
      if (status.tdc1_temp_valid) {
        event.SetTag(prefix + "TEMP_TDC1_C", std::to_string(status.temp_tdc1));
      }
      if (status.hv_status_valid) {
        event.SetTag(prefix + "HV_ON", std::to_string(status.hv_on));
        event.SetTag(prefix + "HV_RAMPING", std::to_string(status.hv_ramping));
        event.SetTag(prefix + "HV_OVERCURRENT", std::to_string(status.hv_over_current));
        event.SetTag(prefix + "HV_OVERVOLTAGE", std::to_string(status.hv_over_voltage));
      }
    }
  }

  void ResetAlignmentWaitState() {
    m_alignment_wait_active = false;
    m_alignment_wait_trigger = 0;
  }

  bool HasAnyQueuedEvents() const {
    for (int board_id : m_board_ids) {
      if (!m_event_queues.at(board_id).empty()) {
        return true;
      }
    }
    return false;
  }

  uint64_t SelectAlignmentTrigger() {
    uint64_t trigger_n = std::numeric_limits<uint64_t>::max();
    for (int board_id : m_board_ids) {
      const auto& queue = m_event_queues.at(board_id);
      if (!queue.empty()) {
        trigger_n = std::min(trigger_n, queue.front().trigger_id);
        m_alignment_wait_start = queue.front().acq_time;
      }
    }
    return trigger_n;
  }

  struct BoardMatchResult {
    std::vector<int> matched_boards;
    bool any_board_ahead = false;
  };
  
  BoardMatchResult InspectQueuesForTrigger(uint64_t trigger_n) const {
    BoardMatchResult result;
    result.matched_boards.reserve(m_board_ids.size());

    HIDRA_DEBUG("Searching for trigger {} in fers boards", trigger_n);
    for (int board_id : m_board_ids) {
      const auto& queue = m_event_queues.at(board_id);
      if (queue.empty()) {
        HIDRA_DEBUG("Empty queue for board {}", board_id);
        continue;
      }

      const uint64_t front_id = queue.front().trigger_id;
      HIDRA_DEBUG("Found trigger id {} at front of queue for board {}", front_id, board_id);

      if (front_id == trigger_n) {
        result.matched_boards.push_back(board_id);
      } else if (front_id > trigger_n) {
        result.any_board_ahead = true;
        HIDRA_DEBUG("Board {} is ahead of expected trigger, skipping", board_id);
      }
    }

    return result;
  }

  void EmitAlignedEvent(uint64_t trigger_n, const std::vector<int>& matched_boards) {
    if (matched_boards.empty()) {
      return;
    }

    if (matched_boards.size() != m_board_ids.size()) {
      std::ostringstream missing;
      bool first = true;
      std::unordered_set<int> matched_set(matched_boards.begin(), matched_boards.end());
      for (int board_id : m_board_ids) {
        if(!matched_set.count(board_id))
        {
          if (!first) {
            missing << ", ";
          }
          first = false;
          missing << board_id;
        }
      }

      HIDRA_WARN(std::string(GetName()) + " alignment gap at trigger " + std::to_string(trigger_n) +
                 ", missing boards: " + missing.str());
    }

    auto ev = eudaq::Event::MakeUnique(GetName());
    ev->SetTag("Producer", GetName());
    ev->SetTriggerN(trigger_n); // TODO: do we want to differentiate the two?
    ev->SetEventN(trigger_n);

    std::size_t total_payload_bytes = 0;
    bool timestamp_set = false;

    for (int board_id : m_board_ids) {
      auto& queue = m_event_queues[board_id];
      if (queue.empty() || queue.front().trigger_id != trigger_n) {
        continue;
      }

      const auto& event = queue.front();
      if (m_send_timestamp && !timestamp_set) {
        uint64_t start_ts_ns = event.timestamp_us > 0.0 ? static_cast<uint64_t>(1000 * event.timestamp_us) : 0u;
        start_ts_ns += m_start_boards_call_ts_ns;
        ev->SetTimestamp(start_ts_ns, start_ts_ns + 100UL);
        timestamp_set = true;
      }

      HIDRA_DEBUG("Building payload for board " + std::to_string(board_id) + ", trigger id " +
                  std::to_string(trigger_n));

      uint16_t BOARD_BLOCK_MARKER = 0xAAAA;
      uint32_t dataqualifier = std::numeric_limits<uint32_t>::max();
      if (event.data_qualifier > 0) {
        dataqualifier = static_cast<uint32_t>(event.data_qualifier);
      } else {
        HIDRA_ERROR("Assigning dummy qualifier to {} trig ID {}. Qualifier was {}",
                    GetName(),
                    trigger_n,
                    event.data_qualifier);
      }
      const uint16_t ext_block_size = static_cast<uint16_t>(event.payload.size() + 9u);
      m_emit_scratch_buffer.resize(ext_block_size);
      auto& ext_block = m_emit_scratch_buffer;
      ext_block[0] = static_cast<uint8_t>(BOARD_BLOCK_MARKER);
      ext_block[1] = static_cast<uint8_t>(BOARD_BLOCK_MARKER >> 8);
      ext_block[2] = static_cast<uint8_t>(ext_block_size);
      ext_block[3] = static_cast<uint8_t>(ext_block_size >> 8);
      ext_block[4] = static_cast<uint8_t>(dataqualifier);
      ext_block[5] = static_cast<uint8_t>(dataqualifier >> 8);
      ext_block[6] = static_cast<uint8_t>(dataqualifier >> 16);
      ext_block[7] = static_cast<uint8_t>(dataqualifier >> 24);
      ext_block[8] = static_cast<uint8_t>(board_id);
      std::memcpy(ext_block.data() + 9u, event.payload.data(), event.payload.size());
      ev->AddBlock(static_cast<uint32_t>(ev->GetNumBlock()), ext_block);
      total_payload_bytes += ext_block.size();

      queue.pop_front();
    }

    int64_t ts_now = hidra::utils::getTimens();
    if (m_stamp_last_sent_ns > 0) {
      HIDRA_DEBUG("Trig {}, time elapsed since last sent: {} ns", trigger_n, ts_now - m_stamp_last_sent_ns);
    }
    ev->SetTag("nativeTimestampBegin", std::to_string(ev->GetTimestampBegin() - m_start_boards_call_ts_ns));
    ev->SetTag("detectorDataSize", std::to_string(total_payload_bytes));
    ev->SetTag("endianness", m_machine_endianness); // TODO review this tag
    AddMonitorStatusTags(*ev);
    SendEvent(std::move(ev));
    m_stamp_last_sent_ns = ts_now;
    HIDRA_DEBUG("{} producer sent event for trg {}, time since last status poll {}", GetName(), trigger_n, std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_last_status_log).count());
    if (std::chrono::steady_clock::now() - m_last_status_log > 1000ms) {
      m_last_status_log = std::chrono::steady_clock::now();
      HIDRA_INFO("{} producer sent event for trg {}. Total events sent: {}", GetName(), trigger_n, m_evt_f);
      SendStatus(); //TODO move in dedicated logging struct
    }
    ++m_evt_f;
  }

  void FlushAlignedEvents() {
    while (true) {
      if (m_board_ids.empty()) {
        return;
      }

      if (!HasAnyQueuedEvents()) {
        ResetAlignmentWaitState();
        return;
      }

      uint64_t trigger_n = 0;
      if (m_alignment_wait_active) {
        trigger_n = m_alignment_wait_trigger;
      } else {
        trigger_n = SelectAlignmentTrigger();
        if (trigger_n == std::numeric_limits<uint64_t>::max()) {
          return;
        }
        m_alignment_wait_trigger = trigger_n;
        m_alignment_wait_active = true;
      }

      const auto [matched_boards, any_board_ahead] = InspectQueuesForTrigger(trigger_n);
      HIDRA_DEBUG("Inspected board for trigger {}: found {}/{}", trigger_n, matched_boards.size(), m_board_ids.size());
      const bool all_matched = matched_boards.size() == m_board_ids.size();
      const auto waited_time = std::chrono::steady_clock::now() - m_alignment_wait_start;
      const auto max_wait = any_board_ahead ? kAlignmentWaitAheadTimeoutUs : kAlignmentWaitIdleTimeoutUs;
      const bool expired = waited_time >= max_wait;

      if (!all_matched && expired) {
        HIDRA_WARN(std::string(GetName()) + " alignment timeout waiting for trigger " +
                   std::to_string(trigger_n) +
                   ", proceeding with available boards only");
        ++m_evt_incomplete;
        SetStatusTag("PartialEvts", std::to_string(m_evt_incomplete));
      }

      if (all_matched or expired) {
        ResetAlignmentWaitState();
        EmitAlignedEvent(trigger_n, matched_boards);
      } else {
        break;
      }
    }
  }

  FERSConfiguration m_config;
  std::unique_ptr<FERSBoardManager> m_board_manager;
  std::map<int, std::deque<FERSEvent>> m_event_queues;
  std::map<int, BoardMonitorStatus> m_monitor_status;
  std::vector<int> m_board_ids;
  std::vector<uint8_t> m_emit_scratch_buffer;
  std::string m_config_file;
  int m_run_number = 0;
  int m_readout_mode = 0;
  int m_configure_mode = CFG_HARD;
  int m_start_mode = STARTRUN_ASYNC;
  uint64_t m_evt_f;
  uint64_t m_max_total_events;
  uint64_t m_evt_incomplete;
  int m_poll_sleep_us = 1000;
  int m_status_poll_interval_s = 0;
  bool m_attach_status_tags = true;
  bool m_send_timestamp = true;
  bool m_exit_of_run = false;
  std::chrono::steady_clock::time_point m_next_status_poll = std::chrono::steady_clock::time_point::min();
  std::chrono::steady_clock::time_point m_last_event_read = std::chrono::steady_clock::time_point::min();
  std::chrono::steady_clock::time_point m_last_status_log = std::chrono::steady_clock::time_point::min();
  std::chrono::steady_clock::time_point m_alignment_wait_start  = std::chrono::steady_clock::time_point::min();
  uint64_t m_stamp_last_sent_ns = 0;
  uint64_t m_start_boards_call_ts_ns = 0;
  uint64_t m_alignment_wait_trigger = 0;
  bool m_alignment_wait_active = false;
  std::string m_machine_endianness = hidra::utils::is_little_endian() ? "LE" : "BE";
};

namespace {
auto dummy0 = eudaq::Factory<eudaq::Producer>::Register<HidraFERS2Producer, const std::string&, const std::string&>(
    HidraFERS2Producer::m_id_factory_fers2);
auto dummy1 = eudaq::Factory<eudaq::Producer>::Register<HidraFERS2Producer, const std::string&, const std::string&>(
    HidraFERS2Producer::m_id_factory_maxicc);
}
