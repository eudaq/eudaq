#include "TimeAlignmentCalibrator.hh"
#include "TransportBase.hh"
#include "HidraUtils.hh"
#include "HidraMergedBinaryWriter.hh"
#include "HidraRootEventWriter.hh"
#include <eudaq/DataCollector.hh>
#include <eudaq/Factory.hh>

#include <cstdint>
#include <memory>
#include <map>
#include <string>
#include <vector>


class HidraDataCollector : public eudaq::DataCollector {
  
  // --- Per-source event container ---
  struct SourceEvent {
    std::string ConnectionName;
    eudaq::EventSP event;
    uint64_t timestamp;
  };

  // --- Buffer for a given trigger ---
  struct PendingTrigger {
    uint64_t trigger_number;
    std::map<int, SourceEvent> events_by_source; // access through detID: events_by_source[x]
    uint64_t first_seen_ns;
    std::uint32_t flags = hidra::utils::HidraEventFlags::None;
  };

public:
  HidraDataCollector(const std::string& name, const std::string& runcontrol)
      : eudaq::DataCollector(name, runcontrol) {}

  ~HidraDataCollector() override = default;

  static const uint32_t m_id_factory = eudaq::cstr2hash("HidraDataCollector");

  void DoInitialise() override;
  void DoConfigure() override;
  void DoStartRun() override;
  void DoStopRun() override;
  void DoReset() override;
  void DoTerminate() override;
  void DoConnect(eudaq::ConnectionSPC id) override;
  void DoDisconnect(eudaq::ConnectionSPC id) override;
  void DoReceive(eudaq::ConnectionSPC id, eudaq::EventSP ev) override;

private:
  bool IsExpectedSource(std::string& source);
  bool IsExpectedSource(int& detID) { return m_is_source_enabled[detID]; }
  bool IsComplete(PendingTrigger& pending);

  eudaq::EventSP BuildFullEvent(PendingTrigger& pending);
  bool EnqueueMergedEvent(const eudaq::EventSP& event);
  std::string MakeOutputFile(const std::string& extension, const std::string& directory) const;
  void FlushOldIncompleteEvents();
  void CheckMaxEvents();
  void UpdateStatusTags();

  int getDetectorProgressiveIndex(int detID);


  const int MAX_SOURCES = 8;

  bool m_is_replay_mode;
  // when in replay mode, it uses timestamp of greatest trigger instead of
  // runtime timestamp to evaluate whether incomplete events shall be built
  uint64_t m_event_count = 0;
  uint64_t m_max_events;
  int m_n_complete_events = 0;
  int m_n_incomplete_events = 0;
  bool m_stop_sent = false;
  bool m_running = false;

  int m_Nevents_time_calib = 100;
  uint8_t m_log_level = 0;
  hidra::timealignment::TriggerAlignmentConfig m_calib_timing_cfg; 
  std::vector<std::map<long long, long long>> m_calib_timing_events; // vector of maps <triggerN, timestamp> 
  std::vector<long long> m_calib_timing_mean{};
  std::vector<long long> m_calib_timing_spread{};
  std::vector<long long> m_calib_timing_trg_offsets{};
  bool m_calib_timing_enabled = false; // this is static until re-configuration of EuDAQ
  bool m_calib_timing_needed = false; // this becomes true/false when calibration along the run is needed. If calib is enabled, is set true at the beginning of each run
  bool m_calib_timing_validated = false;
  long long m_maxTimeSpread = -1;
  std::string m_calib_trg_offset_report = "";
  std::vector<bool> m_is_source_enabled = std::vector<bool>(MAX_SOURCES, false);
  std::map<std::string, int> m_expected_sources_map;
  std::map<uint64_t, PendingTrigger> m_pending_events;
  uint64_t m_sync_timeout_us = 1000000;
  uint64_t m_tstamp_window_ns = 50000;

  uint64_t m_max_trigger_built = 0;
  uint64_t m_max_trigger_seen = 0;

  std::map<int, std::string> m_vme_geo_map;

  bool m_single_producer_mode = false; // when no sources are specified, only the first one is accepted and DetectorID 0 is assigned.
  bool m_write_binary_output = true;
  bool m_write_root_output = false;
  uint64_t m_writer_flush_interval_ms = 50;

  std::string m_filePattern;
  // Optional output directories: when non-empty, the merged file name built
  // from m_filePattern is placed inside this directory (created if missing),
  // so the .raw and .root streams can be written to different locations. When
  // empty, the pattern is used as-is (its embedded path, if any, still applies).
  std::string m_binary_output_dir;
  std::string m_root_output_dir;
  std::string m_fwType;
  std::string m_xdc_config_json;
  std::string m_binary_output_file;
  std::string m_root_output_file;

  std::unique_ptr<hidra::HidraMergedBinaryWriter> m_binary_writer;
  std::unique_ptr<hidra::HidraRootEventWriter> m_root_writer;
  std::chrono::steady_clock::time_point m_last_status_log = std::chrono::steady_clock::time_point::min();
  std::chrono::steady_clock::time_point m_last_receive_log = std::chrono::steady_clock::time_point::min();

};

namespace {
auto dummy0 =
    eudaq::Factory<eudaq::DataCollector>::Register<HidraDataCollector, const std::string&, const std::string&>(
        HidraDataCollector::m_id_factory);

}
