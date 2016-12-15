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
    void OnConfigure(const Configuration & param)override final{}
    void OnStartRun(uint32_t /*runnumber*/)override final{}
    void OnStopRun() override final{};
    void OnReset() override final{}; //TODO: reset member variable
    void OnServer() override;
    void OnTerminate() override final;
    //void OnLog(const std::string &param) override final{};
    void OnData(const std::string &param) override final{};
    void Exec() override;

    virtual void DoTerminate() = 0;
    virtual void DoConnect(const ConnectionInfo & /*id*/) {}
    virtual void DoDisconnect(const ConnectionInfo & /*id*/) {}
    virtual void DoReceive(const LogMessage &msg) = 0;

    void LogThread();

  private:
    void LogHandler(TransportEvent &ev);
    bool m_done, m_listening;
    std::unique_ptr<TransportServer> m_logserver; ///< Transport for receiving log messages
    std::thread m_thread;
    std::string m_filename;
    std::ofstream m_file;
  };
}

#endif // EUDAQ_INCLUDED_LogCollector
