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
	   (const std::string&, const std::string&,
	    const std::string&, const int&)>&
  Factory<LogCollector>::Instance<const std::string&, const std::string&,
				  const std::string&, const int&>();//TODO: check const int& 
#endif
  
  class LogMessage;

  /** Implements the functionality of the File Writer application.
   *
   */
  class DLLEXPORT LogCollector : public CommandReceiver {
  public:
    LogCollector(const std::string &runcontrol,
                 const std::string &listenaddress,
		 const std::string & logdirectory);
    ~LogCollector() override;
    void OnConfigure() override final{}
    void OnStartRun() override final{}
    void OnStopRun() override final{};
    void OnReset() override final{}; //TODO: reset member variable
    void OnServer() override final;
    void OnTerminate() override final;
    void OnLog(const std::string &param) override final{};
    void OnData(const std::string &param) override final{};
    void Exec() override;

    virtual void DoTerminate() = 0;
    virtual void DoConnect(std::shared_ptr<const ConnectionInfo> id) {}
    virtual void DoDisconnect(std::shared_ptr<const ConnectionInfo> id) {}
    virtual void DoReceive(const LogMessage &msg) = 0;

    void StartLogCollector();
    void CloseLogCollector();
    bool IsActiveLogCollector(){return m_thd_server.joinable();}
  private:
    void LogThread();
    void LogHandler(TransportEvent &ev);
    bool m_exit;
    std::unique_ptr<TransportServer> m_logserver; ///< Transport for receiving log messages
    std::thread m_thd_server;
    std::string m_filename;
    std::ofstream m_file;
  };
}

#endif // EUDAQ_INCLUDED_LogCollector
