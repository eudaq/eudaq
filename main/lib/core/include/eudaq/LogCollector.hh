#ifndef EUDAQ_INCLUDED_LogCollector
#define EUDAQ_INCLUDED_LogCollector

#include <string>
#include <fstream>
#include "eudaq/Platform.hh"
#include "eudaq/Configuration.hh"
#include "eudaq/TransportServer.hh"
#include "eudaq/CommandReceiver.hh"
#include "eudaq/Factory.hh"
#include <memory>
#include <thread>

namespace eudaq {
  class LogCollector;
#ifndef EUDAQ_CORE_EXPORTS
  extern template class DLLEXPORT Factory<LogCollector>;
  extern template DLLEXPORT
  std::map<uint32_t, typename Factory<LogCollector>::UP_BASE (*)
	   (const std::string&, const std::string&)>&
  Factory<LogCollector>::Instance<const std::string&, const std::string&>();
#endif
  
  class LogMessage;
  using LogCollectorSP = Factory<LogCollector>::SP_BASE;
  class DLLEXPORT LogCollector : public CommandReceiver {
  public:
    LogCollector(const std::string &name, const std::string &runcontrol);
    ~LogCollector() override;
    
    void OnInitialise() override final;
    void OnTerminate() override final;
    void OnLog(const std::string &param) override final{};
    virtual void Exec();

    virtual void DoInitialise(){};
    virtual void DoTerminate(){};

    virtual void DoConnect(ConnectionSPC id) {}
    virtual void DoDisconnect(ConnectionSPC id) {}
    virtual void DoReceive(const LogMessage &msg) = 0;

    void SetServerAddress(const std::string &addr){m_log_addr = addr;};
    void StartLogCollector();
    void CloseLogCollector();
    bool IsActiveLogCollector(){return m_thd_server.joinable();}

    static LogCollectorSP Make(const std::string &code_name,
			       const std::string &run_name,
			       const std::string &runcontrol);
  private:
    void LogThread();
    void LogHandler(TransportEvent &ev);
    bool m_exit;
    std::unique_ptr<TransportServer> m_logserver; ///< Transport for receiving log messages
    std::thread m_thd_server;
    std::string m_log_addr;
  };
}

#endif // EUDAQ_INCLUDED_LogCollector
