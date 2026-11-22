#include <eudaq/Configuration.hh>
#include <eudaq/Event.hh>
#include <eudaq/Factory.hh>
#include <eudaq/Logger.hh>
#include <eudaq/Producer.hh>
#include "HidraUtils.hh"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

namespace {

// Official tracker file format: whitespace-separated, no header, one event per
// line, 29 fields:
//   [0..5]   x1 y1 x2 y2 x3 y3  - measured coordinates (cm, double, E-notation);
//                                 a large negative value (e.g. -5000/-6000) is a
//                                 "no hit" sentinel for that coordinate.
//   [6..17]  12 cluster strip counts (two-digit, one per module).
//   [18..28] 11 hex fields:
//     18 event number from start of run
//     19 constant marker 0x50ABCDEF
//     20 silicon timestamp, low 16 bits
//     21 silicon timestamp, upper 8 bits
//     22 timestamp sent to DREAM, low 16 bits
//     23 timestamp sent to DREAM, upper 8 bits
//     24 timestamp sent to DREAM, packed
//     25 event number from start of run + 1
//     26 DREAM event number   <-- the cross-detector alignment key (trigger)
//     27 event number within the spill
//     28 global event number
constexpr std::size_t TRACKER_NFIELDS = 29;
constexpr std::size_t NCOORDS = 6; // x1,y1,x2,y2,x3,y3 -> 3 stations
constexpr std::size_t STRIP_FIRST_INDEX = 6;
constexpr std::size_t STRIP_COUNT = 12;

constexpr std::size_t EVENT_FROM_START_INDEX = 18;
constexpr std::size_t MARKER_INDEX = 19;
constexpr std::size_t SILICON_TS_LOW_INDEX = 20;
constexpr std::size_t SILICON_TS_HIGH_INDEX = 21;
constexpr std::size_t DREAM_TS_LOW_INDEX = 22;
constexpr std::size_t DREAM_TS_HIGH_INDEX = 23;
constexpr std::size_t DREAM_TS_PACKED_INDEX = 24;
constexpr std::size_t EVENT_FROM_START_PLUS1_INDEX = 25;
constexpr std::size_t DREAM_EVENT_NUMBER_INDEX = 26; // alignment key -> SetTriggerN
constexpr std::size_t EVENT_IN_SPILL_INDEX = 27;
constexpr std::size_t GLOBAL_EVENT_NUMBER_INDEX = 28;

constexpr std::uint32_t TRACKER_MARKER = 0x50ABCDEF;

std::string Trim(const std::string& value) {
  const auto first = value.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) {
    return {};
  }
  const auto last = value.find_last_not_of(" \t\r\n");
  return value.substr(first, last - first + 1);
}


// Split a line on runs of whitespace (the official format is space-separated).
std::vector<std::string> Tokenize(const std::string& line) {
  std::vector<std::string> fields;
  std::istringstream stream(line);
  std::string field;
  while (stream >> field) {
    fields.push_back(field);
  }
  return fields;
}

// Where a field came from, for error messages — bundled so the per-field
// parsers don't each take file + line separately.
struct RowContext {
  const std::filesystem::path& file;
  std::size_t line;
};

[[noreturn]] void ThrowParse(const char* what, const std::string& value, const RowContext& ctx, std::size_t column) {
  HIDRA_THROW("Cannot parse tracker " + std::string(what) + " (column " + std::to_string(column + 1) + ") at " +
              ctx.file.string() + ":" + std::to_string(ctx.line) + ": " + value);
}

double ParseDouble(const std::string& value, const RowContext& ctx, std::size_t column) {
  std::size_t parsed = 0;
  try {
    const double result = std::stod(value, &parsed);
    if (parsed != value.size()) {
      throw std::invalid_argument("trailing characters");
    }
    if (!std::isfinite(result)) {
      // A non-finite coordinate should never occur in real data; warn (but still
      // keep the value, like every other field) so a corrupt input is flagged.
      HIDRA_WARN("Non-finite tracker coordinate (column " + std::to_string(column + 1) + ") at " + ctx.file.string() +
                 ":" + std::to_string(ctx.line) + ": " + value);
    }
    return result;
  } catch (const std::exception&) {
    ThrowParse("coordinate", value, ctx, column);
  }
}

