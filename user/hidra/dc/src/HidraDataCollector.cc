#include "HidraDataCollector.hh"
#include "HidraRootPayloadDecoders.hh"
#include "HidraUtils.hh"
#include "TimeAlignmentCalibrator.hh"
#include <cmath>
#include <eudaq/Event.hh>
#include <eudaq/Exception.hh>
#include <eudaq/FileNamer.hh>
#include <eudaq/Logger.hh>

#include <chrono>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <map>
#include <numeric>
#include <string>
#include <sys/types.h>
#include <vector>

using namespace std::chrono_literals;
// user custom

bool HidraDataCollector::IsExpectedSource(std::string& source) {
  auto it = m_expected_sources_map.find(source);
  if (it == m_expected_sources_map.end()) {
    return false;
  } else {
    return m_is_source_enabled[m_expected_sources_map.at(source)];
  }
}

bool HidraDataCollector::IsComplete(PendingTrigger& pending) {
  for (const auto& s : m_expected_sources_map) {
    if (pending.events_by_source.count(s.second) == 0) {
      return false;
    }
  }
  return true;
}

std::string HidraDataCollector::MakeOutputFile(const std::string& extension, const std::string& directory) const {
  std::time_t time_now = std::time(nullptr);
  char time_buff[13];
  time_buff[12] = 0;
  std::strftime(time_buff, sizeof(time_buff), "%y%m%d%H%M%S", std::localtime(&time_now));

  std::string name =
      eudaq::FileNamer(m_filePattern).Set('X', extension).Set('R', GetRunNumber()).Set('D', std::string(time_buff));

  // No directory override: keep the pattern as-is (it may carry its own path).
  if (directory.empty()) {
    return name;
  }

  // Place the file inside the requested directory, creating it if needed so the
  // run doesn't fail just because the destination doesn't exist yet.
  std::filesystem::path dir(directory);
  std::error_code ec;
  std::filesystem::create_directories(dir, ec);
  if (ec) {
    HIDRA_ERROR("Could not create output directory {}: {}. Falling back to pattern path.", directory, ec.message());
    return name;
  }
  return (dir / name).string();
}

bool HidraDataCollector::EnqueueMergedEvent(const eudaq::EventSP& event) {
  if (!event) {
    return false;
  }

  if (m_calib_timing_needed) {
    HIDRA_DEBUG("Merged event for trigger {} will not be enqueued for writing because timing calibration is not yet validated",
                event->GetTriggerN());
    if (m_event_count < 5){
      HIDRA_INFO("Merged evt for trig {} not enqueued because timing calib not yet validated. This will be true till evt {}...",
                event->GetTriggerN(), m_Nevents_time_calib);
    }
    return false;
  }

  bool accepted = false;

  if (m_write_binary_output && m_binary_writer) {
    accepted = m_binary_writer->EnqueueEvent(event) || accepted;
  }

  if (m_write_root_output && m_root_writer) {
    accepted = m_root_writer->EnqueueEvent(event) || accepted;
  }

  if (!accepted) {
    HIDRA_ERROR("Merged event for trigger {} was not accepted by any active writer", event->GetTriggerN());
  }

  return accepted;
}

