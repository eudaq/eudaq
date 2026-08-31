#include "HidraLogCollectorGUI.hh"

#include "eudaq/FileNamer.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Utils.hh"

#include <QModelIndex>

#include <ctime>
#include <iostream>

HidraLogCollectorGUI::HidraLogCollectorGUI(const std::string& name,
                                           const std::string& runcontrol)
    : LogCollectorGUI(name, runcontrol),
      m_level_print(0),
      m_level_write(0),
      m_file_pattern("HidraLog$12D.log") {
  qRegisterMetaType<QModelIndex>("QModelIndex");
  qRegisterMetaType<eudaq::LogMessage>("eudaq::LogMessage");
}

void HidraLogCollectorGUI::DoInitialise() {
  auto ini = GetInitConfiguration();

  m_file_pattern = "HidraLog$12D.log";
  m_level_print = 0;
  m_level_write = 0;

  if (ini) {
    m_file_pattern = ini->Get("FILE_PATTERN",
                              ini->Get("EULOG_GUI_LOG_FILE_PATTERN",
                                       m_file_pattern));
    m_level_print = ini->Get("LOG_LEVEL_PRINT", m_level_print);
    m_level_write = ini->Get("LOG_LEVEL_WRITE", m_level_write);
  }

  if (m_level_print < static_cast<uint32_t>(cmbLevel->count())) {
    cmbLevel->setCurrentIndex(m_level_print);
  }

  std::time_t time_now = std::time(nullptr);
  char time_buff[13];
  time_buff[12] = 0;
  std::strftime(time_buff, sizeof(time_buff), "%y%m%d%H%M%S",
                std::localtime(&time_now));

  m_os_file.open(std::string(eudaq::FileNamer(m_file_pattern)
                                 .Set('D', std::string(time_buff)))
                     .c_str(),
                 std::ios_base::app);

  if (!m_os_file) {
    EUDAQ_ERROR("Cannot open Hidra GUI log file: " + m_file_pattern);
    return;
  }

  m_os_file << "\n*** LogCollector started at "
            << eudaq::Time::Current().Formatted() << " ***\n";
}

void HidraLogCollectorGUI::DoReceive(const eudaq::LogMessage& msg) {
  if (msg.GetLevel() >= m_level_print) {
    std::cout << msg << std::endl;
  }

  if (msg.GetLevel() >= m_level_write && m_os_file.is_open()) {
    m_os_file << msg << std::endl;
  }

  if (msg.GetLevel() >= m_level_print) {
    emit RecMessage(msg);
  }
}
