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
    m_exit = false;
  }
  
  void Monitor::OnInitialise(){    
    auto conf = GetConfiguration();
    try{
      DoInitialise();
      SetStatus(Status::STATE_UNCONF, "Initialized");
    }catch (const Exception &e) {
      std::string msg = "Error when init by " + conf->Name() + ": " + e.what();
      EUDAQ_ERROR(msg);
      SetStatus(Status::STATE_ERROR, msg);
    }
  }
  
  void Monitor::OnConfigure(){
    auto conf = GetConfiguration();
    try {
      SetStatus(Status::STATE_UNCONF, "Configuring");
      DoConfigure();
      SetStatus(Status::STATE_CONF, "Configured");
    }catch (const Exception &e) {
      std::string msg = "Error when configuring by " + conf->Name() + ": " + e.what();
      EUDAQ_ERROR(msg);
      SetStatus(Status::STATE_ERROR, msg);
    }
  }
    
  void Monitor::OnStartRun(){
    EUDAQ_INFO("Preparing for run " + std::to_string(GetRunNumber()));
    try {
      if (!m_dataserver) {
        EUDAQ_THROW("You must configure before starting a run");
      }
      DoStartRun();
      SetStatus(Status::STATE_RUNNING, "Started");
    } catch (const Exception &e) {
      std::string msg = "Error preparing for run " + std::to_string(GetRunNumber()) + ": " + e.what();
      EUDAQ_ERROR(msg);
      SetStatus(Status::STATE_ERROR, msg);
    }
  }

  void Monitor::OnStopRun(){
    EUDAQ_INFO("End of run ");
    try {
      DoStopRun();
      SetStatus(Status::STATE_CONF, "Stopped");
    } catch (const Exception &e) {
      std::string msg = "Error stopping for run " + std::to_string(GetRunNumber()) + ": " + e.what();
      EUDAQ_ERROR(msg);
      SetStatus(Status::STATE_ERROR, msg);
    }
  }

  void Monitor::OnReset(){
    try{
      EUDAQ_INFO("Resetting");
      DoReset();
      SetStatus(Status::STATE_UNINIT, "Reset");
    } catch (const std::exception &e) {
      printf("Producer Reset:: Caught exception: %s\n", e.what());
      SetStatus(Status::STATE_ERROR, "Reset Error");
    } catch (...) {
      printf("Producer Reset:: Unknown exception\n");
      SetStatus(Status::STATE_ERROR, "Reset Error");
    }
  }
  
  void Monitor::OnTerminate(){
    std::cout << "Terminating" << std::endl;
    CloseMonitor();
    DoTerminate();
    SetStatus(Status::STATE_UNINIT, "Terminated");
  }
    
  void Monitor::OnStatus(){
    // SetStatusTag("EventN", std::to_string(m_evt_c));
  }
  
  void Monitor::DataHandler(TransportEvent &ev) {
    auto con = ev.id;
    switch (ev.etype) {
    case (TransportEvent::CONNECT):
      m_dataserver->SendPacket("OK EUDAQ DATA Monitor", *con, true);
      break;
    case (TransportEvent::DISCONNECT):
      EUDAQ_INFO("Disconnected: " + to_string(*con));
      break;
    case (TransportEvent::RECEIVE):
      if (con->GetState() == 0) { // waiting for identification
        do {
          size_t i0 = 0, i1 = ev.packet.find(' ');
          if (i1 == std::string::npos)
            break;
          std::string part(ev.packet, i0, i1);
          if (part != "OK")
            break;
          i0 = i1 + 1;
          i1 = ev.packet.find(' ', i0);
          if (i1 == std::string::npos)
            break;
          part = std::string(ev.packet, i0, i1 - i0);
          if (part != "EUDAQ")
            break;
          i0 = i1 + 1;
          i1 = ev.packet.find(' ', i0);
          if (i1 == std::string::npos)
            break;
          part = std::string(ev.packet, i0, i1 - i0);
          if (part != "DATA")
            break;
          i0 = i1 + 1;
          i1 = ev.packet.find(' ', i0);
          part = std::string(ev.packet, i0, i1 - i0);
          con->SetType(part);
          i0 = i1 + 1;
          i1 = ev.packet.find(' ', i0);
          part = std::string(ev.packet, i0, i1 - i0);
          con->SetName(part);
        } while (false);
        m_dataserver->SendPacket("OK", *con, true);
        con->SetState(1); // successfully identified
	EUDAQ_INFO("Connection from " + to_string(*con));
      } else{
        BufferSerializer ser(ev.packet.begin(), ev.packet.end());
	uint32_t id;
	ser.PreRead(id);
	EventUP event = Factory<Event>::MakeUnique<Deserializer&>(id, ser);
	SetStatusTag("EventN", std::to_string(event->GetEventN()));
	DoReceive(std::move(event));
      }
      break;
    default:
      std::cout << "Unknown:    " << *con << std::endl;
    }
  }
  
  void Monitor::DataThread() {
    try {
      while (!m_exit) {
        m_dataserver->Process(100000);
      }
    } catch (const std::exception &e) {
      std::cout << "Error: Uncaught exception: " << e.what() << "\n"
                << "DataThread is dying..." << std::endl;
    } catch (...) {
      std::cout << "Error: Uncaught unrecognised exception: \n"
                << "DataThread is dying..." << std::endl;
    }
  }

  void Monitor::StartMonitor(){
    if(m_exit){
      EUDAQ_THROW("Monitor can not be restarted after exit. (TODO)");
    }
    if(!m_data_addr.empty()){
      m_data_addr = "tcp://";
      uint16_t port = static_cast<uint16_t>(GetCommandReceiverID()) + 1024;
      m_data_addr += to_string(port);
    }
    m_dataserver.reset(TransportServer::CreateServer(m_data_addr));
    m_dataserver->SetCallback(TransportCallback(this, &Monitor::DataHandler));
    m_thd_server = std::thread(&Monitor::DataThread, this);
    std::cout << "###### listenaddress=" << m_dataserver->ConnectionString()
	      << std::endl;
    SetStatusTag("_SERVER", m_data_addr);
  }

  void Monitor::CloseMonitor(){
    m_exit = true;
    if(m_thd_server.joinable()){
      m_thd_server.join();
    } 
  }
  
  void Monitor::Exec(){
    StartMonitor();
    StartCommandReceiver();
    while(IsActiveCommandReceiver() || IsActiveMonitor()){
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
  }
}