eudaq::EventSP HidraDataCollector::BuildFullEvent(PendingTrigger& pending) {

  // just a skeleton. Full event must be built here
  auto it = pending.events_by_source.begin();

  if (it == pending.events_by_source.end()) {
    EUDAQ_ERROR("Cannot build event out of NO PENDING triggers");
    return nullptr;
  }

  // event with no blocks. Header (tag) + subevents
  auto fullEvt = eudaq::Event::MakeUnique("MergedEvent");
  fullEvt->SetRunN(GetRunNumber());
  fullEvt->SetTriggerN(pending.trigger_number);
  fullEvt->SetTag("N_SOURCES", std::to_string(pending.events_by_source.size()));
  
  uint8_t detectormask = 0x0;
  uint32_t detectorFlagsOnly = 0x0;

  for (const auto& is : m_expected_sources_map) {
    // will be overwritten if source is in the event
    fullEvt->SetTag(is.first + "_id",
                    is.second); // "XDCProducer_id = <detID>"
    fullEvt->SetTag(std::to_string(is.second) + "_size",
                    0); // "<detID>_size = 0"
  }

  uint8_t trigMask = 0xFF;
  uint32_t spillNum = std::numeric_limits<uint32_t>::max();
  uint64_t ts_begin = std::numeric_limits<uint64_t>::max();
  uint64_t ts_end = 0;

  for (; it != pending.events_by_source.end(); ++it) {
    fullEvt->SetTag(std::to_string(it->first) + "_size",
                    it->second.event->GetTag("detectorDataSize")); // "<detID>_size = size"
    it->second.event->SetTag("Producer", it->second.ConnectionName);
    it->second.event->SetTag("detID", it->first);
    ts_begin = std::min(ts_begin, it->second.event->GetTimestampBegin());
    ts_end = std::max(ts_end, it->second.event->GetTimestampBegin());
    uint8_t sourceTrigMask = hidra::utils::getTagOr<std::uint8_t>(*it->second.event, "triggerMask", trigMask, false);
    uint32_t sourceSpillNum = hidra::utils::getTagOr<std::uint32_t>(*it->second.event, "spillNumber", spillNum, false);
    if (it != pending.events_by_source.begin() && trigMask != sourceTrigMask) {
      HIDRA_ERROR("Evt {}: Inconsistent trigger mask for detID {}: current subevent has {}, but previous have {}. Using {}",
                  pending.trigger_number,
                  it->first,
                  sourceTrigMask,
                  trigMask,
                  sourceTrigMask);
    }
    if (it != pending.events_by_source.begin() && spillNum != sourceSpillNum) {
      HIDRA_ERROR("Evt {}: Inconsistent spill number for detID {}: current subevent has {}, but previous have {}. Using {}",
                  pending.trigger_number,
                  it->first,
                  sourceSpillNum,
                  spillNum,
                  sourceSpillNum);
    }
    trigMask = sourceTrigMask;
    spillNum = sourceSpillNum;
    detectormask |= (1 << (it->first));
    detectorFlagsOnly |= (hidra::utils::GetEventFlagMask(*it->second.event));
    fullEvt->AddSubEvent(std::move(it->second.event));
  }

  fullEvt->SetTimestamp(ts_begin, std::max(ts_begin, ts_end)); // do not allow ts_end < ts_bing
  fullEvt->SetTag("triggerMask", std::to_string(trigMask));
  fullEvt->SetTag("spillNumber", std::to_string(spillNum));
  fullEvt->SetTag("detectorMask", std::to_string(detectormask));

  HIDRA_DEBUG("Merged event built: {}", hidra::utils::GetEventInfo(fullEvt.get(), 2));
  if (std::chrono::steady_clock::now() - m_last_status_log > 1000ms) {
    m_last_status_log = std::chrono::steady_clock::now();
    HIDRA_INFO("Merged event built: {}", hidra::utils::GetEventInfo(fullEvt.get(), 2));
    HIDRA_INFO("So far: {} events ({} complete, {} incomplete), pending triggers: {}",
               m_event_count,
               m_n_complete_events,
               m_n_incomplete_events,
               m_pending_events.size());
  }

  uint32_t mergedEventFlags = pending.flags;
  mergedEventFlags |= detectorFlagsOnly;
  hidra::utils::SetEventFlagMask(*fullEvt, mergedEventFlags);

  m_max_trigger_built = std::max(m_max_trigger_built, pending.trigger_number);

  return fullEvt;
}

