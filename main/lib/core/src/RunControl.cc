#include "eudaq/RunControl.hh"
#include "eudaq/TransportServer.hh"
#include "eudaq/Configuration.hh"
#include "eudaq/BufferSerializer.hh"
#include "eudaq/Exception.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Logger.hh"

#include <iostream>
#include <ostream>
#include <fstream>

namespace eudaq {
  
  template class DLLEXPORT Factory<RunControl>;
  template DLLEXPORT
  std::map<uint32_t, typename Factory<RunControl>::UP_BASE (*)(const std::string&)>&
  Factory<RunControl>::Instance<const std::string&>();

  namespace{
    auto dummy0 = Factory<RunControl>::
      Register<RunControl, const std::string&>(RunControl::m_id_factory);
  }
  
  RunControl::RunControl(const std::string &listenaddress)
      : m_exit(false), m_listening(true), m_run_n(0){
    std::time_t time_now = std::time(nullptr);
    char time_buff[10];
    time_buff[9] = 0;
    std::strftime(time_buff, sizeof(time_buff), "%V%u%H%M%S", std::localtime(&time_now));
    std::string time_str(time_buff);
    m_run_n = std::stoul(time_str);
    if(listenaddress != ""){
      m_cmdserver.reset(TransportServer::CreateServer(listenaddress));
      m_cmdserver->SetCallback(TransportCallback(this, &RunControl::CommandHandler));
    }
  }

  void RunControl::Initialise(){
    std::vector<ConnectionSPC> conn_to_init; 
    std::unique_lock<std::mutex> lk(m_mtx_conn);
    for(auto &conn_st: m_conn_status){
      auto &conn = conn_st.first;
      auto st = conn_st.second->GetState();
      if(st != Status::STATE_UNINIT && st != Status::STATE_UNCONF){
      	EUDAQ_ERROR(conn->GetName()+" is not Status::STATE_UNINIT OR Status::STATE_UNCONF");
	//TODO:: EUDAQ_THROW
	return;
      }
      else if(st == Status::STATE_UNINIT){
	conn_to_init.push_back(conn);
      }
    }
    lk.unlock();

    m_conf_init->SetSection("");
    std::string log_addr;
    for(auto &conn: conn_to_init){
      std::string conn_type = conn->GetType();
      std::string conn_name = conn->GetName();
      std::string conn_addr = conn->GetRemote();
      if(conn_type == "DataCollector" || conn_type == "LogCollector" || conn_type == "Monitor"){
	lk.lock();
	std::string server_addr = m_conn_status[conn]->GetTag("_SERVER");
	lk.unlock();
	if(server_addr.find("tcp://") == 0 && conn_addr.find("tcp://") == 0){
	  server_addr = conn_addr.substr(0, conn_addr.find_last_not_of("0123456789"))
	    + ":"
	    + server_addr.substr(server_addr.find_last_not_of("0123456789")+1);
	}
	std::string server_name = conn_type+"."+conn_name;
	m_conf_init->SetString(server_name, server_addr);
	if(server_name=="LogCollector.log")
	  log_addr=server_addr;
      }
    }
    if(!log_addr.empty())
      SendCommand("LOG", log_addr);
    m_conf_init->SetSection("RunControl"); //TODO: RunControl section must exist
    for(auto &conn: conn_to_init)
      SendCommand("INIT", to_string(*m_conf_init), conn);
  }
  
  void RunControl::Configure(){
    std::vector<ConnectionSPC> conn_to_conf;
    std::unique_lock<std::mutex> lk(m_mtx_conn);
    for(auto &conn_st: m_conn_status){
      auto &conn = conn_st.first;
      auto st = conn_st.second->GetState();
      if(st != Status::STATE_UNCONF && st != Status::STATE_CONF){
	EUDAQ_ERROR(conn->GetName()+" is not Status::STATE_UNCONF OR Status::STATE_CONF, skipped");
	//TODO:: EUDAQ_THROW
      }
      else{
	conn_to_conf.push_back(conn);
      }
    }
    lk.unlock();
    m_listening = false;

    for(auto &conn: conn_to_conf)
      SendCommand("CONFIG", to_string(*m_conf), conn);
  }

