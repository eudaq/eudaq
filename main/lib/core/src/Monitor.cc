#include "eudaq/Monitor.hh"
#include "eudaq/Logger.hh"
#include "eudaq/TransportServer.hh"
#include "eudaq/BufferSerializer.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Utils.hh"
#include <iostream>
#include <ostream>
#include <ctime>
#include <iomanip>
namespace eudaq {
  template class DLLEXPORT Factory<Monitor>;
  template DLLEXPORT std::map<uint32_t, typename Factory<Monitor>::UP_BASE (*)
			      (const std::string&, const std::string&)>&
  Factory<Monitor>::Instance<const std::string&, const std::string&>(); //TODO
  
  Monitor::Monitor(const std::string &name, const std::string &runcontrol)
    :CommandReceiver("Monitor", name, runcontrol){
  }

  void Monitor::DoInitialise(){
  }

  void Monitor::DoConfigure(){
  }

  void Monitor::DoStartRun(){
  }

  void Monitor::DoStopRun(){
  }

  void Monitor::DoReset(){
  }

  void Monitor::DoTerminate(){
  }

  void Monitor::DoStatus(){
  }

  void Monitor::DoReceive(EventSP ev){
  }
    
  void Monitor::SetServerAddress(const std::string &addr){
    m_data_addr = addr;
  }
  
  void Monitor::OnInitialise(){
    EUDAQ_INFO(GetFullName() + " is to be initialised...");
    auto conf = GetConfiguration();
    try{
      m_data_addr = Listen(m_data_addr);
      SetStatusTag("_SERVER", m_data_addr);
      StopListen();
      DoInitialise();
      CommandReceiver::OnInitialise();
    }catch (const Exception &e) {
      std::string msg = "Error when init by " + conf->Name() + ": " + e.what();
      EUDAQ_ERROR(msg);
      SetStatus(Status::STATE_ERROR, msg);
    }
  }
  
  void Monitor::OnConfigure(){
    EUDAQ_INFO(GetFullName() + " is to be configured...");
    auto conf = GetConfiguration();
    try {
      SetStatus(Status::STATE_UNCONF, "Configuring");
      DoConfigure();
      CommandReceiver::OnConfigure();
    }catch (const Exception &e) {
      std::string msg = "Error when configuring by " + conf->Name() + ": " + e.what();
      EUDAQ_ERROR(msg);
      SetStatus(Status::STATE_ERROR, msg);
    }
  }
    
  void Monitor::OnStartRun(){
    EUDAQ_INFO("RUN #" + std::to_string(GetRunNumber()) + " is to be started...");
    try {
      m_data_addr = Listen(m_data_addr);
      SetStatusTag("_SERVER", m_data_addr);
      DoStartRun();
      CommandReceiver::OnStartRun();
    } catch (const Exception &e) {
      std::string msg = "Error preparing for run " + std::to_string(GetRunNumber()) + ": " + e.what();
      EUDAQ_ERROR(msg);
      SetStatus(Status::STATE_ERROR, msg);
    }
  }

  void Monitor::OnStopRun(){
    EUDAQ_INFO("RUN #" + std::to_string(GetRunNumber()) + " is to be stopped...");
    try {
      DoStopRun();
      StopListen();
      CommandReceiver::OnStopRun();
    } catch (const Exception &e) {
      std::string msg = "Error stopping for run " + std::to_string(GetRunNumber()) + ": " + e.what();
      EUDAQ_ERROR(msg);
      SetStatus(Status::STATE_ERROR, msg);
    }
  }

  void Monitor::OnReset(){
    EUDAQ_INFO(GetFullName() + " is to be reset...");
    try{
      DoReset();
      StopListen();
      CommandReceiver::OnReset();
    } catch (const std::exception &e) {
      printf("Producer Reset:: Caught exception: %s\n", e.what());
      SetStatus(Status::STATE_ERROR, "Reset Error");
    } catch (...) {
      printf("Producer Reset:: Unknown exception\n");
      SetStatus(Status::STATE_ERROR, "Reset Error");
    }
  }
  
  void Monitor::OnTerminate(){
    EUDAQ_INFO(GetFullName() + " is to be terminated...");
    DoTerminate();
    CommandReceiver::OnTerminate();
  }
    
  void Monitor::OnStatus(){
    // SetStatusTag("EventN", std::to_string(m_evt_c));
    DoStatus();
  }

  void Monitor::OnReceive(ConnectionSPC id, EventSP ev){
    DoReceive(ev);
  }
  
  MonitorSP Monitor::Make(const std::string &code_name,
			  const std::string &run_name,
			  const std::string &runcontrol){
    return Factory<Monitor>::
      MakeShared<const std::string&,const std::string&>
      (eudaq::str2hash(code_name), run_name, runcontrol);
  }

}