void HidraDataCollector::FlushOldIncompleteEvents() {

  if (m_single_producer_mode) {
    return;
  }

  for (auto it = m_pending_events.begin(); it != m_pending_events.end();) {

    uint64_t age_ns = hidra::utils::getTimens() - it->second.first_seen_ns;

    bool is_old_time = age_ns > m_sync_timeout_us*1000;
    bool is_old_trg = (m_max_trigger_seen - it->second.trigger_number) > 64000; // we have to flush to free the buffer for new "trigger & 0xFFFF" 

    if (!is_old_time && !is_old_trg) {
      ++it;
      continue;
    }

    if (is_old_trg && !is_old_time){
      HIDRA_WARN("Trg {}: event building timeout not exceeded but there are more than 64000 trigger numbers in the buffer: building", it->second.trigger_number);
    }


    /*HIDRA_WARN(
        "Timeout waiting for complete event for trigger {}: {} > {} ns", trigger, age_ns, m_sync_timeout_us * 1000);
*/
    // if one wants to discard:
    // it = m_pending_events.erase(it);

    auto mergedEvt = BuildFullEvent(it->second);

    if (!mergedEvt) {
      HIDRA_WARN("Failed to build incomplete event for trigger {}", it->second.trigger_number);
      it = m_pending_events.erase(it);
      return;
    }

    mergedEvt->SetTag("SYNC_STATUS", "INCOMPLETE");
    m_n_incomplete_events++;
    ++m_event_count;
    UpdateStatusTags();
    EnqueueMergedEvent(mergedEvt);
    WriteEvent(mergedEvt);

    it = m_pending_events.erase(it);
  }
}

void HidraDataCollector::CheckMaxEvents() {

  if (!m_stop_sent && m_max_events != 0 && m_event_count >= m_max_events) {
    m_stop_sent = true;
    HIDRA_INFO("Max events reached. Sending STOP request");
    SetStatus(eudaq::Status::STATE_RUNNING, "STOP_REQUEST");
    SendStatus();
  }
}

void HidraDataCollector::UpdateStatusTags() {
  SetStatusTag("EventN", std::to_string(m_event_count));
  SetStatusTag("Pending", std::to_string(m_pending_events.size()));
  SetStatusTag("Completes", std::to_string(m_n_complete_events));
  SetStatusTag("Incompletes", std::to_string(m_n_incomplete_events));
  SetStatusTag("EventsOnDisk", std::to_string(m_binary_writer ? m_binary_writer->GetWrittenEventCount() : 0));
  if (m_calib_timing_enabled) {
    SetStatusTag("kBOnDisk", std::to_string(m_binary_writer ? m_binary_writer->GetWrittenByteCount() / 1000 : 0));
    SetStatusTag("CalibTimingStatus", m_calib_timing_needed ? "Waiting" : (m_calib_timing_validated ? "Ok" : "Failed"));
    SetStatusTag("CalibMaxTimeSpread", m_calib_timing_needed ? "Waiting" : std::to_string(m_maxTimeSpread) + " ns");
    SetStatusTag("CalibOffsets", m_calib_timing_needed ? "Waiting" : m_calib_trg_offset_report);
  }
  SendStatus();
}

int HidraDataCollector::getDetectorProgressiveIndex(int detID) {
  if (detID < 0 || detID >= MAX_SOURCES) {
    HIDRA_THROW("Invalid detID {}. Must be between 0 and {}", detID, MAX_SOURCES - 1);
  }
  int index = 0;

  if (!m_is_source_enabled[detID]) {
    HIDRA_THROW("detID {} is not an enabled source", detID);
  }
  for (int currentDetID = 0; currentDetID < detID; ++currentDetID) {
    index += m_is_source_enabled[currentDetID] ? 1 : 0;
  }

  return index;
}

//////////////////////////////////////////////

void HidraDataCollector::DoInitialise() {
  auto ini = GetInitConfiguration();
  if (!ini) {
    HIDRA_WARN("HidraDataCollector: missing init configuration");
  }

  m_event_count = 0;
  m_pending_events.clear();
  m_calib_timing_events.clear();
  m_calib_timing_validated = false;
  m_calib_timing_needed = false;

  HIDRA_INFO("HidraDataCollector initialized");
}