  void RunControl::ReadConfigureFile(const std::string &path){
    m_conf = Configuration::MakeUniqueReadFile(path);
    m_conf->SetSection("RunControl");
  }
  
  void RunControl::ReadInitilizeFile(const std::string &path){
    m_conf_init = Configuration::MakeUniqueReadFile(path);
    m_conf_init->SetSection("RunControl");
  }
  
  void RunControl::Reset() {
    m_listening = true;
    EUDAQ_INFO("Resetting");
    SendCommand("RESET", "");
  }

  void RunControl::StartRun(){
    m_listening = false;
    EUDAQ_INFO("Starting Run " + to_string(m_run_n));
    std::vector<ConnectionSPC> conn_to_run;
    std::unique_lock<std::mutex> lk(m_mtx_conn);
    for(auto &conn_st: m_conn_status){
      auto &conn = conn_st.first;
      auto st = conn_st.second->GetState();
      if(st != Status::STATE_CONF){
	EUDAQ_ERROR(conn->GetName()+" is not Status::STATE_CONF, skipped");
	//TODO:: EUDAQ_THROW
      }
      else{
	conn_to_run.push_back(conn);
      }
    }
    lk.unlock();
    
    for(auto &conn :conn_to_run){
      if(conn->GetType() != "Producer" && conn->GetType() != "DataCollector"){
	SendCommand("START", to_string(m_run_n), conn);
	while(1){
	  int n = 0;
	  n++;
	  std::this_thread::sleep_for(std::chrono::milliseconds(100));
	  auto st = GetConnectionStatus(conn);
	  if(st && st->GetState() == Status::STATE_RUNNING)
	    break;
	  if(n>30)
	    EUDAQ_ERROR("Timesout waiting running status");
	}
      }
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));
    for(auto &conn :conn_to_run){
      if(conn->GetType() == "DataCollector"){
	SendCommand("START", to_string(m_run_n), conn);
	while(1){
	  int n = 0;
	  n++;
	  std::this_thread::sleep_for(std::chrono::milliseconds(100));
	  auto st = GetConnectionStatus(conn);
	  if(st && st->GetState() == Status::STATE_RUNNING)
	    break;
	  if(n>30)
	    EUDAQ_ERROR("Timesout waiting running status");
	}
      }
    }
    
