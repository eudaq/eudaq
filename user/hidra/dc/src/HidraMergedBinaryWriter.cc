#include "HidraMergedBinaryWriter.hh"
#include "EventSerializer.hh"
#include "HidraUtils.hh"

#include <chrono>
#include <exception>

namespace hidra {

HidraMergedBinaryWriter::HidraMergedBinaryWriter(const std::string& output_file, uint64_t flush_interval_ms)
    : m_output_file(output_file), m_flush_interval_ms(flush_interval_ms) {
  if (m_flush_interval_ms == 0) {
    HIDRA_WARN("HidraMergedBinaryWriter: flush interval of 0 ms would busy-wait; using 1 ms instead");
    m_flush_interval_ms = 1;
  }
  HIDRA_INFO("HidraMergedBinaryWriter created with output file path: {}", output_file);
}

HidraMergedBinaryWriter::~HidraMergedBinaryWriter() { Stop(); }

void HidraMergedBinaryWriter::Start() {
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_running) {
      HIDRA_WARN("HidraMergedBinaryWriter: already running");
      return;
    }

    m_running = true;
    m_stop_requested = false;
    m_has_error = false;
    m_error_message.clear();
    m_events_written = 0;
    m_bytes_written = 0;
    m_events.clear();
  }

  m_writer_thread = std::thread(&HidraMergedBinaryWriter::WriterLoop, this);
  HIDRA_INFO("HidraMergedBinaryWriter started, output file: {}", m_output_file);
}

void HidraMergedBinaryWriter::Stop() {
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_running) {
      return;
    }
    m_stop_requested = true;
  }

  if (m_writer_thread.joinable()) {
    m_writer_thread.join();
  }

  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_running = false;
    m_events.clear();
  }

  HIDRA_INFO("HidraMergedBinaryWriter stopped. Events written: {}", m_events_written);
}

bool HidraMergedBinaryWriter::EnqueueEvent(eudaq::EventSP event) {
  if (!event) {
    return false;
  }

  std::lock_guard<std::mutex> lock(m_mutex);

  if (!m_running) {
    HIDRA_ERROR("HidraMergedBinaryWriter: not running");
    return false;
  }

  if (m_has_error) {
    HIDRA_ERROR("HidraMergedBinaryWriter: in error state: {}", m_error_message);
    return false;
  }

  m_events.push_back(std::move(event));
  return true;
}

size_t HidraMergedBinaryWriter::GetPendingEventCount() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_events.size();
}

bool HidraMergedBinaryWriter::IsActive() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_running && !m_has_error;
}

bool HidraMergedBinaryWriter::HasError() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_has_error;
}

std::string HidraMergedBinaryWriter::GetLastError() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_error_message;
}

uint64_t HidraMergedBinaryWriter::GetWrittenEventCount() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_events_written;
}

uint64_t HidraMergedBinaryWriter::GetWrittenByteCount() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_bytes_written;
}

void HidraMergedBinaryWriter::WriterLoop() {
  std::ofstream output;

  try {
    // Worker thread opens and owns the output file
    output.open(m_output_file, std::ios::binary);
    if (!output.is_open()) {
      std::lock_guard<std::mutex> lock(m_mutex);
      m_has_error = true;
      m_error_message = "Cannot open output file: " + m_output_file;
      HIDRA_ERROR("HidraMergedBinaryWriter: cannot open file {}", m_output_file);
      return;
    }

    auto last_flush = std::chrono::steady_clock::now();

    while (true) {
      std::deque<eudaq::EventSP> local_batch;
      {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_stop_requested && m_events.empty()) {
          break;
        }
        local_batch.swap(m_events);
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(m_flush_interval_ms));

      for (auto& event : local_batch) {
        if (!event) {
          continue;
        }
        uint64_t bytes_written = EventSerializer::WriteToStream(*event, output);
        ++m_events_written;
        m_bytes_written += bytes_written;
      }

      if (!local_batch.empty()) {
        const auto now = std::chrono::steady_clock::now();
        const bool flush_due = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_flush).count() >=
                               static_cast<int64_t>(m_flush_interval_ms);
        if (flush_due) {
          output.flush();
          last_flush = now;
        }
      }
    }

    // Final flush and close - all on worker thread
    output.flush();
    output.close();
  } catch (const std::exception& ex) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_has_error = true;
    m_error_message = ex.what();
    HIDRA_ERROR("HidraMergedBinaryWriter: write error: {}", m_error_message);
    throw;
  }
}

} // namespace hidra