void HidraDataCollector::DoConfigure() {
  auto conf = GetConfiguration();
  m_log_level = (int)(conf->Get("HIDRA_MUTE_DEBUG", 1));
  EUDAQ_LOG_LEVEL(m_log_level);

  m_event_count = 0;
  m_stop_sent = false;
  m_pending_events.clear();

  m_calib_timing_enabled = conf->Get("TIME_ALIGNMENT_ENABLE", 0); // this is static until re-configuration of EuDAQ
  m_calib_timing_events.clear();
  m_calib_timing_validated = false;
  m_calib_timing_needed = m_calib_timing_enabled; // this becomes true/false when calibration along the run is needed

  m_expected_sources_map.clear();
  std::fill(m_is_source_enabled.begin(), m_is_source_enabled.end(), false);
  m_single_producer_mode = false;

  if (!conf) {
    HIDRA_ERROR("HidraDataCollector: missing run configuration");
  }

  m_filePattern = conf->Get("EUDAQ_FW_PATTERN", "/home/eudaq/daq/TB2026_HidraData/HidraData/run$3R_$12D$X");
  // Optional separate output directories for the binary (.raw) and ROOT (.root)
  // streams. Empty -> use the pattern path as before (both files together).
  m_binary_output_dir = conf->Get("BINARY_OUTPUT_DIR", "");
  m_root_output_dir = conf->Get("ROOT_OUTPUT_DIR", "");
  m_fwType = conf->Get("EUDAQ_FW", "HidraNullFileWriter");
  m_write_binary_output = conf->Get("WRITE_BINARY_OUTPUT", 1);
  m_write_root_output = conf->Get("WRITE_ROOT_OUTPUT", 1);
  m_writer_flush_interval_ms = conf->Get("WRITER_FLUSH_INTERVAL_MS", 50);

  if (!m_write_binary_output && !m_write_root_output) {
    HIDRA_WARN("Both WRITE_BINARY_OUTPUT and WRITE_ROOT_OUTPUT are disabled. Collector will run without file output.");
  }

  m_max_events = conf->Get("MAX_EVENTS", 0);
  m_sync_timeout_us = conf->Get("SYNC_TIMEOUT_US", 15000000);
  m_tstamp_window_ns = conf->Get("TIME_ALIGNMENT_WINDOW_NS", 300000);
  m_Nevents_time_calib = conf->Get("TIME_ALIGNMENT_NEVENTS_CALIB", 100);

  std::string configsources = conf->Get("EXPECTED_SOURCES", "");

  if (configsources == "") {
    m_expected_sources_map = std::map<std::string, int>{};
    std::fill(m_is_source_enabled.begin(), m_is_source_enabled.end(), false);
    m_is_source_enabled[0] = true;
    m_single_producer_mode = true;
  } else {
    std::map<std::string, std::string> tempprod = hidra::utils::parseConfigMap(configsources);
    for (const auto& kv : tempprod) {
      const std::string& idetid = kv.first;
      const std::string& iproducer = kv.second;
      int detID = std::stoi(idetid);
      if (detID >= 0 && detID < MAX_SOURCES) {
        HIDRA_INFO("Detector ID {} assigned to producer {}", detID, iproducer);
        m_expected_sources_map[iproducer] = detID;
        m_is_source_enabled[detID] = true;
      } else {
        HIDRA_THROW("Detector ID {} cannot be assigned. ID must be between "
                    "0 and {}",
                    detID,
                    MAX_SOURCES);
      }
    }
  }

  m_vme_geo_map.clear();
  std::string vmecrateconfig = conf->Get("VME_CRATE_1", "2:V792,4:V792,6:V792,8:V792,10:V792,12:V862,14:V775N");
  if (vmecrateconfig != "") {
    std::map<std::string, std::string> tempvme = hidra::utils::parseConfigMap(vmecrateconfig);
    for (const auto& kv : tempvme) {
      const std::string& geo = kv.first;
      const std::string& modname = kv.second;
      int geoaddr = std::stoi(geo);
      m_vme_geo_map[geoaddr] = modname;
      HIDRA_INFO("VME module at geo address {} is {}", geoaddr, modname);
    }
  }

  if (m_expected_sources_map.empty()) {
    HIDRA_WARN("No EXPECTED_SOURCES configured. Collector will accept only the first source received, assigning it "
               "DetectorID 0");
  }

  m_calib_timing_cfg =
      hidra::timealignment::TriggerAlignmentConfig{.maxAbsTrgOffset = m_Nevents_time_calib,
                                                   .minMatches = m_Nevents_time_calib / 2,
                                                   .maxDeltaTRange = static_cast<long long>(m_tstamp_window_ns),
                                                   .stopAtFirstValid = false};

  HIDRA_INFO("HidraDataCollector configured");
}

