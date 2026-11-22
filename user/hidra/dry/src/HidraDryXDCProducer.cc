#include "../include/HidraDryXDCProducer.hh"
#include "Logger.hh"
#include <chrono>
#include <cstring>
#include <exception>
#include <limits>
#include <string>

namespace {
constexpr uint32_t XDC_EVENT_MARKER = 0xccaaffeeu;
constexpr uint32_t XDC_HEADER_END_MARKER = 0xaccadeadu;
constexpr uint32_t XDC_EVENT_TRAILER = 0xbbeeddaau;
constexpr uint32_t XDC_HEADER_WORDS = 14u;
constexpr uint32_t XDC_TRAILER_WORDS = 1u;
} // namespace

namespace {
auto dummy0 = eudaq::Factory<eudaq::Producer>::Register<HidraDryXDCProducer, const std::string&, const std::string&>(
    HidraDryXDCProducer::m_id_factory);
}

HidraDryXDCProducer::HidraDryXDCProducer(const std::string& name, const std::string& runcontrol)
    : eudaq::Producer(name, runcontrol),
      m_exit_of_run(false) {}

void HidraDryXDCProducer::DoInitialise() {
  auto ini = GetInitConfiguration();
  (void)ini;
}

void HidraDryXDCProducer::DoConfigure() {
  auto conf = GetConfiguration();
  EUDAQ_LOG_LEVEL((int)(conf->Get("HIDRA_MUTE_DEBUG", 0)));
  m_event_spacing_ns = 1000000 * (long long)conf->Get("REPLAY_EVENT_SPACING_MS", -1);
  std::string inforeplay = m_event_spacing_ns < 0 ? "automatic" : std::to_string(m_event_spacing_ns) + " ns";
  EUDAQ_INFO("Replay rate set to " + inforeplay);
  m_data_in_path = conf->Get("DATA_IN_PATH", "infile.txt");
  EUDAQ_INFO("Using XDC raw data file " + m_data_in_path);
  ReadFileSize();
}

void HidraDryXDCProducer::DoStartRun() {
  // ReadFileSize() closes and reopens the replay file and seeks back to the start, so every run replays from the
  // beginning. DoStopRun() closes the file, so without this a second start (without a re-configure) would read from a
  // closed stream and replay nothing.
  ReadFileSize();

  m_runNumber = GetRunNumber();
  auto bore = eudaq::Event::MakeUnique("DryXDC");
  bore->SetBORE();
  bore->SetRunN(static_cast<uint32_t>(m_runNumber));
  bore->SetTag("Producer", "HidraDryXDCProducer");
  EUDAQ_INFO("Starting HidraDryXDCProducer run");
  SendEvent(std::move(bore));

  m_exit_of_run = false;
  m_start_of_run_ts = std::chrono::steady_clock::now();
  m_thd_run = std::thread(&HidraDryXDCProducer::Mainloop, this);
}

void HidraDryXDCProducer::DoStopRun() {
  auto eore = eudaq::Event::MakeUnique("DryXDC");
  eore->SetEORE();
  eore->SetRunN(static_cast<uint32_t>(m_runNumber));
  SendEvent(std::move(eore));

  m_exit_of_run = true;
  EUDAQ_INFO("Stopping HidraDryXDCProducer run");
  if (m_thd_run.joinable()) {
    m_thd_run.join();
  }
  m_ifile.close();
}

void HidraDryXDCProducer::DoReset() {
  m_exit_of_run = true;
  if (m_thd_run.joinable()) {
    m_thd_run.join();
  }

  m_ifile.close();
  m_thd_run = std::thread();
  m_exit_of_run = false;
}

void HidraDryXDCProducer::DoTerminate() {
  m_exit_of_run = true;
  if (m_thd_run.joinable()) {
    m_thd_run.join();
  }
}

// Open the replay file (closing it first if already open), record its size and seek back to the beginning. Called both
// at configure and at the start of every run, so each run replays from byte 0.
void HidraDryXDCProducer::ReadFileSize() {
  if (m_ifile.is_open()) {
    m_ifile.close();
  }

  m_ifile.open(m_data_in_path, std::ios::binary | std::ios::ate);
  if (!m_ifile.is_open()) {
    EUDAQ_THROW("input data file (" + m_data_in_path + ") can not open for reading");
  }

  m_ifile_size = (uint64_t)m_ifile.tellg();
  m_ifile.seekg(0, std::ios::beg);
}

