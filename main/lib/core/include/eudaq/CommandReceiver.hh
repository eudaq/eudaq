#ifndef EUDAQ_INCLUDED_CommandReceiver
#define EUDAQ_INCLUDED_CommandReceiver

#include "eudaq/Status.hh"
#include "eudaq/Platform.hh"
#include "eudaq/Configuration.hh"


#include <thread>
#include <memory>
#include <string>
#include <iosfwd>

namespace eudaq {

  class TransportClient;
  class TransportEvent;

  class DLLEXPORT CommandReceiver {
  public:
    CommandReceiver(const std::string & type, const std::string & name, const std::string & runcontrol,
      bool startthread = true);
    void SetStatus(Status::Level level, const std::string & info = "");
    virtual ~CommandReceiver();

    virtual void OnConfigure(const Configuration & param);
    virtual void OnStartRun(uint32_t /*runnumber*/) {}
    virtual void OnStopRun() {}
    virtual void OnTerminate() {}
    virtual void OnReset() {}
    virtual void OnStatus() {}
    virtual void OnData(const std::string & /*param*/) {}
    virtual void OnLog(const std::string & /*param*/);
    virtual void OnServer() {}
    virtual void OnIdle();
    virtual void OnUnrecognised(const std::string & /*cmd*/, const std::string & /*param*/) {}

    void Process(int timeout);
    void CommandThread();
    void StartThread();
    const std::string& get_name() const{ return m_name; }
  protected:
    Status m_status;
    std::unique_ptr<TransportClient> m_cmdclient;
  private:
    bool m_done;
    std::string m_type, m_name;
    void CommandHandler(TransportEvent &);
    std::thread m_thread;
  };

}

#endif // EUDAQ_INCLUDED_CommandReceiver
