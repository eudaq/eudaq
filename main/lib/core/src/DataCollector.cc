#include "eudaq/DataCollector.hh"
#include "eudaq/TransportServer.hh"
#include "eudaq/BufferSerializer.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Utils.hh"
#include <iostream>
#include <ostream>
#include <ctime>
#include <iomanip>
namespace eudaq {

  template class DLLEXPORT Factory<DataCollector>;
  template DLLEXPORT std::map<uint32_t, typename Factory<DataCollector>::UP_BASE (*)
			      (const std::string&, const std::string&)>&
  Factory<DataCollector>::Instance<const std::string&, const std::string&>(); //TODO
  
  DataCollector::DataCollector(const std::string &name, const std::string &runcontrol)
    :CommandReceiver("DataCollector", name, runcontrol){  
    m_dct_n= str2hash(GetFullName());
    m_evt_c = 0;
    m_exit = false;
  }
  
  void DataCollector::OnInitialise(){
    EUDAQ_INFO(GetFullName() + " is to be initialised...");
    auto conf = GetConfiguration();
    try{
      DoInitialise();
      SetStatus(Status::STATE_UNCONF, "Initialized");
      EUDAQ_INFO(GetFullName() + " is initialised.");
    }catch (const Exception &e) {
      std::string msg = "Error when init by " + conf->Name() + ": " + e.what();
      EUDAQ_ERROR(msg);
      SetStatus(Status::STATE_ERROR, msg);
    }
  }
  
  void DataCollector::OnConfigure(){
    EUDAQ_INFO(GetFullName() + " is to be configured...");
    auto conf = GetConfiguration();
    try {
      SetStatus(Status::STATE_UNCONF, "Configuring");
      m_fwtype = conf->Get("EUDAQ_FW", "native");
      m_fwpatt = conf->Get("EUDAQ_FW_PATTERN", "$12D_run$6R$X");
      m_dct_n = conf->Get("EUDAQ_ID", m_dct_n);
      DoConfigure();
      SetStatus(Status::STATE_CONF, "Configured");
      EUDAQ_INFO(GetFullName() + " is configured.");
    }catch (const Exception &e) {
      std::string msg = "Error when configuring by " + conf->Name() + ": " + e.what();
      EUDAQ_ERROR(msg);
      SetStatus(Status::STATE_ERROR, msg);
    }
  }  
  void DataCollector::OnStartRun(){
    EUDAQ_INFO("RUN #" + std::to_string(GetRunNumber()) + " is to be started...");
    try {
      if (!m_dataserver) {
        EUDAQ_THROW("You must configure before starting a run");
      }
      m_writer = Factory<FileWriter>::Create<std::string&>(str2hash(m_fwtype), m_fwpatt);
      m_evt_c = 0;

      std::string mn_str = GetConfiguration()->Get("EUDAQ_MN", "");
      std::vector<std::string> col_mn_name = split(mn_str, ";,", true);
      std::string cur_backup = GetInitConfiguration()->GetCurrentSectionName();
      GetInitConfiguration()->SetSection("");//NOTE: it is m_conf_init
      for(auto &mn_name: col_mn_name){
	std::string mn_addr =  GetInitConfiguration()->Get("Monitor."+mn_name, "");
	if(!mn_addr.empty()){
	  m_senders[mn_addr]
	    = std::unique_ptr<DataSender>(new DataSender("DataCollector", GetName()));
	  m_senders[mn_addr]->Connect(mn_addr);
	}
      }
      
      DoStartRun();
      SetStatus(Status::STATE_RUNNING, "Started");
      EUDAQ_INFO("RUN #" + std::to_string(GetRunNumber()) + " is started.");
    } catch (const Exception &e) {
      std::string msg = "Error preparing for run " + std::to_string(GetRunNumber()) + ": " + e.what();
      EUDAQ_ERROR(msg);
      SetStatus(Status::STATE_ERROR, msg);
    }
  }

  void DataCollector::OnStopRun(){
    EUDAQ_INFO("RUN #" + std::to_string(GetRunNumber()) + " is to be stopped...");
    try {
      DoStopRun();
      m_senders.clear();
      SetStatus(Status::STATE_CONF, "Stopped");
      EUDAQ_INFO("RUN #" + std::to_string(GetRunNumber()) + " is stopped.");
    } catch (const Exception &e) {
      std::string msg = "Error stopping for run " + std::to_string(GetRunNumber()) + ": " + e.what();
      EUDAQ_ERROR(msg);
      SetStatus(Status::STATE_ERROR, msg);
    }
  }

  void DataCollector::OnReset(){
    EUDAQ_INFO(GetFullName() + " is to be reset...");
    try{
      DoReset();
      m_senders.clear();
      SetStatus(Status::STATE_UNINIT, "Reset");
      EUDAQ_INFO(GetFullName() + " is reset.");
    } catch (const std::exception &e) {
      EUDAQ_THROW( std::string("DataCollector Reset:: Caught exception: ") + e.what() );
      SetStatus(Status::STATE_ERROR, "Reset Error");
    } catch (...) {
      EUDAQ_THROW("DataCollector Reset:: Unknown exception\n");
      SetStatus(Status::STATE_ERROR, "Reset Error");
    }
  }

  
  void DataCollector::OnTerminate(){
    EUDAQ_INFO(GetFullName() + " is to be terminated...");
    CloseDataCollector();
    DoTerminate();
    SetStatus(Status::STATE_UNINIT, "Terminated");
    EUDAQ_INFO(GetFullName() + " is terminated.");
  }
    
