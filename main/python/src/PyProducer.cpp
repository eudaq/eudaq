#include "eudaq/Configuration.hh"
#include "eudaq/Producer.hh"
#include "eudaq/Logger.hh"
#include "eudaq/StringEvent.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/AidaPacket.hh"
#include "eudaq/Utils.hh"
#include "eudaq/OptionParser.hh"

#include <iostream>
#ifndef WIN32
#include <inttypes.h> /* uint64_t */
#endif

using eudaq::StringEvent;
using eudaq::RawDataEvent;
using eudaq::AidaPacket;

class PyProducer : public eudaq::Producer {
  public:
    PyProducer(const std::string & name, const std::string & runcontrol)
      : eudaq::Producer(name, runcontrol), m_internalstate(Init), m_name(name), m_run(0), m_evt(0), m_config(NULL) {}
  
    void SendEvent(uint8_t* data, size_t size) {
      RawDataEvent ev(m_name, m_run, ++m_evt);
      ev.AddBlock(0, data, size);
      eudaq::DataSender::SendEvent(ev);
    }

    virtual void OnConfigure(const eudaq::Configuration & param) {
      std::cout << "[PyProducer] Received Configuration" << std::endl;
      m_config = new eudaq::Configuration(param);
      m_internalstate = Configuring;
      SetConnectionState(eudaq::ConnectionState::STATE_UNCONF, "Waiting for Python-side configure");
      // now wait until the python side has everything configured and called SetConfigured()
      while (m_internalstate != Configured){
	eudaq::mSleep(100);
      }
      if (m_internalstate == Configured) {
	SetConnectionState(eudaq::ConnectionState::STATE_CONF, "Configured (" + m_config->Name() + ")");
      }
    }

    virtual void OnStartRun(unsigned param) {
      m_run = param;
      m_evt = 0;
      // check if we are beyond the configuration stage
      if (!(m_internalstate==Configured || m_internalstate==Stopped)) {
	SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Cannot start run/unconfigured!");
	m_internalstate = Error;
      } else {
	m_internalstate = StartingRun;
	SetConnectionState(eudaq::ConnectionState::STATE_CONF, "Waiting for Python-side run start" );
      }
      // now wait until the python side is ready to start and called SendBORE()
      while (m_internalstate != Running){
	eudaq::mSleep(100);
      }
      if (m_internalstate == Running) {
	eudaq::DataSender::SendEvent(RawDataEvent::BORE(m_name, m_run));
	SetConnectionState(eudaq::ConnectionState::STATE_RUNNING);
      }
    }
    virtual void OnStopRun() {
      std::cout << "[PyProducer] Stop Run received" << std::endl;
      SetConnectionState(eudaq::ConnectionState::STATE_RUNNING, "Waiting for Python-side run stop");
      m_internalstate = StoppingRun;
      // now wait until the python side is ready to start and called SendEORE()
      while (m_internalstate != Stopped){
	eudaq::mSleep(100);
      }
      if (m_internalstate == Stopped) {
	eudaq::DataSender::SendEvent(RawDataEvent::EORE(m_name, m_run, ++m_evt));
  if(m_connectionstate != eudaq::ConnectionState::STATE_ERROR)
	 SetConnectionState(eudaq::ConnectionState::STATE_CONF);
      }
    }
    virtual void OnTerminate() {
      std::cout << "[PyProducer] Terminate received" << std::endl;
      m_internalstate = Terminating;
    }
    virtual void OnReset() {
      std::cout << "[PyProducer] Reset received" << std::endl;
      m_internalstate = Resetting;
      while (m_internalstate != Init){
	eudaq::mSleep(100);
      }
      if (m_internalstate == Init) {
	SetConnectionState(eudaq::ConnectionState::STATE_UNCONF);
      }
    }
    virtual void OnStatus() {
    }
    virtual void OnUnrecognised(const std::string & cmd, const std::string & param) {
      std::cout << "[PyProducer] Unrecognised: (" << cmd.length() << ") " << cmd;
      if (param.length() > 0) std::cout << " (" << param << ")";
      std::cout << std::endl;
      //SetConnectionState(eudaq::ConnectionState::LVL_WARN, "Received Unrecognized Command");
    }
  std::string GetConfigParameter(const std::string & item){
    if (!m_config) return std::string();
    return m_config->Get(item,std::string());
  }
  bool SetConfigured(){
    if (m_internalstate!=Configuring) return false;    
    EUDAQ_INFO("Configured (" + m_config->Name() + ")");
    m_internalstate=Configured;
    return true;
  }
  bool SetReset(){
    if (m_internalstate!=Resetting) return false;
    m_config = NULL;
    m_run = 0;
    m_evt = 0;
    m_internalstate = Init;
    return true;
  }
  void SetError(){
    m_internalstate = Error;
    SetConnectionState(eudaq::ConnectionState::LVL_ERROR);
  }
  bool SendBORE(){
    if (m_internalstate!=StartingRun) return false;
    std::cout << "[PyProducer] Start Run: " << m_run << std::endl;
    m_internalstate = Running;
    return true;
  }
  bool SendEORE(){
    if (m_internalstate!=StoppingRun) return false;
    std::cout << "[PyProducer] Stopping Run: " << m_run << std::endl;
    m_internalstate = Stopped;
    return true;
  }
  bool IsConfiguring(){return (m_internalstate == Configuring);}
  bool IsStartingRun(){return (m_internalstate == StartingRun);}
  bool IsStoppingRun(){return (m_internalstate == StoppingRun);}
  bool IsTerminating(){return (m_internalstate == Terminating);}
  bool IsResetting  (){return (m_internalstate == Resetting);}
  bool IsError      (){return (m_internalstate == Error);}
private:
  enum PyState {Init, Configuring, Configured, StartingRun, Running, StoppingRun, Stopped, Terminating, Resetting, Error};
  PyState m_internalstate; 
  std::string m_name;
  unsigned m_run, m_evt;
  eudaq::Configuration * m_config;
};

