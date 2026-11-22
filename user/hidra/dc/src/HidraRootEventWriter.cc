#include "HidraRootEventWriter.hh"
#include "Logger.hh"

#include <cstdint>
#include <deque>
#include <mutex>
#include <thread>
#include <utility>

#if __has_include("TFile.h") && __has_include("TTree.h")
#include "HidraRootPayloadDecoders.hh"
#include "HidraUtils.hh"
#include "TFile.h"
#include "TTree.h"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <exception>
#include <map>
#include <string>
#include <vector>
#define HIDRA_HAS_ROOT_HEADERS 1
#else
#define HIDRA_HAS_ROOT_HEADERS 0
#endif

namespace hidra {

#if HIDRA_HAS_ROOT_HEADERS
using namespace std::chrono_literals;

namespace {

std::vector<std::uint8_t> CollectPayload(const eudaq::Event& event) {
  std::vector<std::uint8_t> payload;
  for (const auto block_id : event.GetBlockNumList()) {
    const auto block = event.GetBlock(block_id);
    payload.insert(payload.end(), block.begin(), block.end());
  }
  return payload;
}

RootDetectorPayload MakeDetectorPayload(const eudaq::Event& event, int det_id, const std::string& producer) {
  RootDetectorPayload detector;
  detector.det_id = det_id;
  detector.producer = producer;
  detector.trigger_n = event.GetTriggerN();
  detector.event_time_begin = event.GetTimestampBegin();
  detector.event_time_end = event.GetTimestampEnd();
  detector.payload = CollectPayload(event);
  return detector;
}

std::string MakeRootBranchName(std::string name) {
  for (auto& ch : name) {
    const auto value = static_cast<unsigned char>(ch);
    if (!std::isalnum(value) && ch != '_') {
      ch = '_';
    }
  }
  if (name.empty() || std::isdigit(static_cast<unsigned char>(name.front()))) {
    name.insert(name.begin(), '_');
  }
  return name;
}

} // namespace

struct HidraRootEventWriter::Impl {
  Impl(std::string output_file, std::uint64_t flush_interval_ms, std::size_t flush_every_events,
       std::map<int, std::string> vme_geo_map, uint8_t log_level=1)
      : output_file(std::move(output_file)), flush_interval_ms(flush_interval_ms),
        flush_every_events(flush_every_events), xdc_decoder{vme_geo_map, log_level} {
    if (this->flush_interval_ms == 0) {
      this->flush_interval_ms = 1;
    }
    EUDAQ_LOG_LEVEL((int)(log_level));
    HIDRA_INFO("HidraRootEventWriter created with output file path: {}", this->output_file);
    for(auto vme_geo: vme_geo_map) {
      vme_geo_vect.push_back(vme_geo.first);
    }
    XDCoffset.resize(vme_geo_vect.size(), 0);
  }

  std::string output_file;
  std::uint64_t flush_interval_ms = 50;
  std::size_t flush_every_events = 32;
  std::thread writer_thread;
  mutable std::mutex mutex;
  std::deque<eudaq::EventSP> queue;
  bool running = false;
  bool stop_requested = false;
  bool has_error = false;
  std::string error_message;
  std::atomic<uint64_t> written_events = 0;
  std::atomic<uint64_t> tdc_missing_events = 0;
  std::uint64_t last_warn_tdc_missing_count = 0;
  std::chrono::steady_clock::time_point last_missing_tdc_warning_time = std::chrono::steady_clock::now();

  int run_number = 0;
  std::uint32_t event_number = 0;
  std::uint64_t event_time = 0;
  std::uint32_t time_span = 0;
  std::uint32_t spill_number = 0;
  std::uint8_t detector_mask = 0;
  std::uint8_t trigger_mask = 0;
  std::uint32_t event_flags = 0;
  int n_detectors = 0;
  

  std::vector<int> q_det;
  std::vector<std::string> q_name;
  std::vector<double> q_value;
  std::vector<uint64_t> XDCoffset;
  std::vector<int> vme_geo_vect;
  std::vector<std::string> q_unit;
  std::map<std::string, std::vector<double>> root_branch_values;

  HidraXdcPayloadDecoder xdc_decoder;
  HidraFersPayloadDecoder fers_decoder;
  HidraMaxiccPayloadDecoder maxicc_decoder;
  HidraTrackerPayloadDecoder tracker_decoder;
  HidraGenericPayloadDecoder generic_decoder;

  void ResetEntryBuffers() {
    q_det.clear();
    q_name.clear();
    q_value.clear();
    q_unit.clear();
    for (auto& branch : root_branch_values) {
      branch.second.clear();
    }
  }

