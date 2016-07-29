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
class TestLogCollector : public eudaq::LogCollector {
  public:
    TestLogCollector(const std::string & runcontrol,
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
    virtual void OnConfigure(const eudaq::Configuration & param) {
      std::cout << "Configure: " << param << std::endl;
      SetConnectionState(eudaq::ConnectionState::STATE_CONF, "Configured (" + param.Name() + ")");
    }
    virtual void OnStartRun(unsigned param) {
      std::cout << "Start Run: " << param << std::endl;
      SetConnectionState(eudaq::ConnectionState::STATE_RUNNING);
    }
    virtual void OnStopRun() {
      std::cout << "Stop Run" << std::endl;
      if(m_connectionstate.GetState() != eudaq::ConnectionState::STATE_ERROR)
        SetConnectionState(eudaq::ConnectionState::STATE_CONF);
    }
    virtual void OnTerminate() {
      std::cout << "Terminating" << std::endl;
      done = true;
    }
    virtual void OnReset() {
      std::cout << "Reset" << std::endl;
      //SetConnectionState(eudaq::ConnectionState::STATE_UNCONF);
    }
    virtual void OnStatus() {
      std::cout << "ConnectionState - " << m_connectionstate << std::endl;
      //SetConnectionState(eudaq::ConnectionState::LVL_WARNING, "Only joking");
    }
    virtual void OnUnrecognised(const std::string & cmd, const std::string & param) {
      std::cout << "Unrecognised: (" << cmd.length() << ") " << cmd;
      if (param.length() > 0) std::cout << " (" << param << ")";
      std::cout << std::endl;
      SetConnectionState(eudaq::ConnectionState::STATE_ERROR);
    }
    int m_loglevel;
    bool done;
};

// static TestLogCollector * g_ptr = 0;
// void ctrlc_handler(int) {
//   if (g_ptr) g_ptr->done = 1;
// }

int main(int /*argc*/, const char ** argv) {
  eudaq::OptionParser op("EUDAQ Log Collector", "1.0", "A comand-line version of the Log Collector");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol", "tcp://localhost:44000", "address",
      "The address of the RunControl application");
  eudaq::Option<std::string> addr (op, "a", "listen-address", "tcp://44002", "address",
      "The address on which to listen for Log connections");
  eudaq::Option<std::string> level(op, "l", "log-level", "INFO", "level",
      "The minimum level for displaying log messages");
  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL("NONE");
    TestLogCollector fw(rctrl.Value(), addr.Value(), eudaq::Status::String2Level(level.Value()));
    //g_ptr = &fw;
    //std::signal(SIGINT, &ctrlc_handler);
    do {
      eudaq::mSleep(10);
    } while (!fw.done);
    std::cout << "Quitting" << std::endl;
  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}