void HidraDataCollector::DoStartRun() {
  m_event_count = 0;
  m_stop_sent = false;
  m_running = true;
  m_n_complete_events = 0;
  m_n_incomplete_events = 0;
  m_max_trigger_seen = 0;
  m_max_trigger_built = 0;
  m_pending_events.clear();
  m_calib_timing_events.clear();
  m_calib_timing_events.assign(
      m_expected_sources_map.size(),
      std::map<long long, long long>{}); // initialize the vector of maps with the number of expected sources. Each map
                                         // will be filled with <triggerN, timestamp> for the calibration events of the
                                         // corresponding source.
  m_calib_timing_validated = false;
  m_calib_timing_needed = m_calib_timing_enabled;
  m_last_status_log = std::chrono::steady_clock::now();
  m_binary_writer.reset();
  m_root_writer.reset();

  if (m_write_binary_output) {
    m_binary_output_file = MakeOutputFile(".raw", m_binary_output_dir);
    HIDRA_INFO("Output_file: {}", m_binary_output_file);
    auto binary_writer =
        std::make_unique<hidra::HidraMergedBinaryWriter>(m_binary_output_file, m_writer_flush_interval_ms);

    binary_writer->Start();
    if (!binary_writer->IsActive()) {
      HIDRA_ERROR("Binary writer could not start: {}", binary_writer->GetLastError());
      m_write_binary_output = false;
    } else {
      m_binary_writer = std::move(binary_writer);
      HIDRA_INFO("HidraDataCollector binary output file {}", m_binary_output_file);
    }
  }

  if (m_write_root_output) {
    m_root_output_file = MakeOutputFile(".root", m_root_output_dir);
    auto root_writer = std::make_unique<hidra::HidraRootEventWriter>(
        m_root_output_file, m_writer_flush_interval_ms, 32, m_vme_geo_map, m_log_level);
    root_writer->Start();
    if (!root_writer->IsActive()) {
      HIDRA_ERROR("ROOT writer could not start: {}", root_writer->GetLastError());
      m_write_root_output = false;
    } else {
      m_root_writer = std::move(root_writer);
      HIDRA_INFO("HidraDataCollector ROOT output file {}", m_root_output_file);
    }
  }

  HIDRA_INFO("HidraDataCollector start run {}", GetRunNumber());
  UpdateStatusTags();
}

void HidraDataCollector::DoStopRun() {
  m_stop_sent = true;
  m_running = false;
  // TOOD: what do we want to happen to incomplete events when we click stop? If discard, keep the line commented.
  FlushOldIncompleteEvents();
  m_pending_events.clear();
  m_calib_timing_events.clear();
  m_calib_timing_validated = false;
  m_calib_timing_needed = m_calib_timing_enabled;

  if (m_binary_writer) {
    m_binary_writer->Stop();
  }
  if (m_root_writer) {
    m_root_writer->Stop();
  }

  HIDRA_INFO("HidraDataCollector stop run {}", GetRunNumber());
}

