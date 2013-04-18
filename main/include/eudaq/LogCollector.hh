#ifndef EUDAQ_INCLUDED_LogCollector
#define EUDAQ_INCLUDED_LogCollector

#include <pthread.h>
#include <string>
#include <fstream>

#include "eudaq/TransportServer.hh"
#include "eudaq/CommandReceiver.hh"

namespace eudaq {

  class LogMessage;

  /** Implements the functionality of the File Writer application.
   *
   */
  class LogCollector : public CommandReceiver {
    public:
      LogCollector(const std::string & runcontrol,
          const std::string & listenaddress);

      virtual void OnConnect(const ConnectionInfo & /*id*/) {}
      virtual void OnDisconnect(const ConnectionInfo & /*id*/) {}
      virtual void OnServer();
      virtual void OnReceive(const LogMessage & msg) = 0;
      virtual ~LogCollector();

      void LogThread();
    private:
      void LogHandler(TransportEvent & ev);
      void DoReceive(const LogMessage & msg);
      bool m_done, m_listening;
      TransportServer * m_logserver; ///< Transport for receiving log messages
      pthread_t m_thread;
      pthread_attr_t m_threadattr;
      std::string m_filename;
      std::ofstream m_file;
  };

}

#endif // EUDAQ_INCLUDED_LogCollector