  void FillMetadata(const eudaq::Event& event) {
    run_number = event.GetRunN();
    event_number = static_cast<std::uint32_t>(event.GetTriggerN());
    event_time = event.GetTimestampBegin();
    time_span = event.GetTimestampEnd() > event.GetTimestampBegin() ? static_cast<std::uint32_t>(event.GetTimestampEnd() - event.GetTimestampBegin()) : 0;
    spill_number = hidra::utils::getTagOr<std::uint32_t>(event, "spillNumber", 0xFFFFFFFF);
    detector_mask = hidra::utils::getTagOr<std::uint8_t>(event, "detectorMask", 0xFF);
    trigger_mask = hidra::utils::getTagOr<std::uint8_t>(event, "triggerMask", 0xFF);
    event_flags = hidra::utils::GetEventFlagMask(event);
    n_detectors = event.GetNumSubEvent();
  }

  void AddQuantities(int det_id, const std::vector<RootQuantity>& quantities) {
    for (const auto& quantity : quantities) {
      q_det.push_back(det_id);
      q_name.push_back(quantity.name);
      q_value.push_back(quantity.value);
      q_unit.push_back(quantity.unit);
    }
  }

  std::vector<double>& EnsureRootBranch(TTree* tree, const std::string& requested_name) {
    const auto branch_name = MakeRootBranchName(requested_name);
    auto result = root_branch_values.insert(std::make_pair(branch_name, std::vector<double>{}));
    auto& values = result.first->second;

    if (result.second) {
      tree->Branch(branch_name.c_str(), &values);
    }

    return values;
  }

  void RegisterKnownRootBranches(TTree* tree) {
    for (const auto& name : generic_decoder.BranchNames()) {
      EnsureRootBranch(tree, name);
    }
    for (const auto& name : xdc_decoder.BranchNames()) {
      EnsureRootBranch(tree, name);
    }
    for (const auto& name : fers_decoder.BranchNames()) {
      EnsureRootBranch(tree, name);
    }
    for (const auto& name : maxicc_decoder.BranchNames()) {
      EnsureRootBranch(tree, name);
    }
    for (const auto& name : tracker_decoder.BranchNames()) {
      EnsureRootBranch(tree, name);
    }
  }

  void AddBranchValues(TTree* tree, const RootBranchValues& branches) {
    for (const auto& branch : branches) {
      auto& values = EnsureRootBranch(tree, branch.first);
      values.insert(values.end(), branch.second.begin(), branch.second.end());
    }
  }

  void DecodeDetector(TTree* tree, int det_id, const std::string& producer, const eudaq::Event& detector_event) {
    auto detector = MakeDetectorPayload(detector_event, det_id, producer);

    if (xdc_decoder.Matches(detector)) {
      xdc_decoder.Decode(detector, detector.quantities, detector.branches);
      const auto tdcs = detector.branches.find("TDCs");
      if (tdcs != detector.branches.end() && hidra::utils::isXDCEmpty(tdcs->second)) {
        ++tdc_missing_events;
      }
      const auto xdc_triggers = detector.branches.find("XDCTriggers");
      if(xdc_triggers != detector.branches.end()) {
        const auto adcs = detector.branches.find("ADCs");
        const auto tdcs = detector.branches.find("TDCs");
        const auto trigger_count = std::min({xdc_triggers->second.size(), XDCoffset.size(), vme_geo_vect.size()});
        if (xdc_triggers->second.size() != trigger_count) {
          HIDRA_WARN("Event {}: XDCTriggers has {} entries but {} VME modules are configured. Ignoring extra entries.",
                     detector.trigger_n,
                     xdc_triggers->second.size(),
                     trigger_count);
        }
        for(std::size_t i = 0; i < trigger_count; i++) {
          const auto trigger_value = xdc_triggers->second[i];
          if (trigger_value < 0) {
            continue;
          }
          auto trig_offset = static_cast<uint64_t>(trigger_value) - detector.trigger_n;
          if (trig_offset != XDCoffset[i]) {
            
            HIDRA_WARN("Event {}: XDC at geo {} has new offset {}, incremented {} from previous {} value. Total channel count: ADC {}, TDC {}", 
              detector.trigger_n, 
              vme_geo_vect[i],
              trig_offset, 
              static_cast<int>(trig_offset) - XDCoffset[i], 
              XDCoffset[i], 
              adcs == detector.branches.end() ? 0 : adcs->second.size(),
              tdcs == detector.branches.end() ? 0 : tdcs->second.size());
            XDCoffset[i]=trig_offset;
          }
        }
      }
    } else if (maxicc_decoder.Matches(detector)) {
      maxicc_decoder.Decode(detector, detector.quantities, detector.branches);
    } else if (fers_decoder.Matches(detector)) {
      fers_decoder.Decode(detector, detector.quantities, detector.branches);
    } else if (tracker_decoder.Matches(detector)) {
      tracker_decoder.Decode(detector, detector.quantities, detector.branches);
    } else {
      generic_decoder.Decode(detector, detector.quantities, detector.branches);
    }

    AddQuantities(det_id, detector.quantities);
    AddBranchValues(tree, detector.branches);
  }