std::uint64_t ParseHex(const std::string& value, const RowContext& ctx, std::size_t column) {
  std::size_t parsed = 0;
  try {
    const std::uint64_t result = std::stoull(value, &parsed, 16);
    if (parsed != value.size()) {
      throw std::invalid_argument("trailing characters");
    }
    return result;
  } catch (const std::exception&) {
    ThrowParse("hex field", value, ctx, column);
  }
}

} // namespace

// EUDAQ producer for the silicon tracker.
//
// The tracker writes ASCII files into a watched directory; this producer tails
// that directory from a background thread and turns every complete data line
// into one `TrackerRaw` event:
//   * the three stations' (x, y) coordinates (cm) are packed into block 0 as
//     doubles (see HidraTrackerDecoder, which reads them back);
//   * the DREAM event number becomes the EUDAQ trigger number, so the
//     DataCollector can align the tracker with the other detectors;
//   * the remaining per-event counters and timestamps are attached as tags.
// The per-field layout is documented at the top of this file.
class HidraTrackerProducer : public eudaq::Producer {
public:
  HidraTrackerProducer(const std::string& name, const std::string& runcontrol)
      : eudaq::Producer(name, runcontrol) {}

  ~HidraTrackerProducer() override {
    StopWorker();
  }

  static const uint32_t m_id_factory = eudaq::cstr2hash("HidraTrackerProducer");

private:
  void DoInitialise() override {}

  void DoConfigure() override {
    const auto conf = GetConfiguration();
    if (!conf) {
      HIDRA_THROW("Run configuration is missing");
    }

    EUDAQ_LOG_LEVEL((int)(conf->Get("HIDRA_MUTE_DEBUG", 1)));
    // TRACKER_DIRECTORY: directory tailed for tracker files (required). ${VAR}
    // is expanded from the environment so the path can be machine-independent
    // (e.g. ${REPO_ROOT}/...); REPO_ROOT is exported by misc/setup.sh.
    m_directory = hidra::utils::ExpandEnv(conf->Get("TRACKER_DIRECTORY", std::string("/home/eudaq/TB2026_TrackerData/ascii_dream_2026")));
    if (m_directory.empty()) {
      HIDRA_THROW("TRACKER_DIRECTORY is missing from the run configuration");
    }
    if (!std::filesystem::is_directory(m_directory)) {
      HIDRA_THROW("TRACKER_DIRECTORY is not a directory: " + m_directory.string());
    }

    // TRACKER_FILE_EXTENSION: only files with this extension are read.
    m_extension = conf->Get("TRACKER_FILE_EXTENSION", std::string(".dat"));
    // TRACKER_POLL_INTERVAL_MS: directory re-scan period (>= 1 ms).
    m_poll_interval_ms = std::max(1, conf->Get("TRACKER_POLL_INTERVAL_MS", 100));
    // TRACKER_TIMESTAMP_SCALE_NS: multiplier from DREAM timestamp ticks to ns.
    m_timestamp_scale_ns = conf->Get("TRACKER_TIMESTAMP_SCALE_NS", uint64_t{1});
    if (m_timestamp_scale_ns == 0) {
      HIDRA_THROW("TRACKER_TIMESTAMP_SCALE_NS must be greater than zero");
    }

    HIDRA_INFO("HidraTrackerProducer watching " + m_directory.string());
  }

  void DoStartRun() override {
    StopWorker();
    m_run_number = GetRunNumber();
    m_events_sent = 0;
    m_processed_files.clear();
    m_observed_sizes.clear();

    auto bore = eudaq::Event::MakeUnique("TrackerRaw");
    bore->SetBORE();
    bore->SetRunN(static_cast<uint32_t>(m_run_number));
    bore->SetTag("Producer", "HidraTrackerProducer");
    bore->SetTag("Directory", m_directory.string());
    SendEvent(std::move(bore));

    m_running = true;
    m_worker = std::thread(&HidraTrackerProducer::MainLoop, this);
    HIDRA_INFO("Starting HidraTrackerProducer run " + std::to_string(m_run_number));
  }

