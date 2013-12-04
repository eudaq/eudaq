#include <iostream>

#include "eudaq/Utils.hh"
#include "eudaq/DataCollector.hh"
#include "eudaq/Logger.hh"
#include "eudaq/OptionParser.hh"

class PyDataCollector : public eudaq::DataCollector {
  public:
    PyDataCollector(const std::string & runcontrol,
        const std::string & listenaddress)
      : eudaq::DataCollector(runcontrol, listenaddress),
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
      SetStatus(eudaq::Status::LVL_OK, "Configured (" + param.Name() + ")");
    }
    virtual void OnStartRun(unsigned param) {
      DataCollector::OnStartRun(param);
      SetStatus(eudaq::Status::LVL_OK);
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
      SetStatus(eudaq::Status::LVL_OK);
    }
    //   virtual void OnStatus() {
    //     DataCollector::OnStatus();
    //     //std::cout << "Status - " << m_status << std::endl;
    //   }
    virtual void OnUnrecognised(const std::string & cmd, const std::string & param) {
      std::cout << "Unrecognised: (" << cmd.length() << ") " << cmd;
      if (param.length() > 0) std::cout << " (" << param << ")";
      std::cout << std::endl;
      SetStatus(eudaq::Status::LVL_WARN, "Just testing");
    }
    bool done;
};


// ctypes can only talk to C functions -- need to provide them through 'extern "C"'
extern "C" {
  PyDataCollector* PyDataCollector_new(char *rcaddress, char *listenaddress){return new PyDataCollector(std::string(rcaddress),std::string(listenaddress));}
}
