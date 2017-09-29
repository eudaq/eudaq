#include "eudaq/TransportClient.hh"
#include "eudaq/BufferSerializer.hh"
#include "eudaq/Configuration.hh"
#include "eudaq/Exception.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Utils.hh"
#include "eudaq/CommandReceiver.hh"
#include <iostream>
#include <ostream>

#define CHECK_RECIVED_PACKET(packet,position,expectedString) \
  if(packet[position] != std::string(expectedString))\
   EUDAQ_THROW("Invalid response from RunControl server. Expected: " + std::string(expectedString) + "  recieved: " +packet[position])

#define CHECK_FOR_REFUSE_CONNECTION(packet,position,expectedString) \
  if(packet[position] != std::string(expectedString))\
   EUDAQ_THROW("Connection refused by RunControl server: " + packet[position])

namespace eudaq {
  
  TransportClient* make_client(const std::string & runcontrol, const std::string & type, const std::string & name) {
    TransportClient* ret =  TransportClient::CreateClient(runcontrol);
    if (!ret->IsNull()) {
      std::string packet;
      if (!ret->ReceivePacket(&packet, 1000000)) EUDAQ_THROW("No response from RunControl server");
      // check packet is OK ("EUDAQ.CMD.RunControl nnn")
      auto splitted = split(packet, " ");
      if (splitted.size() < 4) {
	EUDAQ_THROW("Invalid response from RunControl server: '" + packet + "'");
      }
      CHECK_RECIVED_PACKET(splitted, 0, "OK");
      CHECK_RECIVED_PACKET(splitted, 1, "EUDAQ");
      CHECK_RECIVED_PACKET(splitted, 2, "CMD");
      CHECK_RECIVED_PACKET(splitted, 3, "RunControl");
      ret->SendPacket("OK EUDAQ CMD " + type + " " + name);
      packet = "";
      if (!ret->ReceivePacket(&packet, 1000000)) EUDAQ_THROW("No response from RunControl server");

      auto splitted_res = split(packet, " ");
      CHECK_FOR_REFUSE_CONNECTION(splitted_res, 0, "OK");
      return ret;
    } else {
      EUDAQ_THROW("Could not create client for: '" + runcontrol + "'");
      return ret;
    }
  }

  CommandReceiver::CommandReceiver(const std::string & type, const std::string & name,
				   const std::string & runcontrol)
    : m_type(type), m_name(name), m_exit(false), m_exited(false){
    uint64_t addr = static_cast<uint64_t>(reinterpret_cast<std::uintptr_t>(this));
    m_cmdrcv_id = static_cast<uint32_t>((addr>>32)+(addr<<32)+addr);
    int i = 0;
    while (true){
      try {
	m_cmdclient = std::unique_ptr<TransportClient>( TransportClient::CreateClient(runcontrol));
	if (!m_cmdclient->IsNull()) {
	  std::string packet;
	  if (!m_cmdclient->ReceivePacket(&packet, 1000000)) EUDAQ_THROW("No response from RunControl server");
	  auto splitted = split(packet, " ");
	  if (splitted.size() < 5) {
	    EUDAQ_THROW("Invalid response from RunControl server: '" + packet + "'");
	  }
	  CHECK_RECIVED_PACKET(splitted, 0, "OK");
	  CHECK_RECIVED_PACKET(splitted, 1, "EUDAQ");
	  CHECK_RECIVED_PACKET(splitted, 2, "CMD");
	  CHECK_RECIVED_PACKET(splitted, 3, "RunControl");
	  m_cmdclient->SendPacket("OK EUDAQ CMD " + type + " " + name);
	  m_addr_client = splitted[4];
	  packet = "";
	  if (!m_cmdclient->ReceivePacket(&packet, 1000000)) EUDAQ_THROW("No response from RunControl server");

	  auto splitted_res = split(packet, " ");
	  CHECK_FOR_REFUSE_CONNECTION(splitted_res, 0, "OK");
	}
	break;
      } catch (...) {
	std::cout << "easdasdasd\n";
	if (++i>10){
	  throw;
	}
      }
    }
    m_cmdclient->SetCallback(TransportCallback(this, &CommandReceiver::CommandHandler));
  }

