#include "Status.hh"
#include "eudaq/RunControl.hh"

#include "HidraUtils.hh"

#include <chrono>
#include <fstream>
#include <iomanip>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct ModuleStatus {
  std::string state;
  std::map<std::string, std::string> tags;
  std::chrono::system_clock::time_point last_update;
};

class HidraRunControl : public eudaq::RunControl {
public:
  HidraRunControl(const std::string& listenaddress);
  void Configure() override;
  void StartRun() override;
  void StopRun() override;
  void Exec() override;

  static const uint32_t m_id_factory = eudaq::cstr2hash("HidraRunControl");

  void DoStatus(eudaq::ConnectionSPC con, eudaq::StatusSPC st) override;

private:
  void DumpStatusJsonl();

  bool m_flag_running;
  std::chrono::steady_clock::time_point m_tp_start_run;
  std::chrono::steady_clock::time_point m_tp_stop_run;

  std::map<std::string, ModuleStatus> module_status;
  std::map<std::string, std::string> last_printed_state;

  std::mutex mtx;

  std::ofstream m_status_dump;
  std::chrono::steady_clock::time_point m_last_status_dump;
  uint32_t m_last_run_n = 0;
};

namespace {
auto dummy0 =
    eudaq::Factory<eudaq::RunControl>::Register<HidraRunControl, const std::string&>(
        HidraRunControl::m_id_factory);
}

HidraRunControl::HidraRunControl(const std::string& listenaddress)
    : RunControl(listenaddress) {
  m_flag_running = false;
}

void HidraRunControl::StartRun() {
  RunControl::StartRun();

  m_tp_start_run = std::chrono::steady_clock::now();
  m_last_status_dump = std::chrono::steady_clock::now();
  m_flag_running = true;

  if (m_status_dump.is_open()) {
    m_status_dump.close();
  }

  std::ostringstream filename;
  filename << "monitoring/run" << std::setw(6) << std::setfill('0') << GetRunN()
           << "_status.jsonl";

  m_status_dump.open(filename.str(), std::ios::out | std::ios::app);

  if (!m_status_dump) {
    EUDAQ_ERROR("Cannot open status dump file: " + filename.str());
  } else {
    EUDAQ_INFO("Writing RunControl status dump to: " + filename.str());
  }
}

void HidraRunControl::StopRun() {

  m_last_run_n = GetRunN();

  RunControl::StopRun();
  m_tp_stop_run = std::chrono::steady_clock::now();
  m_flag_running = false;
  DumpStatusJsonl();




  if (m_status_dump.is_open()) {
    m_status_dump.flush();
    m_status_dump.close();
  }
}

void HidraRunControl::Configure() {
  RunControl::Configure();
}

void HidraRunControl::Exec() {
  StartRunControl();

  while (IsActiveRunControl()) {
    bool collector_ready = false;

    {
      std::lock_guard<std::mutex> lock(mtx);

      for (const auto& p : module_status) {
        const std::string& name = p.first;
        const std::string& state = p.second.state;

        if (last_printed_state[name] != state) {
          EUDAQ_INFO("[DEVICE]: " + name + " [STATUS]: " + state);
          last_printed_state[name] = state;
        }

        if (m_flag_running) {
          if (name == "HidraDataCollector" && state == "STOP_REQUEST") {
            collector_ready = true;
          }
        }
      }
    } // --- Mutex goes out of scope and is released ---

    auto now = std::chrono::steady_clock::now();

    if (m_flag_running &&
        now - m_last_status_dump >= std::chrono::seconds(2)) {
      DumpStatusJsonl();
      m_last_status_dump = now;
    }

    if (collector_ready) {
      EUDAQ_INFO("All devices ready, stopping run");
      StopRun();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  if (m_status_dump.is_open()) {
    m_status_dump.flush();
    m_status_dump.close();
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

void HidraRunControl::DoStatus(eudaq::ConnectionSPC con,
                               eudaq::StatusSPC st) {
  std::lock_guard<std::mutex> lock(mtx);

  auto& status = module_status[con->GetName()];

  status.state = st->GetMessage();
  status.last_update = std::chrono::system_clock::now();

  status.tags.clear();

  status.tags = st->GetTags(); // map<string,string>
}

void HidraRunControl::DumpStatusJsonl() {
  if (!m_status_dump.is_open()) {
    return;
  }

  std::map<std::string, ModuleStatus> snapshot;

  // keep mutel only while for copy
  {
    std::lock_guard<std::mutex> lock(mtx);
    snapshot = module_status;
  }

  auto now = std::chrono::system_clock::now();
  auto unix_ns =
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          now.time_since_epoch())
          .count();

  json j;
  j["time_unix_ns"] = unix_ns;
  j["run"] = m_flag_running ? GetRunN() : m_last_run_n;

  for (const auto& p : snapshot) {
    const auto& name = p.first;
    const auto& status = p.second;

    auto last_update_ns =
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            status.last_update.time_since_epoch())
            .count();

    
    j["devices"][name]["state"] = status.state;
    j["devices"][name]["last_update_unix_ns"] = last_update_ns;
    j["devices"][name]["tags"] = status.tags;
  }

  m_status_dump << j.dump() << "\n";
  m_status_dump.flush();
}