void HidraDataCollector::DoReset() {
  m_event_count = 0;
  m_stop_sent = false;
  m_running = false;
  m_max_events = 0;
  m_pending_events.clear();
  m_calib_timing_events.clear();
  m_calib_timing_validated = false;
  m_calib_timing_needed = false;
  m_expected_sources_map.clear();
  std::fill(m_is_source_enabled.begin(), m_is_source_enabled.end(), false);

  if (m_binary_writer) {
    m_binary_writer->Stop();
  }
  if (m_root_writer) {
    m_root_writer->Stop();
  }
  HIDRA_INFO("HidraDataCollector reset");
}

void HidraDataCollector::DoTerminate() {
  m_running = false;

  if (m_binary_writer) {
    m_binary_writer->Stop();
  }
  if (m_root_writer) {
    m_root_writer->Stop();
  }
  HIDRA_INFO("HidraDataCollector terminate");
}

void HidraDataCollector::DoConnect(eudaq::ConnectionSPC id) {
  HIDRA_INFO("Connected: {} / {} ", id->GetType(), id->GetName());
  eudaq::DataCollector::DoConnect(id);
}

void HidraDataCollector::DoDisconnect(eudaq::ConnectionSPC id) {
  HIDRA_INFO("Disconnected: {} / {} ", id->GetType(), id->GetName());
  eudaq::DataCollector::DoDisconnect(id);
}

