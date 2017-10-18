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
  
  CommandReceiver::CommandReceiver(const std::string & type, const std::string & name,
				   const std::string & runcontrol)
    : m_type(type), m_name(name), m_is_destructing(false), m_is_connected(false){
    m_fut_deamon = std::async(std::launch::async, &CommandReceiver::Deamon, this); 
  }

  CommandReceiver::~CommandReceiver(){
    m_is_destructing = true;
    if(m_fut_deamon.valid()){
      m_fut_deamon.get();
    }
  }
  
  void CommandReceiver::CommandHandler(TransportEvent &ev) {
    if (ev.etype == TransportEvent::RECEIVE) {
      std::string cmd = ev.packet;
      std::string param;
      size_t i = cmd.find('\0');
      if (i != std::string::npos) {
        param = std::string(cmd, i + 1);
        cmd = std::string(cmd, 0, i);
	m_qu_cmd.push(std::make_pair(cmd, param));
	std::unique_lock<std::mutex> lk(m_mx_qu_cmd);
	lk.unlock();
	m_cv_not_empty.notify_all();
      }
    }
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
  
  bool CommandReceiver::AsyncReceiving(){
    try{
      while(m_is_connected){
	m_cmdclient->Process(-1); //how long does it wait?
      }
    } catch (const std::exception &e) {
      std::cout <<"CommandReceiver::ProcessThread() Error: Uncaught exception: " <<e.what() <<std::endl;
    } catch (...) {
      std::cout <<"CommandReceiver::ProcessThread() Error: Uncaught unrecognised exception" <<std::endl;
    }
  }

  bool CommandReceiver::AsyncForwarding(){
    while(m_is_connected){
      std::unique_lock<std::mutex> lk(m_mx_qu_cmd);
      if(m_qu_cmd.empty()){
	m_cv_not_empty.wait(lk);
      }
      auto cmd = m_qu_cmd.front().first;
      auto param = m_qu_cmd.front().second;
      m_qu_cmd.pop();
      lk.unlock();
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
      } else if (cmd == "TERMINATE"){
	m_is_destructing = true;
	OnTerminate();
	// TODO:
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
      std::unique_lock<std::mutex> lk_st(m_mtx_status);
      m_status.Serialize(ser);
      // m_status.ResetTags();
      lk_st.unlock();
      m_cmdclient->SendPacket(ser);
    }
    return 0;
  }
  
  std::string CommandReceiver::Connect(const std::string &addr){
    std::unique_lock<std::mutex> lk_deamon(m_mx_deamon);

    if(m_cmdclient){
      EUDAQ_THROW("command receiver is not closed before");
    }
    
    int i = 0;
    while(true){
      try{
	m_cmdclient.reset(TransportClient::CreateClient(addr));
	if (!m_cmdclient->IsNull()){
	  std::string packet;
	  if (!m_cmdclient->ReceivePacket(&packet, 1000000))
	    EUDAQ_THROW("No response from RunControl server");
	  auto splitted = split(packet, " ");
	  if (splitted.size() < 5) {
	    EUDAQ_THROW("Invalid response from RunControl server: '" + packet + "'");
	  }
	  CHECK_RECIVED_PACKET(splitted, 0, "OK");
	  CHECK_RECIVED_PACKET(splitted, 1, "EUDAQ");
	  CHECK_RECIVED_PACKET(splitted, 2, "CMD");
	  CHECK_RECIVED_PACKET(splitted, 3, "RunControl");
	  m_cmdclient->SendPacket("OK EUDAQ CMD " + m_type + " " + m_name);
	  m_addr_client = splitted[4];
	  packet = "";
	  if (!m_cmdclient->ReceivePacket(&packet, 1000000))
	    EUDAQ_THROW("No response from RunControl server");

	  auto splitted_res = split(packet, " ");
	  CHECK_FOR_REFUSE_CONNECTION(splitted_res, 0, "OK");
	}
	m_is_connected = true;    
	break;
      } catch (...) {
	std::cout << "easdasdasd\n";
	if (++i>10){
	  throw;
	}
      }
    }
    m_cmdclient->SetCallback(TransportCallback(this, &CommandReceiver::CommandHandler));    
    m_fut_async_rcv = std::async(std::launch::async, &CommandReceiver::AsyncReceiving, this); 
    m_fut_async_fwd = std::async(std::launch::async, &CommandReceiver::AsyncForwarding, this);
  }

  bool CommandReceiver::Deamon(){
    while(!m_is_destructing){
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
      std::chrono::milliseconds t(10);
      std::unique_lock<std::mutex> lk_deamon(m_mx_deamon);
      if(m_is_connected){
	try{
	  if(m_fut_async_rcv.valid()){
	    m_fut_async_rcv.wait_for(t);
	  }
	  if(m_fut_async_fwd.valid()){
	    m_fut_async_fwd.wait_for(t);
	  }
	}
	catch(...){
	  EUDAQ_WARN("connection execption from disconnetion");
	}
      }
      else{
	try{
	  if(m_fut_async_rcv.valid()){
	    m_fut_async_rcv.get();
	  }
	  if(m_fut_async_fwd.valid()){
	    m_fut_async_fwd.get();
	  }
	  m_cmdclient.reset();
	  //TODO: clear queue
	}
	catch(...){
	  EUDAQ_WARN("connection execption from disconnetion");
	}
      }
    }
    try{
      std::unique_lock<std::mutex> lk_deamon(m_mx_deamon);
      m_is_connected = false;
      if(m_fut_async_rcv.valid()){
	m_fut_async_rcv.get();
      }
      if(m_fut_async_fwd.valid()){
	m_fut_async_fwd.get();
      }
      m_cmdclient.reset();
    }
    catch(...){
      EUDAQ_WARN("connection execption from disconnetion");
    }

  }
  
}
