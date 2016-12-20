#ifndef EUDAQ_INCLUDED_RunControl
#define EUDAQ_INCLUDED_RunControl

#include "eudaq/TransportServer.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Status.hh"
#include "eudaq/Configuration.hh"
#include "eudaq/Platform.hh"
#include "eudaq/Factory.hh"

#include <string>
#include <memory>
#include <thread>
#include <mutex>

namespace eudaq {

  class RunControl;
#ifndef EUDAQ_CORE_EXPORTS
  extern template class DLLEXPORT Factory<RunControl>;
  extern template DLLEXPORT
  std::map<uint32_t, typename Factory<RunControl>::UP_BASE (*)(const std::string&)>&
  Factory<RunControl>::Instance<const std::string&>();  
#endif
  
  /** Implements the functionality of the Run Control application.
   *
   */
  class DLLEXPORT RunControl {
  public:
    explicit RunControl(const std::string &listenaddress = "");
    virtual ~RunControl(){};

    //run in user thread
    virtual void Configure(const Configuration &settings);
    virtual void StartRun(uint32_t run_n); 
    virtual void StopRun();
    virtual void Reset();
    virtual void RemoteStatus();
    virtual void Terminate();
    //
    
    //run in m_thd_server thread
    virtual void DoConnect(const ConnectionInfo & /*id*/) {}
    virtual void DoDisconnect(const ConnectionInfo & /*id*/) {}
    virtual void DoStatus(const ConnectionInfo & /*id*/,
			  std::shared_ptr<Status>) {}
    //

    //thread control
    void StartRunControl();
    void CloseRunControl();
    bool IsActiveRunControl() const {return m_thd_server.joinable();}
    virtual void Exec();
    //

    Configuration ReadConfigFile(const std::string &param);
    size_t NumConnections() const { return m_cmdserver->NumConnections(); }
    const ConnectionInfo &GetConnection(size_t i) const {
      return m_cmdserver->GetConnection(i);
    }
    
  private:
    void InitLog(const ConnectionInfo &id);
    void InitData(const ConnectionInfo &id);
    void InitOther(const ConnectionInfo &id);
    void SendCommand(const std::string &cmd,
		     const std::string &param = "",
                     const ConnectionInfo &id = ConnectionInfo::ALL);
    std::string SendReceiveCommand(const std::string &cmd,
				   const std::string &param = "",
				   const ConnectionInfo &id = ConnectionInfo::ALL);
    void CommandHandler(TransportEvent &ev);
    void CommandThread();

  private:
    bool m_exit;
    bool m_listening;
    std::thread m_thd_server;
    std::unique_ptr<TransportServer> m_cmdserver; ///< Transport for sending commands
    std::map<std::string, std::string> m_addr_data;
    std::string m_addr_log;
    std::mutex m_mtx_sendcmd;
  public:    
    //TODO: move to derived class
    // virtual void DoConfigureLocal(Configuration &config);
    int32_t GetRunNumber() const{ return m_runnumber;}
    int64_t GetRunSizeLimit() const {return m_runsizelimit;}
    int64_t GetRunEventLimit() const {return m_runeventlimit;}
    int64_t m_runsizelimit;
    int64_t m_runeventlimit;
    int32_t m_runnumber;
    
  };
}

#endif // EUDAQ_INCLUDED_RunControl
