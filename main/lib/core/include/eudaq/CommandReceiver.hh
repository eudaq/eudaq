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
		    const std::string & runcontrol);
    virtual ~CommandReceiver();
    virtual void OnInitialise(); 
    virtual void OnConfigure();
    virtual void OnStartRun();
    virtual void OnStopRun();
    virtual void OnTerminate();
    virtual void OnReset();
    virtual void OnStatus();
    virtual void OnLog(const std::string & /*param*/);
    virtual void OnUnrecognised(const std::string & /*cmd*/, const std::string & /*param*/);
    virtual bool RunLoop();
    
    void SetStatus(Status::State state, const std::string & info);
    bool IsStatus(Status::State state);
    void SetStatusTag(const std::string &key, const std::string &val);
    
    std::string GetFullName() const;
    std::string GetName() const;
    uint32_t GetRunNumber() const;
    std::string GetCommandRecieverAddress() const;
    
    ConfigurationSPC GetConfiguration() const;
    ConfigurationSPC GetInitConfiguration() const;

    std::string Connect();
    bool IsConnected();
    void StartCommandReceiver();
    bool IsActiveCommandReceiver();
  private:
    void CommandHandler(TransportEvent &);
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
    uint32_t m_run_number;
    
  };
}

#endif // EUDAQ_INCLUDED_CommandReceiver
