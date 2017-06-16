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

  using RunControlUP = Factory<RunControl>::UP_BASE;
  
  /** Implements the functionality of the Run Control application.
   *
   */
  //----------DOC-MARK-----BEG*DEC-----DOC-MARK----------
  class DLLEXPORT RunControl {
  public:
    explicit RunControl(const std::string &listenaddress = "");
    
    //run in user thread
    virtual void Initialise();
    virtual void Configure();
    virtual void StartRun(); 
    virtual void StopRun();
    virtual void Reset();
    virtual void Terminate();
    
    //run in m_thd_server thread
    virtual void DoConnect(ConnectionSPC con) {}
    virtual void DoDisconnect(ConnectionSPC con) {}
    virtual void DoStatus(ConnectionSPC con, StatusSPC st) {}

    bool IsActiveConnection(ConnectionSPC con);
    StatusSPC GetConnectionStatus(ConnectionSPC con);
    std::vector<ConnectionSPC> GetActiveConnections();
    std::map<ConnectionSPC, StatusSPC> GetActiveConnectionStatusMap();
    
    //thread control
    void StartRunControl(); 
    void CloseRunControl();
    bool IsActiveRunControl() const {return m_thd_server.joinable();}
    virtual void Exec();
    
    void SetRunN(uint32_t n){m_run_n = n;};
    uint32_t GetRunN() const {return m_run_n;};
    void ReadConfigureFile(const std::string &path);
    void ReadInitilizeFile(const std::string &path);
    ConfigurationSPC GetConfiguration() const {return m_conf;};
    ConfigurationSPC GetInitConfiguration() const {return m_conf_init;};
    
    static const uint32_t m_id_factory = eudaq::cstr2hash("DefaultRunControl");
  private:
    void SendCommand(const std::string &cmd,
		     const std::string &param = "",
                     ConnectionSPC id = ConnectionSPC());
    void CommandHandler(TransportEvent &ev);
    void CommandThread();
    void StatusThread();
  private:
    bool m_exit;
    bool m_listening;
    std::thread m_thd_server;
    std::thread m_thd_status;
    std::unique_ptr<TransportServer> m_cmdserver;
    std::shared_ptr<Configuration> m_conf;
    std::shared_ptr<Configuration> m_conf_init;
    std::map<ConnectionSPC, StatusSPC> m_conn_status;
    std::mutex m_mtx_conn;

    std::string m_addr_log;
    std::mutex m_mtx_sendcmd;
    uint32_t m_run_n;
  };
  //----------DOC-MARK-----END*DEC-----DOC-MARK----------
}

#endif // EUDAQ_INCLUDED_RunControl
