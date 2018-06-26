#ifndef EUDAQ_INCLUDED_CommandReceiver
#define EUDAQ_INCLUDED_CommandReceiver

#include "eudaq/Status.hh"
#include "eudaq/Platform.hh"
#include "eudaq/Configuration.hh"
#include "eudaq/Logger.hh"

#include <thread>
#include <memory>
#include <string>
#include <iosfwd>
#include <future>
#include <queue>
#include <mutex>
#include <condition_variable>

namespace eudaq {

  class TransportClient;
  class TransportEvent;

  class DLLEXPORT CommandReceiver {
  public:
    CommandReceiver(const std::string & type, const std::string & name,
		    const std::string & runctrl);
    virtual ~CommandReceiver();
    virtual void OnInitialise();
    virtual void OnConfigure();
    virtual void OnStartRun();
    virtual void OnStopRun();
    virtual void OnTerminate();
    virtual void OnReset();
    virtual void OnStatus();
    virtual void OnLog(const std::string &log);
    virtual void OnIdle();
    virtual void OnUnrecognised(const std::string &cmd, const std::string &argv);
    virtual void RunLoop();
    std::string Connect();

    //    virtual void Exec() = 0;

    void ReadConfigureFile(const std::string &path);
    void ReadInitializeFile(const std::string &path);
    
    
    void StartCommandReceiver();
    void CloseCommandReceiver();

    bool IsActiveCommandReceiver(){return !m_exited && m_thd_client.joinable();};
    
    void SendStatus();
    void SetStatus(Status::State, const std::string&);
    void SetStatusMsg(const std::string&);
    void SetStatusTag(const std::string &key, const std::string &val);

<<<<<<< HEAD
    void ReadConfigureFile(const std::string &path);
    void ReadInitializeFile(const std::string &path);

    void SetRunN(uint32_t n){m_run_number = n;}    

    
    void StartCommandReceiver();
    void CloseCommandReceiver();

    bool IsActiveCommandReceiver(){return !m_exited && m_thd_client.joinable();};
    std::string GetFullName() const;
    std::string GetName() const;
    uint32_t GetRunNumber() const;
    ConfigurationSPC GetConfiguration() const;
    ConfigurationSPC GetInitConfiguration() const;
=======
    void SetRunN(uint32_t n){m_run_number = n;}    
>>>>>>> psarun

    std::string GetFullName() const; // {return m_type+"."+m_name;};
    std::string GetName() const;// {return m_name;};
    uint32_t GetCommandReceiverID() const {return m_cmdrcv_id;};
    uint32_t GetRunNumber() const; // {return m_run_number;};
    std::string GetCommandRecieverAddress() const {return m_addr_client;};
    
    std::shared_ptr<const Configuration> GetConfiguration() const; // {return m_conf;};
    std::shared_ptr<const Configuration> GetInitConfiguration() const; //  {return m_conf_init;};
    std::string GetConfigItem(const std::string &key) const;
    std::string GetInitItem(const std::string &key) const;

    bool IsConnected() const;
    bool IsStatus(Status::State);

  private:
    void ProcessingCommand();
    void CommandHandler(TransportEvent &);
    void ProcessingCommand();
    bool Deamon();
    bool AsyncForwarding();
    bool AsyncReceiving();
    bool RunLooping();

  private:
    std::unique_ptr<TransportClient> m_cmdclient;
    std::string m_addr_client;
    std::string m_addr_runctrl;
    bool m_is_destructing;
    bool m_is_connected;
    bool m_is_runlooping;
    std::future<bool> m_fut_async_rcv;
    std::future<bool> m_fut_async_fwd;
    std::future<bool> m_fut_deamon;
    std::future<bool> m_fut_runloop;
    std::mutex m_mx_qu_cmd;
    std::mutex m_mx_deamon;
    std::queue<std::pair<std::string, std::string>> m_qu_cmd;
    std::condition_variable m_cv_not_empty;
    Status m_status;
    std::mutex m_mtx_status;
    std::shared_ptr<Configuration> m_conf;
    std::shared_ptr<Configuration> m_conf_init;
    std::string m_type;
    std::string m_name;
    std::thread m_thd_client;
    uint32_t m_run_number;
    std::thread m_thd_client;
    bool m_exit;
    bool m_exited;
    uint32_t m_cmdrcv_id;
  };
}

#endif // EUDAQ_INCLUDED_CommandReceiver