void HidraDataCollector::DoReceive(eudaq::ConnectionSPC id, eudaq::EventSP ev) {

  auto t_start = std::chrono::high_resolution_clock::now();

  if (!m_running) {
    return;
  }

  if (!ev) {
    HIDRA_WARN("HidraDataCollector received null event");
    return;
  }

  if (hidra::utils::getTagOr(*ev, "DoNotBuild", std::string("no"), false) == "yes") {
    // use this to skip building: events with this tag can be used for communication
    return;
  }

  auto source = id->GetName();
  auto desc = ev->GetDescription();
  int detectorID = 0;

  if (m_single_producer_mode && m_expected_sources_map.empty() && m_pending_events.empty()) {
    // if here, we are in single producer mode and this is the first event received, so we assign the source to detID 0
    HIDRA_INFO("Single producer mode: assigning source {} to detID 0", source);
    m_expected_sources_map[source] = 0;
    m_is_source_enabled[0] = true;
  }

  if (!IsExpectedSource(source)) {
    HIDRA_ERROR("Event received from unexpected source {}", source);
    return;
  }

  detectorID = m_expected_sources_map[source];

  if (ev->IsBORE()) {
    HIDRA_INFO("Received BORE from {} type= {}", id->GetName(), desc);
    // TODO let's collect info to write a file header
    return;
  }

  if (ev->IsEORE()) {
    HIDRA_INFO("Received EORE from {} type= {}", id->GetName(), desc);
    // TODO let's collect info to write a file trailer
    return;
  }

  // stop when reaching max number of events
  CheckMaxEvents();
  if (m_stop_sent) {
    return;
  }

  FlushOldIncompleteEvents();

  uint64_t trigger_number = ev->GetTriggerN();
  uint64_t timestamp_begin_ns = ev->GetTimestampBegin();
  uint64_t timestamp_end_ns = ev->GetTimestampEnd();
  m_max_trigger_seen = std::max(m_max_trigger_seen, trigger_number);

  /*if (trigger_number <= m_max_trigger_built && detectorID < 3){ // Old triggers can happen for tracker!
    HIDRA_ERROR("Receiving trigger {} from det ID {}, older than the last trigger built {}", trigger_number, detectorID, m_max_trigger_built);
  }*/

  uint64_t timestamp = timestamp_begin_ns; // TODO: decide what to do in case of spread between begin and end (if not
                                           // addressed in the producers)

  // filling map for calibration and timing validation. We do this for all events, even incomplete
  // the verctor is assigned with the number of expected sources at the start of the run
  if (m_calib_timing_needed && m_expected_sources_map.size() > 1 && m_n_complete_events < m_Nevents_time_calib) {
    if (timestamp_end_ns - timestamp_begin_ns < m_tstamp_window_ns) {
      m_calib_timing_events.at(getDetectorProgressiveIndex(detectorID))[(long long)trigger_number] =
          (long long)(timestamp);
      HIDRA_DEBUG("Alignment -- adding entry for detID {}, index {}, trigger {}, timestamp {}",
                  detectorID,
                  getDetectorProgressiveIndex(detectorID),
                  (long long)trigger_number,
                  (long long)timestamp);
    } else {
      HIDRA_DEBUG("Alignment --  discarding entry for detID {}, index {}, trigger {}, timestamp {}",
                  detectorID,
                  getDetectorProgressiveIndex(detectorID),
                  (long long)trigger_number,
                  (long long)timestamp);
    }
  }

  auto& pending = m_pending_events[trigger_number & 0xFFFF]; // creates a default if trigger_number
                                                    // does not exist

  ////////////// BUFFERING PENDING //////////

  // assign triggerN and time ....
  if (pending.events_by_source.empty()) {
    pending.trigger_number = trigger_number;
    pending.first_seen_ns = hidra::utils::getTimens();
  }
  else{
    pending.trigger_number = std::max(pending.trigger_number, trigger_number); // If XDC > 2^16 there is no risk to assign the tracker trigger to the full event
  }

  // ... check if source duplicates ...
  if (pending.events_by_source.count(detectorID) != 0) {
    HIDRA_ERROR("Duplicate event from source/detID {}/{} for trigger {}. "
                "REPLACING PREVIOUS ONE",
                source,
                detectorID,
                trigger_number);
    pending.flags |= hidra::utils::HidraEventFlags::DuplicatedTrigger;
    // TODO : this is severe.. handle it!
  }

  // .. if not, assign also the SourceEvent
  pending.events_by_source[detectorID] = SourceEvent{id->GetName(), std::move(ev), timestamp};

  HIDRA_DEBUG("Buffered: source/detID {}/{} trig {}  n_source {}",
              source,
              detectorID,
              trigger_number,
              pending.events_by_source.size());

  //////////////////////////////////////////////

  if (!IsComplete(pending)) { // wait for next received, if this is not complete yet
    auto t_end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t_end - t_start).count();
    HIDRA_DEBUG("DoReceive (w/o complete building) took {} us", duration);
    return;
  }

  if (m_calib_timing_needed && m_expected_sources_map.size() > 1 && m_n_complete_events >= m_Nevents_time_calib) {
    auto t_calib_start = std::chrono::high_resolution_clock::now();
    auto results = hidra::timealignment::alignTriggerMapsToFirstReference(m_calib_timing_events, m_calib_timing_cfg);
    m_calib_timing_mean = hidra::timealignment::meanDeltaTsFromAlignments(results);
    m_calib_timing_spread = hidra::timealignment::deltaTRangesFromAlignments(results);
    m_calib_timing_trg_offsets = hidra::timealignment::triggerOffsetsFromAlignments(results);
    m_calib_timing_validated = hidra::timealignment::validateTriggerAlignments(results);

    m_calib_timing_needed = false; // TODO: we may want to recalibrate timing during the run, if we see many incomplete
                                   // events, or after a certain time has passed,

    int maxIndex = -1;
    m_maxTimeSpread = -1;
    std::vector<long long> calib_timing_spread_abs;
    for (const auto& s : m_calib_timing_spread) {
      calib_timing_spread_abs.push_back(std::llabs(s));
    }
    auto maxIt = std::max_element(calib_timing_spread_abs.begin(), calib_timing_spread_abs.end());
    if (maxIt != calib_timing_spread_abs.end()) {
      maxIndex = std::distance(calib_timing_spread_abs.begin(), maxIt);
      m_maxTimeSpread = m_calib_timing_spread[maxIndex];
    }

    auto t_calib_end = std::chrono::high_resolution_clock::now();
    auto calib_duration_us = std::chrono::duration_cast<std::chrono::microseconds>(t_calib_end - t_calib_start).count();

    m_calib_trg_offset_report = "";
    for (const auto& t : m_calib_timing_trg_offsets) {
      m_calib_trg_offset_report += std::to_string(t) + ",";
    }

    if (m_calib_timing_validated) {
      HIDRA_INFO("Timing calibration successful! max deltaT spread is {} ns for det progressive index {}. Calibration "
                 "took {} us. "
                 "Result is {}",
                 m_maxTimeSpread,
                 maxIndex,
                 calib_duration_us,
                 m_calib_trg_offset_report);
    } else {
      HIDRA_ERROR(
          "Timing calibration failed! max deltaT spread is {} ns for det progressive index {}, max allowed is {} ns. "
          "Calibration took {} us. Result is {}",
          m_maxTimeSpread,
          maxIndex,
          m_calib_timing_cfg.maxDeltaTRange,
          calib_duration_us,
          m_calib_trg_offset_report);
    }
  }

  // TODO: if here, the event is complete. Nevertheless we should check also incomplete events.
  if (m_calib_timing_validated && !m_calib_timing_mean.empty()) {
    int refDetectorID = -1;
    for (int detID = 0; detID < MAX_SOURCES; ++detID) {
      if (m_is_source_enabled[detID]) {
        refDetectorID = detID;
        break;
      }
    }

    if (refDetectorID < 0) {
      HIDRA_ERROR("Timestamp validation requested, but no enabled reference detector was found");
      return;
    }

    const long long refTimestamp = static_cast<long long>(pending.events_by_source.at(refDetectorID).timestamp);

    for (const auto& expectedSource : m_expected_sources_map) {
      const int currentDetectorID = expectedSource.second;
      const int currentIndex = getDetectorProgressiveIndex(currentDetectorID);
      const long long deltaT =
          static_cast<long long>(pending.events_by_source.at(currentDetectorID).timestamp) - refTimestamp;
      const long long expectedDeltaT = m_calib_timing_mean[currentIndex];

      if (std::llabs(deltaT - expectedDeltaT) > static_cast<long long>(m_tstamp_window_ns)) {
        HIDRA_ERROR("Timestamp mismatch for trigger {}, detID {}: deltaT to detID {} is {} ns, expected {} ns",
                    trigger_number,
                    currentDetectorID,
                    refDetectorID,
                    deltaT,
                    expectedDeltaT);
        pending.flags |= hidra::utils::HidraEventFlags::TimestampMismatch;
      }
    }
  }

  auto mergedEvt = BuildFullEvent(pending);

  if (!mergedEvt) {
    HIDRA_WARN("Failed to build full event for trigger {}", trigger_number);
    m_pending_events.erase(trigger_number & 0xFFFF);
    return;
  }

  // if arriving here, the event is complete
  mergedEvt->SetTag("SYNC_STATUS", "COMPLETE");
  m_n_complete_events++;

  ++m_event_count;
  UpdateStatusTags();
  EnqueueMergedEvent(mergedEvt);
  WriteEvent(mergedEvt);

  m_pending_events.erase(trigger_number & 0xFFFF);

  CheckMaxEvents();

  auto t_end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t_end - t_start).count();
  HIDRA_DEBUG("DoReceive (w/ complete building) took {} us", duration);
  if (std::chrono::steady_clock::now() - m_last_receive_log > 2000ms) {
    m_last_receive_log = std::chrono::steady_clock::now();
    HIDRA_INFO("DoReceive (w/ complete building) took {} us", duration);
  }
}