    //TODO: make sure datacollector is started before producer. waiting
    std::this_thread::sleep_for(std::chrono::seconds(1));
    for(auto &conn :conn_to_run){
      if(conn->GetType() == "Producer"){
	SendCommand("START", to_string(m_run_n), conn);
      }
    }
  }

  void RunControl::StopRun(){
    m_listening = true;
    EUDAQ_INFO("Stopping Run " + to_string(m_run_n));
    m_run_n ++;
    std::vector<ConnectionSPC> conn_to_stop;
    std::unique_lock<std::mutex> lk(m_mtx_conn);
    for(auto &conn_st: m_conn_status){
      auto &conn = conn_st.first;
      auto st = conn_st.second->GetState();
      if(st != Status::STATE_RUNNING){
	EUDAQ_ERROR(conn->GetName()+" is not Status::STATE_RUNNING, skipped");
	//TODO:: EUDAQ_THROW
      }
      else{
	conn_to_stop.push_back(conn);
      }
    }
    lk.unlock();

    for(auto &conn :conn_to_stop){
      if(conn->GetType() == "Producer"){
	SendCommand("STOP", "", conn);
      }
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));
    for(auto &conn :conn_to_stop){
      if(conn->GetType() == "DataCollector"){
	SendCommand("STOP", "", conn);
      }
    }

    //TODO: make sure datacollector is stoped after producer. waiting. check status
    std::this_thread::sleep_for(std::chrono::seconds(1));
    for(auto &conn :conn_to_stop){
      if(conn->GetType() != "Producer" && conn->GetType() != "DataCollector"){
	SendCommand("STOP", "", conn);
      }
    }
  }

  void RunControl::Terminate() {
    m_listening = false;
    EUDAQ_INFO("Terminating connections");
    SendCommand("TERMINATE", "");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    CloseRunControl();
  }

  void RunControl::SendCommand(const std::string &cmd, const std::string &param,
                               ConnectionSPC id){
    std::unique_lock<std::mutex> lk(m_mtx_sendcmd);    
    std::string packet(cmd);
    if(param.length() > 0) {
      packet += '\0' + param;
    }
    if(id)
      m_cmdserver->SendPacket(packet, *id);
    else
      m_cmdserver->SendPacket(packet, ConnectionInfo::ALL);
  }

  void RunControl::CommandThread() {
    while (!m_exit) {
      m_cmdserver->Process(100000);
    }
  }

  void RunControl::StatusThread(){
    while(!m_exit){
      SendCommand("STATUS", "");
      std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
  }
  
  void RunControl::CommandHandler(TransportEvent &ev){
    std::unique_lock<std::mutex> lk(m_mtx_conn);
    auto con = ev.id;
    switch (ev.etype){
    case(TransportEvent::CONNECT):
      if(m_listening){
	EUDAQ_INFO(std::string("Connect:    ") + con->GetName());
	std::string msg = "OK EUDAQ CMD RunControl " + con->GetRemote();
        m_cmdserver->SendPacket(msg.c_str(), *con, true);
	m_conn_status[con].reset();
      } else {
	EUDAQ_INFO(std::string("Refused connect:    ") + con->GetName());
        m_cmdserver->SendPacket("ERROR EUDAQ CMD Not accepting new connections",
                                *con, true);
        m_cmdserver->Close(*con);
	m_conn_status.erase(con);
      }
      break;
    case (TransportEvent::DISCONNECT):
      DoDisconnect(con);
      m_conn_status.erase(con);
      break;
    case (TransportEvent::RECEIVE):
      if (con->GetState() == 0) { // waiting for identification
        do{// check packet
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
          if (part != "CMD")
            break;
          i0 = i1 + 1;
          i1 = ev.packet.find(' ', i0);
          part = std::string(ev.packet, i0, i1 - i0);
          con->SetType(part);
          i0 = i1 + 1;
          i1 = ev.packet.find(' ', i0);
          part = std::string(ev.packet, i0, i1 - i0);
          con->SetName(part);
	  con->SetState(1); // successfully identified
        }while(false);
	m_cmdserver->SendPacket("OK", *con, true);
	
	DoConnect(con);
      }
      else {
        BufferSerializer ser(ev.packet.begin(), ev.packet.end());
        auto status = std::make_shared<Status>(ser);
	m_conn_status.at(con) = status;
	DoStatus(con, status);
      }
      break;
    default:
      EUDAQ_WARN("Unknown TransportEvent type");
    }
  }

  bool RunControl::IsActiveConnection(ConnectionSPC conn){
    std::unique_lock<std::mutex> lk(m_mtx_conn);
    return m_conn_status.find(conn)==m_conn_status.end()?false:true;
  }

  StatusSPC RunControl::GetConnectionStatus(ConnectionSPC conn){
    std::unique_lock<std::mutex> lk(m_mtx_conn);
    auto it = m_conn_status.find(conn);
    if(it == m_conn_status.end()){
      return StatusSPC();
    }
    else
      return it->second;
  }

  std::vector<ConnectionSPC> RunControl::GetActiveConnections(){
    std::vector<ConnectionSPC> conns;
    std::unique_lock<std::mutex> lk(m_mtx_conn);
    for(auto &conn_status: m_conn_status){
      conns.push_back(conn_status.first);
    }
    return conns;
  }
  
  std::map<ConnectionSPC, StatusSPC> RunControl::GetActiveConnectionStatusMap(){
    std::unique_lock<std::mutex> lk(m_mtx_conn);
    return m_conn_status;
  }
  
  void RunControl::StartRunControl(){
    m_thd_status = std::thread(&RunControl::StatusThread, this);
    m_thd_server = std::thread(&RunControl::CommandThread, this);
  }

  void RunControl::CloseRunControl(){
    m_exit = true;
    if(m_thd_status.joinable())
      m_thd_status.join();
    if(m_thd_server.joinable())
      m_thd_server.join();
  }
  
  void RunControl::Exec(){
    StartRunControl();
    while(IsActiveRunControl()){
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
  }
}
