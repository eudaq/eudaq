#include "eudaq/Producer.hh"
#include "eudaq/Logger.hh"
#include "eudaq/StringEvent.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Utils.hh"
#include "eudaq/OptionParser.hh"
#include <iostream>

using eudaq::StringEvent;
using eudaq::RawDataEvent;

class PyProducer : public eudaq::Producer {
  public:
    PyProducer(const std::string & name, const std::string & runcontrol)
      : eudaq::Producer(name, runcontrol), m_internalstate(Init), m_run(0), m_evt(0), m_config(NULL) {}
    void Event(const std::string & str) {
      StringEvent ev(m_run, ++m_evt, str);
      //ev.SetTag("Debug", "foo");
      SendEvent(ev);
    }
    void RandomEvent(unsigned size) {
      RawDataEvent ev("Test", m_run, ++m_evt);
      std::vector<int> tmp((size+3)/4);
      for (size_t i = 0; i < tmp.size(); ++i) {
        tmp[i] = std::rand();
      }
      ev.AddBlock(0, &tmp[0], size);
      //ev.SetTag("Debug", "foo");
      SendEvent(ev);
    }
    virtual void OnConfigure(const eudaq::Configuration & param) {
      std::cout << "Received Configuration" << std::endl;
      m_config = new eudaq::Configuration(param);
      m_internalstate = Configuring;
      SetStatus(eudaq::Status::LVL_NONE, "Preparing");
    }
    virtual void OnStartRun(unsigned param) {
      m_run = param;
      m_evt = 0;
      // check if we are beyond the configuration stage
      if (m_internalstate==Configured || m_internalstate==Stopped) {
	SetStatus(eudaq::Status::LVL_ERROR, "Cannot start run/unconfigured!");
      } else {
	m_internalstate = StartingRun;
	SetStatus(eudaq::Status::LVL_NONE, "Preparing");
      }
    }
    virtual void OnStopRun() {
      std::cout << "Stop Run received" << std::endl;
      m_internalstate = StoppingRun;
      SetStatus(eudaq::Status::LVL_NONE, "Preparing");
    }
    virtual void OnTerminate() {
      std::cout << "Terminate received" << std::endl;
      m_internalstate = Terminating;
      SetStatus(eudaq::Status::LVL_NONE, "Preparing");
    }
    virtual void OnReset() {
      std::cout << "Reset received" << std::endl;
      m_internalstate = Resetting;
      SetStatus(eudaq::Status::LVL_NONE, "Preparing");
    }
    virtual void OnStatus() {
      //std::cout << "Status - " << m_status << std::endl;
      //SetStatus(eudaq::Status::WARNING, "Only joking");
    }
    virtual void OnUnrecognised(const std::string & cmd, const std::string & param) {
      std::cout << "Unrecognised: (" << cmd.length() << ") " << cmd;
      if (param.length() > 0) std::cout << " (" << param << ")";
      std::cout << std::endl;
      SetStatus(eudaq::Status::LVL_WARN, "Received Unrecognized Command");
    }
  std::string GetConfigParameter(const std::string & item){
    if (!m_config) return std::string();
    return m_config->Get(item,std::string());
  }
  bool SetConfigured(){
    if (m_internalstate!=Configuring) return false;    
    EUDAQ_INFO("Configured (" + m_config->Name() + ")");
    SetStatus(eudaq::Status::LVL_OK, "Configured (" + m_config->Name() + ")");
    m_internalstate=Configured;
    return true;
  }
  bool SetReset(){
    if (m_internalstate!=Resetting) return false;
    m_config = NULL;
    m_run = 0;
    m_evt = 0;
    m_internalstate = Init;
    SetStatus(eudaq::Status::LVL_OK);
    return true;
  }
  void SetError(){
    m_internalstate = Error;
    SetStatus(eudaq::Status::LVL_ERROR);
  }
  bool SendBORE(){
    if (m_internalstate!=StartingRun) return false;
    SendEvent(RawDataEvent::BORE("Test", m_run));
    std::cout << "Start Run: " << m_run << std::endl;
    SetStatus(eudaq::Status::LVL_OK, "");
    m_internalstate = Running;
    return true;
  }
  bool SendEORE(){
    if (m_internalstate!=StoppingRun) return false;
    m_internalstate = Stopped;
    std::cout << "Stopping Run: " << m_run << std::endl;
    SendEvent(RawDataEvent::EORE("Test", m_run, ++m_evt));
    m_internalstate = Stopped;
    return true;
  }
  bool IsConfiguring(){return (m_internalstate == Configuring);}
  bool IsStartingRun(){return (m_internalstate == StartingRun);}
  bool IsStoppingRun(){return (m_internalstate == StoppingRun);}
  bool IsTerminating(){return (m_internalstate == Terminating);}
  bool IsResetting  (){return (m_internalstate == Resetting);}
private:
  enum PyState {Init, Configuring, Configured, StartingRun, Running, StoppingRun, Stopped, Terminating, Resetting, Error};
  PyState m_internalstate; 
  unsigned m_run, m_evt;
  eudaq::Configuration * m_config;
};

// ctypes can only talk to C functions -- need to provide them through 'extern "C"'
extern "C" {
  PyProducer* PyProducer_new(char *name, char *rcaddress){return new PyProducer(std::string(name),std::string(rcaddress));}
  void PyProducer_Event(PyProducer *pp, char *data){pp->Event(std::string(data));}
  void PyProducer_RandomEvent(PyProducer *pp, unsigned size){pp->RandomEvent(size);}
  char* PyProducer_GetConfigParameter(PyProducer *pp, char *item){
    std::string value = pp->GetConfigParameter(std::string(item));
    char* rv = (char*) malloc(sizeof(char) * (strlen(value.data())+1));
    strcpy(rv, value.data());
    return rv;
  }
  bool PyProducer_IsConfiguring(PyProducer *pp){return pp->IsConfiguring();}
  bool PyProducer_IsStartingRun(PyProducer *pp){return pp->IsStartingRun();}
  bool PyProducer_IsStoppingRun(PyProducer *pp){return pp->IsStoppingRun();}
  bool PyProducer_IsTerminating(PyProducer *pp){return pp->IsTerminating();}
  bool PyProducer_IsResetting  (PyProducer *pp){return pp->IsResetting();}

  bool PyProducer_SetConfigured  (PyProducer *pp){return pp->SetConfigured();}
  bool PyProducer_SetReset       (PyProducer *pp){return pp->SetReset();}
  void PyProducer_SetError       (PyProducer *pp){pp->SetError();}
  bool PyProducer_SendBORE       (PyProducer *pp){return pp->SendBORE();}
  bool PyProducer_SendEORE       (PyProducer *pp){return pp->SendEORE();}
}