  CommandReceiver::~CommandReceiver(){
    CloseCommandReceiver();
  }
  
  void CommandReceiver::SetStatus(Status::State state,
                                  const std::string &info) {
    Status::Level level;
    if(state == Status::STATE_ERROR)
      level = Status::LVL_ERROR;
    else
      level = Status::LVL_OK;

    std::unique_lock<std::mutex> lk(m_mtx_status);
    m_status.ResetStatus(state, level, info);
  }
  
  void CommandReceiver::SetStatusTag(const std::string &key, const std::string &val){
    std::unique_lock<std::mutex> lk(m_mtx_status);
    m_status.SetTag(key, val);
  }

  bool CommandReceiver::IsStatus(Status::State state){
    std::unique_lock<std::mutex> lk(m_mtx_status);
    return m_status.GetState() == state;
  }
  
  void CommandReceiver::OnLog(const std::string &param) {
    EUDAQ_LOG_CONNECT(m_type, m_name, param);
  }

  void CommandReceiver::OnIdle() { mSleep(500); }

  void CommandReceiver::CommandHandler(TransportEvent &ev) {
    if (ev.etype == TransportEvent::RECEIVE) {
      std::string cmd = ev.packet;
      std::string param;
      size_t i = cmd.find('\0');
      if (i != std::string::npos) {
        param = std::string(cmd, i + 1);
        cmd = std::string(cmd, 0, i);
      }
      if (cmd == "INIT") {
        std::string section = m_type;
        if(m_name != "")
          section += "." + m_name;
	m_conf_init = std::make_shared<Configuration>(param, section);
	std::stringstream ss;
	m_conf_init->Print(ss, 4);
	EUDAQ_INFO("Receive an INI section\n"+ ss.str());
        OnInitialise();
      } else if (cmd == "CONFIG"){
	std::string section = m_type;
        if(m_name != "")
          section += "." + m_name;
	m_conf = std::make_shared<Configuration>(param, section);
	std::stringstream ss;
	m_conf->Print(ss, 4);
	EUDAQ_INFO("Receive a CONF section\n"+ ss.str());
        OnConfigure();
      } else if (cmd == "START") {
	m_run_number = from_string(param, 0);
        OnStartRun();
      } else if (cmd == "STOP") {
        OnStopRun();
      } else if (cmd == "TERMINATE") {
	m_exit = true;
      } else if (cmd == "RESET") {
        OnReset();
      } else if (cmd == "STATUS") {
        OnStatus();
      } else if (cmd == "LOG") {
        OnLog(param);
      } else {
        OnUnrecognised(cmd, param);
      }
      BufferSerializer ser;
      std::unique_lock<std::mutex> lk(m_mtx_status);
      m_status.Serialize(ser);
      // m_status.ResetTags();
      lk.unlock();
      m_cmdclient->SendPacket(ser);
    }
  }

  void CommandReceiver::ProcessingCommand(){
    try {
      //TODO: create m_cmdclient here instead of inside constructor
      while (!m_exit){
	m_cmdclient->Process(-1);
	OnIdle();
      }
      //TODO: SendDisconnect event;
      OnTerminate();
      m_exited = true;
    } catch (const std::exception &e) {
      std::cout <<"CommandReceiver::ProcessThread() Error: Uncaught exception: " <<e.what() <<std::endl;
      std::this_thread::sleep_for(std::chrono::seconds(3));
      OnTerminate();
      m_exited = true;
    } catch (...) {
      std::cout <<"CommandReceiver::ProcessThread() Error: Uncaught unrecognised exception" <<std::endl;
      std::this_thread::sleep_for(std::chrono::seconds(3));
      OnTerminate();
      m_exited = true;
    }
  }

  void CommandReceiver::StartCommandReceiver(){
    if(m_exit){
      EUDAQ_THROW("CommandReceiver can not be restarted after exit. (TODO)");
    }
    m_thd_client = std::thread(&CommandReceiver::ProcessingCommand, this);
  }

  void CommandReceiver::CloseCommandReceiver(){
    m_exit = true;
    if(m_thd_client.joinable()){
      m_thd_client.join();
    }
  }
  
}
