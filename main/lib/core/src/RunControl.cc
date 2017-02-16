#include "RunControl.hh"
#include "TransportFactory.hh"
#include "Configuration.hh"
#include "BufferSerializer.hh"
#include "Exception.hh"
#include "Utils.hh"
#include "Logger.hh"

#include <iostream>
#include <ostream>
#include <fstream>

namespace eudaq {

  template class DLLEXPORT Factory<RunControl>;
  template DLLEXPORT
  std::map<uint32_t, typename Factory<RunControl>::UP_BASE (*)(const std::string&)>&
  Factory<RunControl>::Instance<const std::string&>();
  
  RunControl::RunControl(const std::string &listenaddress)
      : m_exit(false), m_listening(true), m_runnumber(0),
	m_runsizelimit(0),m_runeventlimit(0){
    char *top_dir_c = std::getenv("EUDAQ_TOP_DIR");
    if(top_dir_c){
      m_var_file = std::string(top_dir_c)+"/data/runnumber.dat";
    }
    else{
      m_var_file = "../data/runnumber.dat";
    }
    m_runnumber = ReadFromFile(m_var_file.c_str(), 0U)+1;
    if (listenaddress != ""){
      m_cmdserver.reset(TransportFactory::CreateServer(listenaddress));
      m_cmdserver->SetCallback(TransportCallback(this, &RunControl::CommandHandler));
    }
  }

  void RunControl::Initialise(){
    for(auto &e: m_status){
      auto client_state = e.second->GetState();
      if(client_state != Status::STATE_UNINIT && client_state != Status::STATE_UNCONF){
	EUDAQ_ERROR(e.first+" is not Status::STATE_UNINIT OR Status::STATE_UNCONF");
      }
    }
    SendCommand("INIT", "");
  }
  
  void RunControl::Configure(){
    for(auto &e: m_status){
      auto client_state = e.second->GetState();
      if(client_state != Status::STATE_UNCONF && client_state != Status::STATE_CONF){
	EUDAQ_ERROR(e.first+" is not Status::STATE_UNCONF OR Status::STATE_CONF");
      }
    }
    m_listening = false;
    auto info_client = m_cmdserver->GetConnections();
    for(auto &e: m_addr_data){
      m_conf->SetSection(e.first);
      std::string pdcs_str = m_conf->Get("PRODUCERS", "");
      std::vector<std::string> pdcs = split(pdcs_str, ";:,", true);
      for(auto &pdc :pdcs){
    	if(pdc.empty())
    	  continue;
    	for(auto &info_pdc_sp :info_client){
    	  auto &info_pdc = *info_pdc_sp;
    	  if(info_pdc.GetType() == "Producer" && info_pdc.GetName() == pdc){
    	    SendCommand("DATA", e.second, info_pdc_sp);
    	  }
    	}
      }
      if(pdcs.empty())
	for(auto &info_pdc :info_client){
	  if(info_pdc->GetType() == "Producer")
	    SendCommand("DATA", e.second, info_pdc);  
	}
    }
    SendCommand("CONFIG", to_string(*m_conf));    
  }

  void RunControl::ReadConfigureFile(const std::string &path){
    m_conf = Configuration::MakeUniqueReadFile(path);
  }
  
  void RunControl::ReadInitilizeFile(const std::string &path){
    m_conf_init = Configuration::MakeUniqueReadFile(path);
  }
  
  void RunControl::Reset() {
    m_listening = true;
    EUDAQ_INFO("Resetting");
    SendCommand("RESET", "");
  }

  void RunControl::RemoteStatus() { SendCommand("STATUS"); }

  void RunControl::StartRun(){
    for(auto &e: m_status){
      if(e.second->GetState() != Status::STATE_CONF){
	EUDAQ_ERROR(e.first+"is not Status::STATE_CONF");
      }
    }

    m_listening = false;
    WriteToFile(m_var_file.c_str(), m_runnumber);
    EUDAQ_INFO("Starting Run " + to_string(m_runnumber));
    
    auto info_client = m_cmdserver->GetConnections();
    for(auto &client_sp :info_client){
      auto &client = *client_sp;
      if(client.GetType() != "Producer"){
	SendCommand("START", to_string(m_runnumber), client_sp);
	while(1){
	  int n = 0;
	  n++;
	  std::this_thread::sleep_for(std::chrono::milliseconds(100));
	  auto st = m_status[client_sp->GetName()];
	  if(st && st->GetState() == Status::STATE_RUNNING)
	    break;
	  if(n>30)
	    EUDAQ_ERROR("Timesout waiting running status");
	}
      }
    }

    //TODO: make sure datacollector is started before producer. waiting
    std::this_thread::sleep_for(std::chrono::seconds(1));
    for(auto &client_sp :info_client){
      auto &client = *client_sp;
      if(client.GetType() == "Producer"){
	SendCommand("START", to_string(m_runnumber), client_sp);
      }
    }

  }