  void WriteEvent(TTree* tree, const eudaq::Event& event) {
    FillMetadata(event);
    ResetEntryBuffers();

    for (int index = 0; index < event.GetNumSubEvent(); ++index) {
      auto subevent = event.GetSubEvent(index);
      if (!subevent) {
        continue;
      }

      const int det_id = hidra::utils::getTagOr<int>(*subevent, "detID", index);
      const std::string producer = subevent->HasTag("Producer") ? subevent->GetTag("Producer") : "";
      DecodeDetector(tree, det_id, producer, *subevent);
    }

    tree->Fill();
    ++written_events;
    if(std::chrono::steady_clock::now() - last_missing_tdc_warning_time > 15s && tdc_missing_events != last_warn_tdc_missing_count) {
      last_warn_tdc_missing_count = tdc_missing_events;
      last_missing_tdc_warning_time = std::chrono::steady_clock::now();
      HIDRA_WARN("Root payload decoder found {} events with missing TDC words on the total of {} decoded events", tdc_missing_events.load(), written_events.load());
    }
  }

  void WriterLoop() {
    std::unique_ptr<TFile> file;
    std::unique_ptr<TTree> tree;

    try {
      // Worker thread creates and owns TFile/TTree
      file = std::make_unique<TFile>(output_file.c_str(), "RECREATE");
      if (!file || file->IsZombie()) {
        std::lock_guard<std::mutex> lock(mutex);
        has_error = true;
        error_message = "Cannot open ROOT output file: " + output_file;
        HIDRA_ERROR("{}", error_message);
        return;
      }

      tree = std::make_unique<TTree>("hidra", "HIDRA live decoded quantities");
      tree->Branch("run", &run_number, "run/I");
      tree->Branch("event", &event_number, "event/i");
      tree->Branch("event_time", &event_time, "event_time/l");
      tree->Branch("time_span", &time_span, "time_span/i");
      tree->Branch("spill_number", &spill_number, "spill_number/i");
      tree->Branch("detector_mask", &detector_mask, "detector_mask/b");
      tree->Branch("trigger_mask", &trigger_mask, "trigger_mask/b");
      tree->Branch("event_flags", &event_flags, "event_flags/i");
      tree->Branch("n_detectors", &n_detectors, "n_detectors/I");
      /* 
      TODO: Clean the rest of the code and delete these obsolete vectors and all the functions specifically for filling them, then remove these branches as well
      tree->Branch("q_det", &q_det);
      tree->Branch("q_name", &q_name);
      tree->Branch("q_value", &q_value);
      tree->Branch("q_unit", &q_unit);
      */
      RegisterKnownRootBranches(tree.get());

      size_t events_since_flush = 0;
      auto last_flush = std::chrono::steady_clock::now();

      while (true) {
        std::deque<eudaq::EventSP> local_batch;
        {
          std::lock_guard<std::mutex> lock(mutex);
          if (stop_requested && queue.empty()) {
            break;
          }
          std::swap(local_batch, queue);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(flush_interval_ms));

        for (auto& event : local_batch) {
          if (event) {
            WriteEvent(tree.get(), *event);
            ++events_since_flush;
          }
        }

        const auto now = std::chrono::steady_clock::now();
        const bool flush_by_batch = flush_every_events > 0 && events_since_flush >= flush_every_events;
        const bool flush_by_time =
            std::chrono::duration_cast<std::chrono::milliseconds>(now - last_flush).count() >=
            static_cast<int64_t>(flush_interval_ms);

        if ((flush_by_batch || flush_by_time || local_batch.empty()) && events_since_flush > 0) {
          file->cd();
          tree->Write("", TObject::kOverwrite);
          file->Flush();
          events_since_flush = 0;
          last_flush = now;
        }
      }

      // Final flush and write - all on worker thread
      if (events_since_flush > 0) {
        file->cd();
        tree->Write("", TObject::kOverwrite);
        file->Flush();
      }
      
      // Write tree to file before cleanup
      file->cd();
      tree->Write("", TObject::kOverwrite);
      
      // Let ROOT handle cleanup through unique_ptr destructors
      // Don't call file->Close() explicitly - let the destructor handle it
      // unique_ptrs will call destructors when they go out of scope
    } catch (const std::exception& ex) {
      std::lock_guard<std::mutex> lock(mutex);
      has_error = true;
      error_message = ex.what();
      HIDRA_ERROR("ROOT writer failed: {}", error_message);
    }
    
    // unique_ptrs auto-delete here in reverse order (tree first, then file)
  }
};

HidraRootEventWriter::HidraRootEventWriter(const std::string& output_file, std::uint64_t flush_interval_ms,
                                           std::size_t flush_every_events,
                                           std::map<int, std::string> vme_geo_map,
                                           uint8_t log_level)
    : m_impl(std::make_unique<Impl>(output_file, flush_interval_ms, flush_every_events, std::move(vme_geo_map), log_level)) {}

HidraRootEventWriter::~HidraRootEventWriter() { Stop(); }

void HidraRootEventWriter::Start() {
  {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    if (m_impl->running) {
      return;
    }

    m_impl->running = true;
    m_impl->stop_requested = false;
    m_impl->has_error = false;
    m_impl->error_message.clear();
    m_impl->written_events = 0;
    m_impl->queue.clear();
  }

  m_impl->writer_thread = std::thread(&Impl::WriterLoop, m_impl.get());
}

void HidraRootEventWriter::Stop() {
  {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    if (!m_impl->running) {
      return;
    }
    m_impl->stop_requested = true;
  }

  if (m_impl->writer_thread.joinable()) {
    m_impl->writer_thread.join();
  }

  {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    m_impl->queue.clear();
    m_impl->running = false;
  }

  if (m_impl->last_warn_tdc_missing_count != m_impl->tdc_missing_events) {
    HIDRA_WARN("Root payload decoder found {} events with missing TDC words on the total of {} decoded events", m_impl->tdc_missing_events.load(), m_impl->written_events.load());
  }

}

bool HidraRootEventWriter::EnqueueEvent(eudaq::EventSP event) {
  if (!event) {
    return false;
  }

  std::lock_guard<std::mutex> lock(m_impl->mutex);
  if (!m_impl->running || m_impl->has_error) {
    return false;
  }

  m_impl->queue.push_back(std::move(event));
  return true;
}

bool HidraRootEventWriter::IsActive() const {
  std::lock_guard<std::mutex> lock(m_impl->mutex);
  return m_impl->running && !m_impl->has_error;
}

bool HidraRootEventWriter::HasError() const {
  std::lock_guard<std::mutex> lock(m_impl->mutex);
  return m_impl->has_error;
}

std::string HidraRootEventWriter::GetLastError() const {
  std::lock_guard<std::mutex> lock(m_impl->mutex);
  return m_impl->error_message;
}

std::uint64_t HidraRootEventWriter::GetWrittenEventCount() const {
  std::lock_guard<std::mutex> lock(m_impl->mutex);
  return m_impl->written_events;
}

std::size_t HidraRootEventWriter::GetPendingEventCount() const {
  std::lock_guard<std::mutex> lock(m_impl->mutex);
  return m_impl->queue.size();
}

#else

struct HidraRootEventWriter::Impl {
  Impl(std::string output_file, std::uint64_t flush_interval_ms, std::size_t flush_every_events,
       std::map<int, std::string>)
      : output_file(std::move(output_file)), flush_interval_ms(flush_interval_ms), flush_every_events(flush_every_events) {}

