#pragma once

#include <eudaq/Event.hh>
#include <eudaq/Logger.hh>

#include <cstdint>
#include <deque>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>

namespace hidra {

class HidraMergedBinaryWriter {
public:
  HidraMergedBinaryWriter(const std::string& output_file, uint64_t flush_interval_ms = 50);
  ~HidraMergedBinaryWriter();

  void Start();
  void Stop();

  bool EnqueueEvent(eudaq::EventSP event);

  size_t GetPendingEventCount() const;
  bool IsActive() const;
  bool HasError() const;
  std::string GetLastError() const;
  uint64_t GetWrittenEventCount() const;
  uint64_t GetWrittenByteCount() const;

private:
  void WriterLoop();

  std::string m_output_file;
  uint64_t m_flush_interval_ms;

  mutable std::mutex m_mutex;
  std::thread m_writer_thread;
  std::deque<eudaq::EventSP> m_events;
  bool m_running = false;
  bool m_stop_requested = false;
  bool m_has_error = false;
  std::string m_error_message;
  uint64_t m_events_written = 0;
  uint64_t m_bytes_written = 0;
};

} // namespace hidra