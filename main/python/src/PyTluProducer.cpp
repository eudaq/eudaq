#include "eudaq/Configuration.hh"
#include "eudaq/Producer.hh"
#include "eudaq/Logger.hh"
#include "eudaq/StringEvent.hh"
#include "eudaq/TLUEvent.hh" // for the TLU event
#include "eudaq/AidaPacket.hh"
#include "eudaq/Utils.hh"
#include "eudaq/OptionParser.hh"

#include <iostream>
#ifndef WIN32
#include <inttypes.h> /* uint64_t */
#endif

using eudaq::StringEvent;
using eudaq::TLUEvent;
using eudaq::AidaPacket;

class PyTluProducer : public eudaq::Producer {
  public:
    PyTluProducer( const std::string & runcontrol) :
      eudaq::Producer("TLU", runcontrol),
      m_internalstate(Init), 
      m_run(0), 
      m_evt(0), 
      m_config(nullptr)
    {}

    unsigned int GetRunNumber() {
        std::cout << "[PyTluProducer] GetRunNumber "<< m_run<< std::endl;
    	return m_run;
    }
  
    void SendEvent(unsigned event, long timestamp,const std::string & trigger) {
      TLUEvent ev(m_run, event, timestamp);
      ev.SetTag("trigger",trigger);
      eudaq::DataSender::SendEvent(ev);
    }
    void SendEventExtraInfo(unsigned event, long timestamp,const std::string & trigger, const std::string & particles, const std::string & Scaler) {
      TLUEvent ev(m_run, event, timestamp);

      ev.SetTag("trigger",trigger);
      ev.SetTag("PARTICLES",particles);
      ev.SetTag("SCALER",Scaler);
      eudaq::DataSender::SendEvent(ev);
    }

    virtual void OnConfigure(const eudaq::Configuration & param) {
      std::cout << "[PyTluProducer] Received Configuration" << std::endl;
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
        TLUEvent bore(eudaq::TLUEvent::BORE(m_run));
        eudaq::DataSender::SendEvent(bore);
        SetConnectionState(eudaq::ConnectionState::STATE_RUNNING);
      }
    }
    virtual void OnStopRun() {
      std::cout << "[PyTluProducer] Stop Run received" << std::endl;
      SetConnectionState(eudaq::ConnectionState::STATE_RUNNING, "Waiting for Python-side run stop");
      m_internalstate = StoppingRun;
      // now wait until the python side is ready to start and called SendEORE()
      while (m_internalstate != Stopped){
	eudaq::mSleep(100);
      }
      if (m_internalstate == Stopped) {
        eudaq::DataSender::SendEvent(TLUEvent::EORE(m_run, ++m_evt));
        if(m_connectionstate.GetState() != eudaq::ConnectionState::STATE_ERROR)
          SetConnectionState(eudaq::ConnectionState::STATE_CONF);
      }
    }
    virtual void OnTerminate() {
      std::cout << "[PyTluProducer] Terminate received" << std::endl;
      m_internalstate = Terminating;
    }
    virtual void OnReset() {
      std::cout << "[PyTluProducer] Reset received" << std::endl;
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
      std::cout << "[PyTluProducer] Unrecognised: (" << cmd.length() << ") " << cmd;
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

    SetConnectionState(eudaq::ConnectionState::STATE_ERROR);

  }
  bool SendBORE(){
    if (m_internalstate!=StartingRun) return false;
    std::cout << "[PyTluProducer] Start Run: " << m_run << std::endl;
    m_internalstate = Running;
    return true;
  }
  bool SendEORE(){
    if (m_internalstate!=StoppingRun) return false;
    std::cout << "[PyTluProducer] Stopping Run: " << m_run << std::endl;
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
  unsigned m_run, m_evt;
  eudaq::Configuration * m_config;
};

// ctypes can only talk to C functions -- need to provide them through 'extern "C"'
extern "C" {
  DLLEXPORT PyTluProducer* PyTluProducer_new( char *rcaddress){return new PyTluProducer(std::string(rcaddress));}
  // functions for I/O
  DLLEXPORT void PyTluProducer_SendEvent(PyTluProducer *pp, unsigned event, long timestamp, char* trigger){pp->SendEvent(event,timestamp,std::string(trigger));}
  DLLEXPORT void PyTluProducer_SendEventExtraInfo(PyTluProducer *pp, unsigned event, long timestamp, char* trigger, char* particles, char* Scaler){pp->SendEventExtraInfo(event,timestamp,std::string(trigger),std::string(particles),std::string(Scaler));}

  DLLEXPORT int PyTluProducer_GetConfigParameter(PyTluProducer *pp, char *item, char *buf, int buf_len){
    std::string value = pp->GetConfigParameter(std::string(item));
    if(value.empty()) // key not found
    	return -1;
    // Copy to buffer if it fits
    if (buf_len > strlen(value.c_str())){
    	strcpy(buf, value.c_str());
    	return strlen(value.c_str());
    }
    else
    	return 0;

  }

  DLLEXPORT unsigned int PyTluProducer_GetRunNumber(PyTluProducer *pp){return pp->GetRunNumber();}

  // functions to report on the current state of the producer
  DLLEXPORT bool PyTluProducer_IsConfiguring(PyTluProducer *pp){return pp->IsConfiguring();}
  DLLEXPORT bool PyTluProducer_IsStartingRun(PyTluProducer *pp){return pp->IsStartingRun();}
  DLLEXPORT bool PyTluProducer_IsStoppingRun(PyTluProducer *pp){return pp->IsStoppingRun();}
  DLLEXPORT bool PyTluProducer_IsTerminating(PyTluProducer *pp){return pp->IsTerminating();}
  DLLEXPORT bool PyTluProducer_IsResetting  (PyTluProducer *pp){return pp->IsResetting();}
  DLLEXPORT bool PyTluProducer_IsError      (PyTluProducer *pp){return pp->IsError();}

  // functions to modify the current state of the producer
  DLLEXPORT bool PyTluProducer_SetConfigured  (PyTluProducer *pp){return pp->SetConfigured();}
  DLLEXPORT bool PyTluProducer_SetReset       (PyTluProducer *pp){return pp->SetReset();}
  DLLEXPORT void PyTluProducer_SetError       (PyTluProducer *pp){pp->SetError();}
  DLLEXPORT bool PyTluProducer_SendBORE       (PyTluProducer *pp){return pp->SendBORE();}
  DLLEXPORT bool PyTluProducer_SendEORE       (PyTluProducer *pp){return pp->SendEORE();}
}