  std::string output_file;
  std::uint64_t flush_interval_ms = 50;
  std::size_t flush_every_events = 32;
  std::thread writer_thread;
  mutable std::mutex mutex;
  std::deque<eudaq::EventSP> queue;
  bool running = false;
  bool stop_requested = false;
  bool has_error = true;
  std::string error_message = "ROOT headers are not available";
  std::uint64_t written_events = 0;
};

HidraRootEventWriter::HidraRootEventWriter(const std::string& output_file, std::uint64_t flush_interval_ms,
                                           std::size_t flush_every_events,
                                           std::map<int, std::string> vme_geo_map)
    : m_impl(std::make_unique<Impl>(output_file, flush_interval_ms, flush_every_events, std::move(vme_geo_map))) {}

HidraRootEventWriter::~HidraRootEventWriter() = default;

void HidraRootEventWriter::Start() {}

void HidraRootEventWriter::Stop() {}

bool HidraRootEventWriter::EnqueueEvent(eudaq::EventSP) { return false; }

bool HidraRootEventWriter::IsActive() const { return false; }

bool HidraRootEventWriter::HasError() const { return true; }

std::string HidraRootEventWriter::GetLastError() const { return m_impl->error_message; }

std::uint64_t HidraRootEventWriter::GetWrittenEventCount() const { return 0; }

std::size_t HidraRootEventWriter::GetPendingEventCount() const { return 0; }

#endif

} // namespace hidra
