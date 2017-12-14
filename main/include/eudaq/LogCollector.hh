#ifndef EUDAQ_INCLUDED_LogCollector
#define EUDAQ_INCLUDED_LogCollector

//#include <pthread.h>
#include <string>
#include <fstream>
#include "eudaq/Platform.hh"
#include "eudaq/Configuration.hh"
#include "eudaq/TransportServer.hh"
#include "eudaq/CommandReceiver.hh"
#include <memory>
#include <thread>

namespace eudaq {

  class LogMessage;

  /** Implements the functionality of the File Writer application.
   *
   */
  class DLLEXPORT LogCollector : public CommandReceiver {
  public:
    LogCollector(const std::string &runcontrol,
                 const std::string &listenaddress);

    virtual void OnConnect(const ConnectionInfo & /*id*/) {}
    virtual void OnDisconnect(const ConnectionInfo & /*id*/) {}
    virtual void OnServer();
    virtual void OnReceive(const LogMessage &msg) = 0;
    virtual ~LogCollector();

    virtual void OnStopRun()
    { 
      if (m_connectionstate.GetState() != eudaq::ConnectionState::STATE_ERROR)
        SetConnectionState(eudaq::ConnectionState::STATE_CONF); 
    }

    void LogThread();

  private:
    void LogHandler(TransportEvent &ev);
    void DoReceive(const LogMessage &msg);
    bool m_done, m_listening;
    TransportServer *m_logserver; ///< Transport for receiving log messages
                                  //      pthread_t m_thread;
                                  //      pthread_attr_t m_threadattr;
    std::unique_ptr<std::thread> m_thread;
    std::string m_filename;
    std::ofstream m_file;
  };
}

#endif // EUDAQ_INCLUDED_LogCollector
