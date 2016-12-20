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
      : m_exit(false), m_listening(true), m_runnumber(-1),
	m_runsizelimit(0),m_runeventlimit(0){
    if (listenaddress != "") {
      m_cmdserver.reset(TransportFactory::CreateServer(listenaddress));
      m_cmdserver->SetCallback(TransportCallback(this, &RunControl::CommandHandler));
    }
  }

  void RunControl::Configure(const Configuration &config) {
    m_listening = false;
    auto info_client = m_cmdserver->GetConnections();
    for(auto &e: m_addr_data){
      config.SetSection(e.first);
      std::string pdcs_str = config.Get("PRODUCERS", "");
      std::vector<std::string> pdcs = split(pdcs_str, ";:,", true);
      for(auto &pdc :pdcs){
    	if(pdc.empty())
    	  continue;
    	for(auto &info_pdc_sp :info_client){
    	  auto &info_pdc = *info_pdc_sp;
    	  if(info_pdc.GetType() == "Producer" && info_pdc.GetName() == pdc){
    	    SendCommand("DATA", e.second, info_pdc);
    	  }
    	}
      }
      if(pdcs.empty())
      for(auto &info_pdc :info_client){
	if(info_pdc->GetType() == "Producer")
	  SendCommand("DATA", e.second, *info_pdc);  
      }
    }
    SendCommand("CONFIG", to_string(config));
  }

  Configuration RunControl::ReadConfigFile(const std::string &param){
    EUDAQ_INFO("Reading Configure File (" + param + ")");
    Configuration config;
    std::ifstream file(param);
    if (file.is_open()) {
      Configuration c(file);
      config = c;
      config.Set("Name", param);
    } else {
      EUDAQ_ERROR("Unable to open file '" + param + "'");
    }
    return config;
  }
  
  void RunControl::Reset() {
    m_listening = true;
    EUDAQ_INFO("Resetting");
    SendCommand("RESET");
  }

  void RunControl::RemoteStatus() { SendCommand("STATUS"); }

  void RunControl::StartRun(){
    m_listening = false;
    m_runnumber++;
    EUDAQ_INFO("Starting Run " + to_string(m_runnumber));
    SendCommand("START", to_string(m_runnumber));
  }

  void RunControl::StopRun() {
    m_listening = true;
    EUDAQ_INFO("Stopping Run " + to_string(m_runnumber));
    SendCommand("STOP");
  }

  void RunControl::Terminate() {
    m_listening = false;
    EUDAQ_INFO("Terminating connections");
    SendCommand("TERMINATE");
    mSleep(5000);
    CloseRunControl();
  }

  void RunControl::SendCommand(const std::string &cmd, const std::string &param,
                               const ConnectionInfo &id){
    std::unique_lock<std::mutex> lk(m_mtx_sendcmd);    
    std::string packet(cmd);
    if (param.length() > 0) {
      packet += '\0' + param;
    }
    m_cmdserver->SendPacket(packet, id);
  }

  std::string RunControl::SendReceiveCommand(const std::string &cmd,
                                             const std::string &param,
                                             const ConnectionInfo &id) {
    std::unique_lock<std::mutex> lk(m_mtx_sendcmd);
    mSleep(500); // make sure there are no pending replies //TODO: it is unreliable.
    std::string packet(cmd);
    if (param.length() > 0) {
      packet += '\0' + param;
    }
    std::string result;
    m_cmdserver->SendReceivePacket(packet, &result, id);
    return result;
  }

  void RunControl::CommandThread() {
    while (!m_exit) {
      m_cmdserver->Process(100000);
    }
  }

  void RunControl::CommandHandler(TransportEvent &ev) {
    switch (ev.etype) {
    case (TransportEvent::CONNECT):
      if (m_listening) {
	std::cout << "Connect:    " << ev.id << std::endl;
	std::string msg = "OK EUDAQ CMD RunControl " + ev.id.GetRemote();
        m_cmdserver->SendPacket(msg.c_str(), ev.id, true);
      } else {
	std::cout << "Refused connect:    " << ev.id << std::endl;
        m_cmdserver->SendPacket("ERROR EUDAQ CMD Not accepting new connections",
                                ev.id, true);
        m_cmdserver->Close(ev.id);
      }
      break;
    case (TransportEvent::DISCONNECT):
      if(ev.id.GetType() == "LogCollector")
	m_addr_log.clear();
      DoDisconnect(ev.id);
      break;
    case (TransportEvent::RECEIVE):
      if (ev.id.GetState() == 0) { // waiting for identification
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
          ev.id.SetType(part);
          i0 = i1 + 1;
          i1 = ev.packet.find(' ', i0);
          part = std::string(ev.packet, i0, i1 - i0);
          ev.id.SetName(part);
	  // ev.id.SetState(1); // successfully identified
        }while(false);
	ev.id.SetState(1); // successfully identified
	m_cmdserver->SendPacket("OK", ev.id, true);
	if (ev.id.GetType() == "DataCollector"){
	  std::string data_name = ev.id.GetName();
	  if(data_name.empty())
	    data_name="DataCollector";
	  else
	    data_name="DataCollector."+data_name;
	  std::cout<<data_name <<" conneted, ask for server address\n";
	  std::this_thread::sleep_for(std::chrono::seconds(1));
	  Status st = m_cmdserver->SendReceivePacket<Status>("SERVER", ev.id, 1000000);
	  std::string addr_data = st.GetTag("_SERVER");
	  if(addr_data.empty())
	    EUDAQ_THROW("Empty address reported by datacollector");
	  std::cout <<data_name<<" responded: " << addr_data << std::endl;
	  m_addr_data[data_name]=addr_data;
	}
	
        if (ev.id.GetType() == "LogCollector")
          InitLog(ev.id);
	else{
	  if (!m_addr_log.empty())
	    SendCommand("LOG", m_addr_log, ev.id);
	}	
	DoConnect(ev.id);
      }
      else {
        BufferSerializer ser(ev.packet.begin(), ev.packet.end());
        auto status = std::make_shared<Status>(ser);
        if (status->GetLevel() == Status::LVL_BUSY &&
	    ev.id.GetState() == 1) {
          ev.id.SetState(2);
        }
	else if (status->GetLevel() != Status::LVL_BUSY &&
		 ev.id.GetState() == 2) {
          ev.id.SetState(1);
        }
	
        // if (from_string(status->GetTag("RUN"), m_runnumber) == m_runnumber) {
          // ignore status messages that are marked with a previous runnumber
	DoStatus(ev.id, status);
        // }
      }
      break;
    default:
      std::cout << "Unknown:    " << ev.id << std::endl;
    }
  }

  void RunControl::InitLog(const ConnectionInfo &id) {
    if(id.GetType() != "LogCollector")
      EUDAQ_THROW("This connection is not logcollector");
    Status status = m_cmdserver->SendReceivePacket<Status>("SERVER", id, 1000000);
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
