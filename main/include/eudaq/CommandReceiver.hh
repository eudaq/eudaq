#ifndef EUDAQ_INCLUDED_CommandReceiver
#define EUDAQ_INCLUDED_CommandReceiver

#include "eudaq/ConnectionState.hh"
#include "eudaq/Platform.hh"

#include <thread>
#include <memory>
//#include <pthread.h>
#include <string>
#include <iosfwd>
#include <iostream>

namespace eudaq {

  class TransportClient;
  class TransportEvent;
  class Configuration;

  class DLLEXPORT CommandReceiver {
  public:
    CommandReceiver(const std::string &type, const std::string &name,
                    const std::string &runcontrol, bool startthread = true);
    void SetConnectionState(ConnectionState::State state = ConnectionState::STATE_UNCONF, const std::string &info = "");

    int GetConnectionState() { return m_connectionstate.GetState();}

    virtual ~CommandReceiver();

    virtual void OnInitialise(const Configuration &param) { SetConnectionState(eudaq::ConnectionState::STATE_UNCONF);}
    virtual void OnConfigure(const Configuration &param);
    virtual void OnPrepareRun(unsigned /*runnumber*/) {}
    virtual void OnStartRun(unsigned /*runnumber*/) { SetConnectionState(eudaq::ConnectionState::STATE_CONF);}
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
    virtual void OnUnrecognised(const std::string & /*cmd*/,
                                const std::string & /*param*/) {}

    void Process(int timeout);
    void CommandThread();
    void StartThread();

  protected:    
    ConnectionState m_connectionstate;
    TransportClient *m_cmdclient;

  private:
    bool m_done;
    std::string m_type, m_name;
    void CommandHandler(TransportEvent &);
    std::unique_ptr<std::thread> m_thread;
    bool m_threadcreated;
  };
}

#endif // EUDAQ_INCLUDED_CommandReceiver
