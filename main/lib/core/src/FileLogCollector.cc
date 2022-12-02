#include "eudaq/LogCollector.hh"
#include "eudaq/Utils.hh"
#include "eudaq/FileNamer.hh"
#include "eudaq/Logger.hh"

#include <iostream>
#include <thread>
#include <chrono>
namespace eudaq{

  class FileLogCollector : public eudaq::LogCollector {
  public:
    FileLogCollector(const std::string &name, const std::string &runcontrol);
    void DoInitialise() override final;
    
    void DoReceive(const LogMessage &ev) override final;
    static const uint32_t m_id_factory = eudaq::cstr2hash("FileLogCollector");
  private:
    uint32_t m_level_write;
    uint32_t m_level_print;
    std::string m_file_pattern;
    std::ofstream m_os_file;
    std::string m_start_time;
  };

  namespace{
    auto dummy0 = eudaq::Factory<eudaq::LogCollector>::
      Register<FileLogCollector, const std::string&, const std::string&>(FileLogCollector::m_id_factory);
  }
  namespace{
    auto dummy1 = eudaq::Factory<eudaq::LogCollector>::
      Register<FileLogCollector, const std::string&, const std::string&>(eudaq::cstr2hash("log"));
  }

  FileLogCollector::FileLogCollector(const std::string &name, const std::string &runcontrol)
    :eudaq::LogCollector(name, runcontrol){
    m_file_pattern = "FileLog$12D.log";
    m_level_write = 0;
    m_level_print = 0;

    std::time_t time_now = std::time(nullptr);
    char time_buff[13];
    time_buff[12] = 0;
    std::strftime(time_buff, sizeof(time_buff),
		  "%y%m%d%H%M%S", std::localtime(&time_now));
    m_start_time = std::string(time_buff);
  }
  
  void FileLogCollector::DoInitialise(){
    auto ini = GetInitConfiguration();
    m_file_pattern = "FileLog$12D.log";
    m_level_write = 0;
    m_level_print = 0;
    if(ini){
      m_file_pattern = ini->Get("FILE_PATTERN", m_file_pattern);
      m_level_write = ini->Get("LOG_LEVEL_WRITE", m_level_write);
      m_level_print = ini->Get("LOG_LEVEL_PRINT", m_level_print);
    }
    m_os_file.open(std::string(eudaq::FileNamer(m_file_pattern)
			       .Set('D', m_start_time)).c_str(),
		   std::ios_base::app);
    std::stringstream ss;
    ss << "\n*** LogCollector started at " << Time::Current().Formatted()
       << " ***\n";
    m_os_file<<ss.str();
  }
  
  void FileLogCollector::DoReceive(const eudaq::LogMessage &msg){
    if (msg.GetLevel() >= m_level_print)
      std::cout << msg << std::endl;
    if (msg.GetLevel() >= m_level_write && m_os_file.is_open())
      m_os_file << msg << std::endl;
  }
}
