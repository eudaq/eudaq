#ifndef EUDAQ_INCLUDED_CommandReceiver
#define EUDAQ_INCLUDED_CommandReceiver

#include "eudaq/Status.hh"
#include "eudaq/Platform.hh"


#include <thread>
#include <memory>
//#include <pthread.h>
#include <string>
#include <iosfwd>

namespace eudaq {

  class TransportClient;
  class TransportEvent;
  class Configuration;

  class DLLEXPORT CommandReceiver {
  public:
    CommandReceiver(const std::string & type, const std::string & name, const std::string & runcontrol,
      bool startthread = true);
    void SetStatus(Status::Level level, const std::string & info = "");
    virtual ~CommandReceiver();

    virtual void OnConfigure(const Configuration & param);
    virtual void OnPrepareRun(unsigned /*runnumber*/) {}
    virtual void OnStartRun(unsigned /*runnumber*/) {}
    virtual void OnStopRun() {}
    virtual void OnTerminate() {}
    virtual void OnReset() {}
    virtual void OnStatus() {}
    virtual void OnData(const std::string & /*param*/) {}
    virtual void OnLog(const std::string & /*param*/);
    virtual void OnServer() {}
    virtual void OnGetRun() {}
    virtual void OnIdle();
    virtual void OnClear();
    virtual void OnUnrecognised(const std::string & /*cmd*/, const std::string & /*param*/) {}

    void Process(int timeout);
    void CommandThread();
    void StartThread();
    const std::string& get_name(){ return m_name; }
  protected:
    Status m_status;
    TransportClient * m_cmdclient;
  private:
    bool m_done;
    std::string m_type, m_name;
    void CommandHandler(TransportEvent &);
    std::unique_ptr<std::thread> m_thread;
    bool m_threadcreated;
  };

}

#endif // EUDAQ_INCLUDED_CommandReceiver
