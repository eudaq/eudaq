#include <iostream>
#include <ostream>
#include <cctype>
//#include <csignal>

#include "eudaq/Utils.hh"
#include "eudaq/LogCollector.hh"
#include "eudaq/Logger.hh"
#include "eudaq/OptionParser.hh"

#include <unistd.h>

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
    virtual void OnConfigure(const std::string & param) {
      std::cout << "Configure: " << param << std::endl;
      SetStatus(eudaq::Status::LVL_OK);
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
      SetStatus(eudaq::Status::LVL_OK);
    }
    virtual void OnStatus() {
      std::cout << "Status - " << m_status << std::endl;
      //SetStatus(eudaq::Status::LVL_WARNING, "Only joking");
    }
    virtual void OnUnrecognised(const std::string & cmd, const std::string & param) {
      std::cout << "Unrecognised: (" << cmd.length() << ") " << cmd;
      if (param.length() > 0) std::cout << " (" << param << ")";
      std::cout << std::endl;
      SetStatus(eudaq::Status::LVL_ERROR, "Just testing");
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