  void DoStopRun() override {
    StopWorker();

    auto eore = eudaq::Event::MakeUnique("TrackerRaw");
    eore->SetEORE();
    eore->SetRunN(static_cast<uint32_t>(m_run_number));
    eore->SetTag("EventsSent", std::to_string(m_events_sent));
    SendEvent(std::move(eore));
    HIDRA_INFO("Stopping HidraTrackerProducer run " + std::to_string(m_run_number));
  }

  void DoReset() override {
    StopWorker();
    m_events_sent = 0;
    m_processed_files.clear();
    m_observed_sizes.clear();
  }

  void DoTerminate() override {
    StopWorker();
  }

  // Background worker: re-scan the directory every poll interval until stopped.
  // A scan failure is logged but doesn't kill the loop (the next scan retries).
  void MainLoop() {
    while (m_running) {
      try {
        ScanDirectory();
      } catch (const std::exception& error) {
        HIDRA_ERROR("Tracker directory {} scan failed: {}", m_directory.string(), error.what());
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(m_poll_interval_ms));
    }
  }

  void ScanDirectory() {
    std::vector<std::filesystem::path> files;
    for (const auto& entry : std::filesystem::directory_iterator(m_directory)) {
      HIDRA_DEBUG("File {}, exist: {}, size: {}", entry.path().string(), entry.exists(), entry.file_size());
      if (!entry.is_regular_file()) {
        continue;
      }
      if (!m_extension.empty() && entry.path().extension() != m_extension) {
        HIDRA_DEBUG("File {} found but wrong extension, expecting {}", entry.path().string(), m_extension);
        continue;
      }
      files.push_back(entry.path());
    }
    HIDRA_DEBUG("Found: {} files", files.size());

    // Keep only the last files in alphabetic order.
    // It shoud be the last spill in the last run
    std::sort(files.begin(), files.end(), std::greater<std::string>());
    files.resize(1);	

    for (const auto& file : files) {
      const auto key = file.string();
      if (m_processed_files.count(key) != 0) {
        continue; // already turned into events
      }

      // Only read a file once its size has stayed the same across two
      // consecutive scans, so we never read one the tracker is still writing.
      // A new or growing file is just recorded now and reconsidered next scan.
      const auto size = std::filesystem::file_size(file);
      const auto previous = m_observed_sizes.find(key);
      if (previous == m_observed_sizes.end() || previous->second != size) {
        m_observed_sizes[key] = size;
        continue;
      }

      ProcessFile(file);
      m_processed_files.insert(key);
      m_observed_sizes.erase(key);
    }
  }

  void ProcessFile(const std::filesystem::path& file) {
    std::ifstream input(file);
    if (!input.is_open()) {
      HIDRA_THROW("Cannot open tracker file: " + file.string());
    }

    std::string line;
    std::size_t line_number = 0;
    uint64_t rows_sent_for_file = 0;

    // The official format has no header: every non-empty line is one event.
    while (m_running && std::getline(input, line)) {
      ++line_number;
      if (Trim(line).empty()) {
        continue;
      }

      const auto fields = Tokenize(line);
      if (fields.size() != TRACKER_NFIELDS) {
        HIDRA_WARN("Ignoring tracker row with " + std::to_string(fields.size()) + " fields at " + file.string() + ":" +
                   std::to_string(line_number) + "; expected " + std::to_string(TRACKER_NFIELDS));
        continue;
      }

      SendRow(fields, file, line_number);
      ++rows_sent_for_file;
      ++m_events_sent;
    }

    HIDRA_INFO("Finished processing tracker file " + file.filename().string() + " with " +
               std::to_string(rows_sent_for_file) + " events");
  }

