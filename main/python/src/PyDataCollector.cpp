#include <iostream>

#include "eudaq/Utils.hh"
#include "eudaq/DataCollector.hh"
#include "eudaq/Logger.hh"
#include "eudaq/OptionParser.hh"

class PyDataCollector : public eudaq::DataCollector {
  public:
  PyDataCollector(const std::string & name, 
		  const std::string & runcontrol,
		  const std::string & listenaddress,
		  const std::string & runnumberfile)
    : eudaq::DataCollector(name, runcontrol, listenaddress, runnumberfile)
  {}
    void OnConnect(const eudaq::ConnectionInfo & id) {
      DataCollector::OnConnect(id);
      std::cout << "[PyDataCollector] Connect:    " << id << std::endl;
    }
    virtual void OnConfigure(const eudaq::Configuration & param) {
      std::cout << "[PyDataCollector] Configuring (" << param.Name() << ")..." << std::endl;
      DataCollector::OnConfigure(param);
      std::cout << "[PyDataCollector] ...Configured (" << param.Name() << ")" << std::endl;
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
      std::cout << "[PyDataCollector] Terminating" << std::endl;
    }

    virtual void OnReset() {
      std::cout << "[PyDataCollector] Reset" << std::endl;
      SetConnectionState(eudaq::ConnectionState::STATE_UNCONF);
    }
    //   virtual void OnStatus() {
    //     DataCollector::OnStatus();
    //     //std::cout << "ConnectionState - " << m_connectionstate << std::endl;
    //   }
    virtual void OnUnrecognised(const std::string & cmd, const std::string & param) {
      std::cout << "[PyDataCollector] Unrecognised: (" << cmd.length() << ") " << cmd;
      if (param.length() > 0) std::cout << " (" << param << ")";
      std::cout << std::endl;
      //SetConnectionState(eudaq::ConnectionState::LVL_WARN, "Just testing");
    }
};


// ctypes can only talk to C functions -- need to provide them through 'extern "C"'
extern "C" {
  DLLEXPORT PyDataCollector* PyDataCollector_new(char *name, char *rcaddress, char *listenaddress, char *runnumberfile){return new PyDataCollector(std::string(name), std::string(rcaddress),std::string(listenaddress), std::string(runnumberfile));}
}
