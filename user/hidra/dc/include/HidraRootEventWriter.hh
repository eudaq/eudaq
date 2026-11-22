#pragma once

#include <eudaq/Event.hh>

#include <cstdint>
#include <map>
#include <memory>
#include <string>

namespace hidra {

class HidraRootEventWriter {
public:
  HidraRootEventWriter(const std::string& output_file, std::uint64_t flush_interval_ms = 50,
                       std::size_t flush_every_events = 32,
                       std::map<int, std::string> vme_geo_map = {}, uint8_t log_level = 1);
  ~HidraRootEventWriter();

  void Start();
  void Stop();

  bool EnqueueEvent(eudaq::EventSP event);

  bool IsActive() const;
  bool HasError() const;
  std::string GetLastError() const;
  std::uint64_t GetWrittenEventCount() const;
  std::size_t GetPendingEventCount() const;

private:
  struct Impl;
  std::unique_ptr<Impl> m_impl;
};

} // namespace hidra
