#include <iostream>
#include <ostream>
#include <cctype>


#include "eudaq/Utils.hh"
#include "eudaq/DataCollector.hh"
#include "eudaq/Logger.hh"
#include "eudaq/OptionParser.hh"
#ifndef WIN32
#include <unistd.h>
#endif
class TestDataCollector : public eudaq::DataCollector {
  public:
  TestDataCollector(const std::string & name, const std::string & runcontrol,
		    const std::string & listenaddress, const std::string & runnumberfile)
    : eudaq::DataCollector(name, runcontrol, listenaddress, runnumberfile),
      done(false)
  {}
    void OnConnect(const eudaq::ConnectionInfo & id) {
      DataCollector::OnConnect(id);
      std::cout << "Connect:    " << id << std::endl;
    }
    virtual void OnConfigure(const eudaq::Configuration & param) {
      std::cout << "Configuring (" << param.Name() << ")..." << std::endl;
      DataCollector::OnConfigure(param);
      std::cout << "...Configured (" << param.Name() << ")" << std::endl;
      SetConnectionState(eudaq::ConnectionState::STATE_CONF, "Configured (" + param.Name() + ")");
    }
    virtual void OnStartRun(unsigned param) {
      DataCollector::OnStartRun(param);
      SetConnectionState(eudaq::ConnectionState::STATE_RUNNING);
    }
    //   virtual bool OnStartRun(unsigned param) {
    //     std::cout << "Start Run: " << param << std::endl;
    //     return false;
    //   }
    //   virtual bool OnStopRun() {
    //     std::cout << "Stop Run" << std::endl;
    //     return false;
    //   }
    virtual void OnTerminate() {
      std::cout << "Terminating" << std::endl;
      done = true;
    }
    virtual void OnReset() {
      std::cout << "Reset" << std::endl;
     // SetConnectionState(eudaq::ConnectionState::STATE_UNCONF);
    }
    //   virtual void OnStatus() {
    //     DataCollector::OnStatus();
    //     //std::cout << "ConnectionState - " << m_connectionstate << std::endl;
    //   }
    virtual void OnUnrecognised(const std::string & cmd, const std::string & param) {
      std::cout << "Unrecognised: (" << cmd.length() << ") " << cmd;
      if (param.length() > 0) std::cout << " (" << param << ")";
      std::cout << std::endl;
      //SetConnectionState(eudaq::ConnectionState::LVL_WARN, "Just testing");
    }
    bool done;
};

// static TestDataCollector * g_ptr = 0;
// void ctrlc_handler(int) {
//   if (g_ptr) g_ptr->done = 1;
// }

int main(int /*argc*/, const char ** argv) {
  eudaq::OptionParser op("EUDAQ Data Collector", "1.0", "A command-line version of the Data Collector");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol", "tcp://localhost:44000", "address",
      "The address of the RunControl application");
  eudaq::Option<std::string> addr (op, "a", "listen-address", "tcp://44001", "address",
      "The address on which to listen for Data connections");
  eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level",
      "The minimum level for displaying log messages locally");
  eudaq::Option<std::string> name (op, "n", "name", "", "string",
      "The name of this DataCollector");
  eudaq::Option<std::string> runnumberfile (op, "f", "runnumberfile", "../data/runnumber.dat", "string",
      "The path and name of the file containing the run number of the last run.");
  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL(level.Value());
    TestDataCollector fw(name.Value(), rctrl.Value(), addr.Value(), runnumberfile.Value());
    //g_ptr = &fw;
    //std::signal(SIGINT, &ctrlc_handler);
    do {
      eudaq::mSleep(10);
    } while (!fw.done);
    std::cout << "Quitting" << std::endl;
  } catch (...) {
    std::cout << "DataCollector exception handler" << std::endl;
    return op.HandleMainException();
  }
  return 0;
}
