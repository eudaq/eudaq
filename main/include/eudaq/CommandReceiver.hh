#ifndef EUDAQ_INCLUDED_CommandReceiver
#define EUDAQ_INCLUDED_CommandReceiver

#include "eudaq/TransportClient.hh"
#include "eudaq/Configuration.hh"
#include "eudaq/Status.hh"
#include <pthread.h>
#include <string>
#include <iosfwd>

namespace eudaq {

  class CommandReceiver {
  public:
    CommandReceiver(const std::string & type, const std::string & name, const std::string & runcontrol,
                    bool startthread = true);
    void SetStatus(Status::Level level, const std::string & info = "");
    virtual ~CommandReceiver();

    virtual void OnConfigure(const Configuration & param) { std::cout << "Config:\n" << param << std::endl; }
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
  protected:
    Status m_status;
    TransportClient * m_cmdclient;
  private:
    bool m_done;
    std::string m_type, m_name;
    void CommandHandler(TransportEvent &);
    pthread_t m_thread;
    pthread_attr_t m_threadattr;
    bool m_threadcreated;
  };

}

#endif // EUDAQ_INCLUDED_CommandReceiver