  void DataCollector::OnStatus(){
    SetStatusTag("EventN", std::to_string(m_evt_c));
    if(m_writer && m_writer->FileBytes()){
      SetStatusTag("FILEBYTES", std::to_string(m_writer->FileBytes()));
    }
  }
  
  void DataCollector::DataHandler(TransportEvent &ev) {
    auto con = ev.id;
    switch (ev.etype) {
    case (TransportEvent::CONNECT):
      m_dataserver->SendPacket("OK EUDAQ DATA DataCollector", *con, true);
      break;
    case (TransportEvent::DISCONNECT):
      EUDAQ_INFO("Disconnected: " + to_string(*con));
      for (size_t i = 0; i < m_info_pdc.size(); ++i) {
	if (m_info_pdc[i] == con){
	  m_info_pdc.erase(m_info_pdc.begin() + i);
	  DoDisconnect(con);
	  return;
	}
      }
      EUDAQ_THROW("Unrecognised connection id");
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
	std::cout<< "DoConnecg con type"<< con->GetType()<<" "<< str2hash(con->GetName())<<std::endl;
	std::cout<< "DoConnecg con name "<< con->GetName()<<" "<< str2hash(con->GetName())<<std::endl;
	m_info_pdc.push_back(con);
	DoConnect(con);
      } else{
        BufferSerializer ser(ev.packet.begin(), ev.packet.end());
	uint32_t id;
	ser.PreRead(id);
	EventUP event = Factory<Event>::MakeUnique<Deserializer&>(id, ser);
	DoReceive(con, std::move(event));
      }
      break;
    default:
      std::cout << "Unknown:    " << *con << std::endl;
    }
  }
  
  void DataCollector::WriteEvent(EventSP ev){
    try{
      if(ev->IsBORE()){
	if(GetConfiguration())
	  ev->SetTag("EUDAQ_CONFIG", to_string(*GetConfiguration()));
	if(GetInitConfiguration())
	  ev->SetTag("EUDAQ_CONFIG_INIT", to_string(*GetInitConfiguration()));
      }
      
      ev->SetRunN(GetRunNumber());
      ev->SetEventN(m_evt_c);
      SetStatusTag("EventN", std::to_string(m_evt_c));
      m_evt_c ++;
      ev->SetStreamN(m_dct_n);
      EventSP evsp(std::move(ev));
      if(m_writer)
	m_writer->WriteEvent(evsp);
      else
	EUDAQ_THROW("FileWriter is not created before writing.");
      for(auto &e: m_senders){
	if(e.second)
	  e.second->SendEvent(*(evsp.get()));
	else
	  EUDAQ_THROW("DataCollector::WriterEvent, using a null pointer of DataSender");
      }
    }catch (const Exception &e) {
      std::string msg = "Exception writing to file: ";
      msg += e.what();
      EUDAQ_ERROR(msg);
      SetStatus(Status::STATE_ERROR, msg);
    }
  }

  void DataCollector::DataThread() {
    try {
      while (!m_exit) {
        m_dataserver->Process(100000);
      }
    } catch (const std::exception &e) {
      std::cout << "Error: Uncaught exception: " << e.what() << "\n"
                << "DataThread is dying..." << std::endl;
      m_exit = true;
    } catch (...) {
      std::cout << "Error: Uncaught unrecognised exception: \n"
                << "DataThread is dying..." << std::endl;
      m_exit = true;
    }
  }

  void DataCollector::StartDataCollector(){
    if(m_exit){
      EUDAQ_THROW("DataCollector can not be restarted after exit. (TODO)");
    }
    if(m_data_addr.empty()){
      m_data_addr = "tcp://";
      uint16_t port = static_cast<uint16_t>(GetCommandReceiverID()) + 1024;
      m_data_addr += to_string(port);
    }
    m_dataserver.reset(TransportServer::CreateServer(m_data_addr));
    m_dataserver->SetCallback(TransportCallback(this, &DataCollector::DataHandler));
    m_thd_server = std::thread(&DataCollector::DataThread, this);
    SetStatusTag("_SERVER", m_data_addr);
  }

  void DataCollector::CloseDataCollector(){
    m_exit = true;
    if(m_thd_server.joinable()){
      m_thd_server.join();
    } 
  }
  
  void DataCollector::Exec(){
    StartDataCollector();
    StartCommandReceiver();
    while(IsActiveCommandReceiver() || IsActiveDataCollector()){
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
  }

  DataCollectorSP DataCollector::Make(const std::string &code_name,
				      const std::string &run_name,
				      const std::string &runcontrol){
    return Factory<DataCollector>::
      MakeShared<const std::string&,const std::string&>
      (eudaq::str2hash(code_name), run_name, runcontrol);
  }

}