bool HidraDryXDCProducer::ReadXDCEvent(std::vector<uint32_t>& event_words) {
  event_words.clear();

  auto read_word_ascii = [this](uint32_t& word) -> bool {
    std::string token;
    if (!(m_ifile >> token)) {
      return false;
    }

    try {
      const unsigned long value = std::stoul(token, nullptr, 16);
      if (value > std::numeric_limits<uint32_t>::max()) {
        EUDAQ_WARN("XDC token out of uint32 range: " + token);
        return false;
      }
      word = static_cast<uint32_t>(value);
      return true;
    } catch (const std::exception&) {
      EUDAQ_WARN("Invalid XDC token in ASCII stream: " + token);
      return false;
    }
  };

  std::vector<uint32_t> header(XDC_HEADER_WORDS, 0);
  for (uint32_t i = 0; i < XDC_HEADER_WORDS; ++i) {
    if (!read_word_ascii(header[i])) {
      return false;
    }
  }

  const uint32_t marker = header[0];
  const uint32_t header_size = header[3];
  const uint32_t trailer_size = header[4];
  const uint32_t data_size = header[5];
  const uint32_t event_size = header[6];
  const uint32_t header_end_marker = header[13];

  if (marker != XDC_EVENT_MARKER) {
    EUDAQ_WARN("Invalid XDC marker (" + std::to_string(marker) + "), stopping read loop");
    return false;
  }

  if (header_end_marker != XDC_HEADER_END_MARKER) {
    EUDAQ_WARN("Invalid XDC header end marker, stopping read loop");
    return false;
  }

  if (header_size != XDC_HEADER_WORDS || trailer_size != XDC_TRAILER_WORDS) {
    EUDAQ_WARN("Unexpected XDC header/trailer size, stopping read loop");
    return false;
  }

  if (event_size != (header_size + trailer_size + data_size)) {
    EUDAQ_WARN("Inconsistent XDC event size, stopping read loop");
    return false;
  }

  event_words.reserve(event_size);
  event_words.insert(event_words.end(), header.begin(), header.end());

  if (data_size > 0) {
    for (uint32_t i = 0; i < data_size; ++i) {
      uint32_t data_word = 0;
      if (!read_word_ascii(data_word)) {
        EUDAQ_WARN("Reached EOF or invalid token while reading XDC data words");
        return false;
      }
      event_words.push_back(data_word);
    }
  }

  uint32_t trailer = 0;
  if (!read_word_ascii(trailer)) {
    EUDAQ_WARN("Reached EOF or invalid token while reading XDC trailer");
    return false;
  }
  if (trailer != XDC_EVENT_TRAILER) {
    EUDAQ_WARN("Invalid XDC trailer marker, stopping read loop");
    return false;
  }
  event_words.push_back(trailer);

  return true;
}

void HidraDryXDCProducer::Mainloop() {
  EUDAQ_INFO("Starting XDC dry readout loop");

  uint64_t loop_count = 0;
  while (!m_exit_of_run) {
    std::vector<uint32_t> event_words;
    auto start_of_read_t = std::chrono::high_resolution_clock::now();
    if (!ReadXDCEvent(event_words)) {
      m_exit_of_run = true;
      break;
    }

    auto ev = eudaq::Event::MakeUnique("XDCEvent");

    const uint32_t event_number = event_words[1];
    const uint32_t spill_number = event_words[2];
    const uint32_t data_size = event_words[5];
    const uint32_t event_size = event_words[6];
    const uint64_t event_time_sec = static_cast<uint64_t>(event_words[7]);
    const uint64_t event_time_usec = static_cast<uint64_t>(event_words[8]);
    const uint32_t trigger_mask = event_words[9];
    const uint32_t is_ped_mask = event_words[10];
    const uint32_t is_ped_from_scaler = event_words[11];
    const uint32_t sanity_flag = event_words[12];

    const uint64_t ts_begin_ns = event_time_sec * 1000000000ULL + event_time_usec * 1000ULL;
    ev->SetTimestamp(ts_begin_ns, ts_begin_ns + 100ULL, true);
    ev->SetTag("nativeTimestampBegin", std::to_string(ts_begin_ns));
    ev->SetEventN(event_number);
    ev->SetTriggerN(event_number, true);

    ev->SetTag("spillNumber", spill_number);
    ev->SetTag("triggerMask", trigger_mask);
    ev->SetTag("isPedMask", is_ped_mask);
    ev->SetTag("isPedFromScaler", is_ped_from_scaler);
    ev->SetTag("sanityFlag", sanity_flag);
    ev->SetTag("dataWords", data_size);
    ev->SetTag("eventWords", event_size);
    ev->SetTag("endianness", "BE32");

    std::vector<uint8_t> payload(data_size * sizeof(uint32_t));
    if (data_size > 0) {
      const uint32_t* payload_words = event_words.data() + XDC_HEADER_WORDS;
      std::memcpy(payload.data(), payload_words, payload.size());
    }

    ev->SetTag("detectorDataSize", std::to_string(payload.size()));
    ev->AddBlock(0, payload);

    if (loop_count == 0) {
      EUDAQ_INFO("First XDC event parsed: eventNumber=" + std::to_string(event_number) +
                 ", spillNumber=" + std::to_string(spill_number) + ", dataWords=" + std::to_string(data_size));
    }

    if (loop_count > 0) {
      std::chrono::nanoseconds event_replay_delay{m_event_spacing_ns >= 0 ? m_event_spacing_ns
                                                                          : ts_begin_ns - m_prev_event_timestamp_ns};
      std::chrono::nanoseconds read_duration{std::chrono::duration_cast<std::chrono::nanoseconds>(
          std::chrono::high_resolution_clock::now() - start_of_read_t)};
      std::chrono::nanoseconds event_delay_ns = event_replay_delay - read_duration;
      // EUDAQ_DEBUG("Event time stamps: " + std::to_string(double(ts_begin_ns-m_first_event_timestamp_ns)/1000000000));
      if (event_delay_ns.count() > 0) {
        std::this_thread::sleep_for(event_delay_ns);
      }

    } else {
      m_first_event_timestamp_ns = ts_begin_ns;
    }
    m_prev_event_timestamp_ns = ts_begin_ns;

    SendEvent(std::move(ev));
    ++loop_count;
  }

  EUDAQ_INFO("Exiting XDC dry readout loop after " + std::to_string(loop_count) + " events");
}