  void SendRow(const std::vector<std::string>& fields, const std::filesystem::path& file, std::size_t line_number) {
    const RowContext ctx{file, line_number};
    const auto dbl = [&](std::size_t column) { return ParseDouble(fields[column], ctx, column); };
    const auto hex = [&](std::size_t column) { return ParseHex(fields[column], ctx, column); };

    // Sanity check the constant marker so a mis-parsed/garbled line is dropped
    // rather than turned into a bogus event.
    const std::uint64_t marker = hex(MARKER_INDEX);
    if (marker != TRACKER_MARKER) {
      std::ostringstream msg;
      msg << "Tracker row at " << file.string() << ":" << line_number << " has marker 0x" << std::hex << std::uppercase
          << marker << ", expected 0x" << TRACKER_MARKER << "; skipping";
      HIDRA_WARN(msg.str());
      return;
    }

    // Measured coordinates (cm) as double, in (x, y) pairs per station.
    std::array<double, NCOORDS> coordinates{};
    for (std::size_t i = 0; i < NCOORDS; ++i) {
      coordinates[i] = dbl(i);
    }

    // The DREAM event number is the cross-detector alignment key (the trigger
    // the DataCollector merges on).
    const std::uint64_t dream_event = hex(DREAM_EVENT_NUMBER_INDEX);
    if (dream_event > std::numeric_limits<uint32_t>::max()) {
      HIDRA_THROW("DREAM event number is larger than the EUDAQ uint32 trigger number at " + file.string() + ":" +
                  std::to_string(line_number));
    }

    const std::uint64_t dream_ts = hex(DREAM_TS_PACKED_INDEX);
    if (dream_ts > (std::numeric_limits<uint64_t>::max() - 1) / m_timestamp_scale_ns) {
      HIDRA_THROW("DREAM timestamp overflows after TRACKER_TIMESTAMP_SCALE_NS at " + file.string() + ":" +
                  std::to_string(line_number));
    }

    auto event = eudaq::Event::MakeUnique("TrackerRaw");
    event->SetRunN(static_cast<uint32_t>(m_run_number));
    event->SetTriggerN(static_cast<uint32_t>(dream_event));
    event->SetEventN(static_cast<uint32_t>(dream_event));
    const uint64_t timestamp_ns = dream_ts * m_timestamp_scale_ns;
    event->SetTimestamp(timestamp_ns, timestamp_ns + 1);
    event->SetTag("nativeTimestampBegin", std::to_string(timestamp_ns));
    event->SetTag("Producer", "HidraTrackerProducer");
    event->SetTag("SourceFile", file.filename().string());
    // Keep the auxiliary per-event identifiers/timestamps as tags for offline use.
    event->SetTag("SiliconTimestampLow", std::to_string(hex(SILICON_TS_LOW_INDEX)));
    event->SetTag("SiliconTimestampHigh", std::to_string(hex(SILICON_TS_HIGH_INDEX)));
    event->SetTag("DreamTimestamp", std::to_string(dream_ts));
    event->SetTag("endianness", m_machine_endianness); // TODO review this tag
    // Cluster strip counts, space-joined in module order.
    std::ostringstream strips;
    for (std::size_t i = 0; i < STRIP_COUNT; ++i) {
      if (i != 0) {
        strips << ' ';
      }
      strips << fields[STRIP_FIRST_INDEX + i];
    }
    event->SetTag("StripCounts", strips.str());

    // Coordinate block: NCOORDS doubles (x1,y1,x2,y2,x3,y3), native byte order.
    event->AddBlock(0, coordinates.data(), coordinates.size() * sizeof(double));
    event->SetTag("detectorDataSize", std::to_string(event->GetBlock(0).size()));
    
    SendEvent(std::move(event));
  }

  void StopWorker() {
    m_running = false;
    if (m_worker.joinable()) {
      m_worker.join();
    }
  }

  std::filesystem::path m_directory;     // watched directory (TRACKER_DIRECTORY)
  std::string m_extension = ".csv";      // file extension filter
  int m_poll_interval_ms = 100;          // directory re-scan period
  uint64_t m_timestamp_scale_ns = 1;     // DREAM timestamp ticks -> ns
  std::atomic<bool> m_running{false};    // worker run flag (also stops mid-file)
  std::thread m_worker;                  // background directory watcher
  uint32_t m_run_number = 0;
  uint64_t m_events_sent = 0;
  std::unordered_set<std::string> m_processed_files; // files already consumed
  std::map<std::string, uintmax_t> m_observed_sizes; // last-seen size, for the stability check
  std::string m_machine_endianness = hidra::utils::is_little_endian() ? "LE" : "BE";
};

namespace {
auto dummy0 = eudaq::Factory<eudaq::Producer>::Register<HidraTrackerProducer, const std::string&, const std::string&>(
    HidraTrackerProducer::m_id_factory);
}
