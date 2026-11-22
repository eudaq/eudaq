#ifndef HIDRA_LOG_COLLECTOR_GUI_HH
#define HIDRA_LOG_COLLECTOR_GUI_HH

#include "euLog.hh"

#include <cstdint>
#include <fstream>
#include <string>

class HidraLogCollectorGUI : public LogCollectorGUI {
public:
  HidraLogCollectorGUI(const std::string& name,
                       const std::string& runcontrol);

  void DoInitialise() override;
  void DoReceive(const eudaq::LogMessage& msg) override;

private:
  uint32_t m_level_print;
  uint32_t m_level_write;
  std::string m_file_pattern;
  std::ofstream m_os_file;
};

#endif