// ctypes can only talk to C functions -- need to provide them through 'extern "C"'
extern "C" {
  DLLEXPORT PyProducer* PyProducer_new(char *name, char *rcaddress){return new PyProducer(std::string(name),std::string(rcaddress));}
  // functions for I/O
  DLLEXPORT void PyProducer_SendEvent(PyProducer *pp, uint8_t* buffer, size_t size){pp->SendEvent(buffer,size);}
  DLLEXPORT char* PyProducer_GetConfigParameter(PyProducer *pp, char *item){
    std::string value = pp->GetConfigParameter(std::string(item));
    // convert string to char*
    char* rv = (char*) malloc(sizeof(char) * (strlen(value.data())+1));
    strcpy(rv, value.data());
    return rv;
  }
  // functions to report on the current state of the producer
  DLLEXPORT bool PyProducer_IsConfiguring(PyProducer *pp){return pp->IsConfiguring();}
  DLLEXPORT bool PyProducer_IsStartingRun(PyProducer *pp){return pp->IsStartingRun();}
  DLLEXPORT bool PyProducer_IsStoppingRun(PyProducer *pp){return pp->IsStoppingRun();}
  DLLEXPORT bool PyProducer_IsTerminating(PyProducer *pp){return pp->IsTerminating();}
  DLLEXPORT bool PyProducer_IsResetting  (PyProducer *pp){return pp->IsResetting();}
  DLLEXPORT bool PyProducer_IsError      (PyProducer *pp){return pp->IsError();}

  // functions to modify the current state of the producer
  DLLEXPORT bool PyProducer_SetConfigured  (PyProducer *pp){return pp->SetConfigured();}
  DLLEXPORT bool PyProducer_SetReset       (PyProducer *pp){return pp->SetReset();}
  DLLEXPORT void PyProducer_SetError       (PyProducer *pp){pp->SetError();}
  DLLEXPORT bool PyProducer_SendBORE       (PyProducer *pp){return pp->SendBORE();}
  DLLEXPORT bool PyProducer_SendEORE       (PyProducer *pp){return pp->SendEORE();}
}
