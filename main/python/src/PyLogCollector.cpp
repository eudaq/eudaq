#include <iostream>
#include <ostream>
#include <cctype>


#include "eudaq/Utils.hh"
#include "eudaq/LogCollector.hh"
#include "eudaq/Logger.hh"
#include "eudaq/OptionParser.hh"
#ifndef WIN32
#include <unistd.h>
#endif
class PyLogCollector : public eudaq::LogCollector {
  public:
    PyLogCollector(const std::string & runcontrol,
        const std::string & listenaddress,
        int loglevel)
      : eudaq::LogCollector(runcontrol, listenaddress),
      m_loglevel(loglevel), done(false)
  {}
    void OnConnect(const eudaq::ConnectionInfo & id) {
      std::cout << "Connect:    " << id << std::endl;
    }
    void OnDisconnect(const eudaq::ConnectionInfo & id) {
      std::cout << "Disconnect: " << id << std::endl;
    }
    virtual void OnReceive(const eudaq::LogMessage & ev) {
      if (ev.GetLevel() >= m_loglevel) std::cout << ev << std::endl;
    }
    virtual void OnConfigure(const std::string & param) {
      std::cout << "Configure: " << param << std::endl;
      SetConnectionState(eudaq::ConnectionState::STATE_CONF);
    }
    virtual void OnStartRun(unsigned param) {
      std::cout << "Start Run: " << param << std::endl;
    }
    virtual void OnStopRun() {
      std::cout << "Stop Run" << std::endl;
    }
    virtual void OnTerminate() {
      std::cout << "Terminating" << std::endl;
      done = true;
    }
    virtual void OnReset() {
      std::cout << "Reset" << std::endl;
      SetConnectionState(eudaq::ConnectionState::STATE_UNCONF);
    }
    virtual void OnStatus() {
      std::cout << "ConnectionState - " << m_connectionstate << std::endl;
    }
    virtual void OnUnrecognised(const std::string & cmd, const std::string & param) {
      std::cout << "Unrecognised: (" << cmd.length() << ") " << cmd;
      if (param.length() > 0) std::cout << " (" << param << ")";
      std::cout << std::endl;
      //SetConnectionState(eudaq::ConnectionState::LVL_ERROR, "Just testing");
    }
    int m_loglevel;
    bool done;
};

// ctypes can only talk to C functions -- need to provide them through 'extern "C"'
extern "C" {
  DLLEXPORT PyLogCollector* PyLogCollector_new(char *rcaddress, char *listenaddress, char *loglevel){return new PyLogCollector(std::string(rcaddress),std::string(listenaddress), eudaq::Status::String2Level(loglevel));}
  DLLEXPORT void PyLogCollector_SetLogLevel(PyLogCollector *plc, char *loglevel){plc->m_loglevel = eudaq::Status::String2Level(loglevel);}

}