  void RunControl::StopRun(){
    m_listening = true;
    EUDAQ_INFO("Stopping Run " + to_string(m_runnumber));
    m_runnumber ++;
    auto info_client = m_cmdserver->GetConnections();
    for(auto &client_sp :info_client){
      auto &client = *client_sp;
      if(client.GetType() == "Producer"){
	SendCommand("STOP", "", client_sp);
      }
    }
    //TODO: make sure datacollector is stoped after producer. waiting
    std::this_thread::sleep_for(std::chrono::seconds(1));
    for(auto &client_sp :info_client){
      auto &client = *client_sp;
      if(client.GetType() != "Producer"){
	SendCommand("STOP", "", client_sp);
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
    if (param.length() > 0) {
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

  void RunControl::CommandHandler(TransportEvent &ev) {
    auto con = ev.id;
    switch (ev.etype) {
    case (TransportEvent::CONNECT):
      if (m_listening) {
	std::cout << "Connect:    " << *con << std::endl;
	std::string msg = "OK EUDAQ CMD RunControl " + con->GetRemote();
        m_cmdserver->SendPacket(msg.c_str(), *con, true);
      } else {
	std::cout << "Refused connect:    " << *con << std::endl;
        m_cmdserver->SendPacket("ERROR EUDAQ CMD Not accepting new connections",
                                *con, true);
        m_cmdserver->Close(*con);
      }
      break;
    case (TransportEvent::DISCONNECT):
      if(con->GetType() == "LogCollector")
	m_addr_log.clear();
      DoDisconnect(con);
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
	  // con->SetState(1); // successfully identified
        }while(false);
	con->SetState(1); // successfully identified
	m_cmdserver->SendPacket("OK", *con, true);
	if (con->GetType() == "DataCollector"){
	  std::string data_name = con->GetName();
	  if(data_name.empty())
	    data_name="DataCollector";
	  else
	    data_name="DataCollector."+data_name;
	  std::cout<<data_name <<" conneted, ask for server address\n";
	  Status st = m_cmdserver->SendReceivePacket<Status>("SERVER", *con, 1000000);
	  std::string addr_data = st.GetTag("_SERVER");
	  if(addr_data.empty())
	    EUDAQ_THROW("Empty address reported by datacollector");
	  std::cout <<data_name<<" responded: " << addr_data << std::endl;
	  m_addr_data[data_name]=addr_data;
	}
	
        if (con->GetType() == "LogCollector")
          InitLog(con);
	else{
	  if (!m_addr_log.empty())
	    SendCommand("LOG", m_addr_log, con);
	}	
	DoConnect(con);
      }
      else {
        BufferSerializer ser(ev.packet.begin(), ev.packet.end());
        auto status = std::make_shared<Status>(ser);
	m_status[con->GetName()] = status;
	DoStatus(con, status);
      }
      break;
    default:
      std::cout << "Unknown:    " << *con << std::endl;
    }
  }

  void RunControl::InitLog(ConnectionSPC id) {
    if(id->GetType() != "LogCollector")
      EUDAQ_THROW("This connection is not logcollector");
    Status status = m_cmdserver->SendReceivePacket<Status>("SERVER", *id, 1000000);
    m_addr_log = status.GetTag("_SERVER");
    std::cout << "LogServer responded: " << m_addr_log << std::endl;
    
    if (m_addr_log.empty())
      EUDAQ_THROW("No LogCollector is connected, or invalid response from LogCollector");
    EUDAQ_LOG_CONNECT("RunControl", "", m_addr_log);
    SendCommand("LOG", m_addr_log);
  }

  void RunControl::StartRunControl(){
    m_thd_server = std::thread(&RunControl::CommandThread, this);
    std::cout << "DEBUG: listenaddress=" << m_cmdserver->ConnectionString()
              << std::endl;  
  }

  void RunControl::CloseRunControl(){
    m_exit = true;
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
