#include "DataCollector.hh"
#include "TransportFactory.hh"
#include "BufferSerializer.hh"
#include "Logger.hh"
#include "Utils.hh"
#include "Processor.hh"
#include "ExportEventPS.hh"
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
    std::string addr_server("tcp://");
    uint16_t port = static_cast<uint16_t>(GetCommandReceiverID()) + 1024;
    addr_server += to_string(port);
    m_dataserver.reset(TransportFactory::CreateServer(addr_server)); //TODO
    m_dataserver->SetCallback(TransportCallback(this, &DataCollector::DataHandler));
  }

  void DataCollector::OnConfigure(){
    auto conf = GetConfiguration();
    std::cout << "Configuring (" << conf->Name() << ")..." << std::endl;
    try {
      std::string fwtype = conf->Get("FileType", "native");
      std::string fwpatt = conf->Get("FilePattern", "run$6R$X");
      m_dct_n = conf->Get("EUDAQ_ID", m_dct_n);
      std::stringstream ss;
      std::time_t time_now = std::time(nullptr);
      char time_buff[12];
      std::strftime(time_buff, sizeof(time_buff), "%y%m%d%H%M%S", std::localtime(&time_now));
      ss<<time_buff<<"_"<<GetName()<<"_"<<fwpatt;
      fwpatt = ss.str();
      m_writer = Factory<FileWriter>::Create<std::string&>(str2hash(fwtype), fwpatt);
      DoConfigure();
      std::cout << "...Configured (" << conf->Name() << ")" << std::endl;
      SetStatus(eudaq::Status::LVL_OK, "Configured (" + conf->Name() + ")");
    }catch (const Exception &e) {
      std::string msg = "Error when configuring by " + conf->Name() + ": " + e.what();
      EUDAQ_ERROR(msg);
      SetStatus(Status::LVL_ERROR, msg);
    }
  }
  
  void DataCollector::OnServer(){
    if (!m_dataserver)
      EUDAQ_THROW("OnServer is called when no dataserver is created");
    std::string addr_server = m_dataserver->ConnectionString();
    std::string addr_cmd = GetCommandRecieverAddress();
    if(addr_server.find("tcp://") == 0 && addr_cmd.find("tcp://") == 0){
      addr_server = addr_cmd.substr(0, addr_cmd.find_last_of(":")) +
	addr_server.substr(addr_server.find_last_of(":"));
    }
    SetStatusTag("_SERVER", addr_server);
  }
  
  void DataCollector::OnStartRun(){
    EUDAQ_INFO("Preparing for run " + std::to_string(GetRunNumber()));
    try {
      if (!m_writer || !m_dataserver) {
        EUDAQ_THROW("You must configure before starting a run");
      }
      m_evt_c = 0;
      m_writer->StartRun(GetRunNumber());
      DoStartRun();
      SetStatus(Status::LVL_OK);
    } catch (const Exception &e) {
      std::string msg = "Error preparing for run " + std::to_string(GetRunNumber()) + ": " + e.what();
      EUDAQ_ERROR(msg);
      SetStatus(Status::LVL_ERROR, msg);
    }
  }

  void DataCollector::OnStopRun(){
    EUDAQ_INFO("End of run ");
    try {
      DoStopRun();
      //Set Status
    } catch (const Exception &e) {
      std::string msg = "Error stopping for run " + std::to_string(GetRunNumber()) + ": " + e.what();
      EUDAQ_ERROR(msg);
      SetStatus(Status::LVL_ERROR, msg);
    }
  }
  
  void DataCollector::OnTerminate(){
    std::cout << "Terminating" << std::endl;
    CloseDataCollector();
    DoTerminate();
  }
    
  void DataCollector::OnStatus() {
    std::cout << "OnStatus" <<std::endl;
    SetStatusTag("EVENT", std::to_string(m_evt_c));
    SetStatusTag("RUN", std::to_string(GetRunNumber()));
    if(m_writer)
      SetStatusTag("FILEBYTES", std::to_string(m_writer->FileBytes()));
  }
  
  void DataCollector::DataHandler(TransportEvent &ev) {
    auto con = ev.id;
    switch (ev.etype) {
    case (TransportEvent::CONNECT):
      m_dataserver->SendPacket("OK EUDAQ DATA DataCollector", *con, true);
      DoConnect(con);
      break;
    case (TransportEvent::DISCONNECT):
      EUDAQ_INFO("Disconnected: " + to_string(*con));
      for (size_t i = 0; i < m_info_pdc.size(); ++i) {
	if (m_info_pdc[i]->Matches(*con)){
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
	m_info_pdc.push_back(con);
      } else {
        BufferSerializer ser(ev.packet.begin(), ev.packet.end());
	uint32_t id;
	ser.PreRead(id);
	EventUP event = Factory<Event>::MakeUnique<Deserializer&>(id, ser);
	//TODO: check if OnStopRun is called. if yes, DO NOT call DoReceive
	//TODO: check if OnStartRun is called.
	DoReceive(con, std::move(event));
      }
      break;
    default:
      std::cout << "Unknown:    " << *con << std::endl;
    }
  }
  
  void DataCollector::WriteEvent(EventUP ev){
    try{
      if(ev->IsBORE()){
	if(GetConfiguration())
	  ev->SetTag("EUDAQ_CONFIG", to_string(*GetConfiguration()));
	if(GetInitConfiguration())
	  ev->SetTag("EUDAQ_CONFIG_INIT", to_string(*GetInitConfiguration()));
      }else if(ev->IsEORE()){
	//TODO: add summary tag to EOREvent
	;
      }
      
      ev->SetRunN(GetRunNumber());
      ev->SetEventN(m_evt_c);
      m_evt_c ++;
      ev->SetStreamN(m_dct_n);
      EventSP evsp(std::move(ev));
      if(m_writer)
	m_writer->WriteEvent(evsp);
      else
	EUDAQ_THROW("FileWriter is not created before writing.");
    }catch (const Exception &e) {
      std::string msg = "Exception writing to file: ";
      msg += e.what();
      EUDAQ_ERROR(msg);
      SetStatus(Status::LVL_ERROR, msg);
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
    } catch (...) {
      std::cout << "Error: Uncaught unrecognised exception: \n"
                << "DataThread is dying..." << std::endl;
    }
  }

  void DataCollector::StartDataCollector(){
    if(m_exit){
      EUDAQ_THROW("DataCollector can not be restarted after exit. (TODO)");
    }
    m_thd_server = std::thread(&DataCollector::DataThread, this);
    std::cout << "###### listenaddress=" << m_dataserver->ConnectionString()
	      << std::endl;
  }

  void DataCollector::CloseDataCollector(){
    m_exit = true;
    if(m_thd_server.joinable()){
      m_thd_server.join();
    }
    
  }
  
  void DataCollector::Exec(){
    StartDataCollector(); //TODO: Start it OnServer
    StartCommandReceiver();
    while(IsActiveCommandReceiver() || IsActiveDataCollector()){
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
  }

}
