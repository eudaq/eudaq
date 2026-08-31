#ifndef HIDRA_DRY_FERS_PRODUCER_HH
#define HIDRA_DRY_FERS_PRODUCER_HH

#include "eudaq/Producer.hh"
#include <cstring>

class HidraDryFERSProducer : public eudaq::Producer {
public:
  HidraDryFERSProducer(const std::string& name, const std::string& runcontrol);

  void DoInitialise() override;
  void DoConfigure() override;
  void DoStartRun() override;
  void DoStopRun() override;
  void DoTerminate() override;
  void DoReset() override;
  void sleepUntilNext(uint64_t b_last_evt, uint64_t b_current_evt, uint64_t last_real); // all in usec
  void ReadFileInfo();
  void Mainloop();

  static const uint32_t m_id_factory = eudaq::cstr2hash("HidraDryFERSProducer");

private:
  int m_file_run_number;
  int m_eudaq_run_number;
  std::ifstream m_ifile;
  uint64_t m_ifile_size;
  std::size_t m_bytes_read;
  std::string m_data_in_path;
  int m_event_spacing_us; // microsec
  std::thread m_thd_run;
  mutable bool m_exit_of_run;
};

#endif
