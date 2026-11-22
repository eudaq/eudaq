#ifndef HIDRA_DRY_XDC_PRODUCER_HH
#define HIDRA_DRY_XDC_PRODUCER_HH

#include "eudaq/Producer.hh"

#include <chrono>
#include <cstdint>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

class HidraDryXDCProducer : public eudaq::Producer {
public:
  HidraDryXDCProducer(const std::string& name, const std::string& runcontrol);

  void DoInitialise() override;
  void DoConfigure() override;
  void DoStartRun() override;
  void DoStopRun() override;
  void DoTerminate() override;
  void DoReset() override;
  void ReadFileSize();
  bool ReadXDCEvent(std::vector<uint32_t>& event_words);
  void Mainloop();

  static const uint32_t m_id_factory = eudaq::cstr2hash("HidraDryXDCProducer");

private:
  std::ifstream m_ifile;
  uint64_t m_ifile_size;
  std::string m_data_in_path;
  uint64_t m_prev_event_timestamp_ns;
  uint64_t m_first_event_timestamp_ns;
  long long m_event_spacing_ns;
  std::thread m_thd_run;
  mutable bool m_exit_of_run;
  std::chrono::time_point<std::chrono::steady_clock> m_start_of_run_ts;

  uint32_t m_runNumber;
};

#endif
