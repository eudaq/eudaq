#include "eudaq/Configuration.hh"
#include "eudaq/Producer.hh"
#include "eudaq/Logger.hh"
#include "eudaq/StringEvent.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/AidaPacket.hh"
#include "eudaq/PyPacket.hh"
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
    void SendEvent(uint64_t* data, size_t size) {
      RawDataEvent ev(m_name, m_run, ++m_evt);
        ev.AddBlock(0, data, size);
        eudaq::DataSender::SendEvent(ev);
      }
    void SendPacket( uint64_t* meta_data, size_t meta_data_size, uint64_t* data, size_t data_size ) {
    	AidaPacket packet( AidaPacket::str2type( "-pytest-" ), 0 );
    	for ( int i = 0; i < meta_data_size; i++ )
    		packet.GetMetaData().getArray().push_back( meta_data[i] );
    	packet.SetData( data, data_size );
        eudaq::DataSender::SendPacket(packet);
      }
    void sendPacket() {
    	eudaq::PyPacket * p = eudaq::PyPacket::getNextToSend();
    	if ( p )
    		eudaq::DataSender::SendPacket( *(p->packet) );
      }
    virtual void OnConfigure(const eudaq::Configuration & param) {
      std::cout << "[PyProducer] Received Configuration" << std::endl;
      m_config = new eudaq::Configuration(param);
      m_internalstate = Configuring;
      SetStatus(eudaq::Status::LVL_BUSY, "Waiting for Python-side configure");
      // now wait until the python side has everything configured and called SetConfigured()
      while (m_internalstate != Configured){
	eudaq::mSleep(100);
      }
      if (m_internalstate == Configured) {
	SetStatus(eudaq::Status::LVL_OK, "Configured (" + m_config->Name() + ")");
      }
    }
    virtual void OnStartRun(unsigned param) {
      m_run = param;
      m_evt = 0;
      // check if we are beyond the configuration stage
      if (!(m_internalstate==Configured || m_internalstate==Stopped)) {
	SetStatus(eudaq::Status::LVL_ERROR, "Cannot start run/unconfigured!");
	m_internalstate = Error;
      } else {
	m_internalstate = StartingRun;
	SetStatus(eudaq::Status::LVL_BUSY, "Waiting for Python-side run start");
      }
      // now wait until the python side is ready to start and called SendBORE()
      while (m_internalstate != Running){
	eudaq::mSleep(100);
      }
      if (m_internalstate == Running) {
	eudaq::DataSender::SendEvent(RawDataEvent::BORE(m_name, m_run));
	SetStatus(eudaq::Status::LVL_OK, "");
      }
    }
    virtual void OnStopRun() {
      std::cout << "[PyProducer] Stop Run received" << std::endl;
      SetStatus(eudaq::Status::LVL_BUSY, "Waiting for Python-side run stop");
      m_internalstate = StoppingRun;
      // now wait until the python side is ready to start and called SendEORE()
      while (m_internalstate != Stopped){
	eudaq::mSleep(100);
      }
      if (m_internalstate == Stopped) {
	eudaq::DataSender::SendEvent(RawDataEvent::EORE(m_name, m_run, ++m_evt));
	SetStatus(eudaq::Status::LVL_OK);
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
	SetStatus(eudaq::Status::LVL_OK);
      }
    }
    virtual void OnStatus() {
    }
    virtual void OnUnrecognised(const std::string & cmd, const std::string & param) {
      std::cout << "[PyProducer] Unrecognised: (" << cmd.length() << ") " << cmd;
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
    SetStatus(eudaq::Status::LVL_ERROR);
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
  DLLEXPORT void PyProducer_SendEvent(PyProducer *pp, uint64_t* buffer, size_t size){pp->SendEvent(buffer,size);}
  DLLEXPORT void PyProducer_SendPacket(PyProducer *pp, uint64_t* meta_data, size_t meta_data_size, uint64_t* data, size_t data_size ) {
	  pp->SendPacket( meta_data, meta_data_size, data, data_size );
  }
  DLLEXPORT void PyProducer_sendPacket(PyProducer *pp ) {
	  pp->sendPacket();
  }
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
