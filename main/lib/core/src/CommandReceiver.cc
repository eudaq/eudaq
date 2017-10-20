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
   EUDAQ_THROW("CommandReceiver: Invalid response from RunControl server. Expected: " + std::string(expectedString) + "  recieved: " +packet[position])

#define CHECK_FOR_REFUSE_CONNECTION(packet,position,expectedString) \
  if(packet[position] != std::string(expectedString))\
   EUDAQ_THROW("CommandReceiver: Connection refused by RunControl server: " + packet[position])

namespace eudaq {
  
  CommandReceiver::CommandReceiver(const std::string & type, const std::string & name,
				   const std::string & runcontrol)
    : m_type(type), m_name(name), m_is_destructing(false), m_is_connected(false), m_is_runlooping(false), m_addr_runctrl(runcontrol){
  }

  CommandReceiver::~CommandReceiver(){
    m_is_destructing = true;
    if(m_fut_deamon.valid()){
      m_fut_deamon.get();
    }
  }
  
  void CommandReceiver::CommandHandler(TransportEvent &ev) {
    if (ev.etype == TransportEvent::RECEIVE) {
      // std::cout<<"Receving >> new cmd"<<std::endl;
      std::string cmd = ev.packet;
      std::string param;
      size_t i = cmd.find('\0');
      // std::cout<<"Receving "<<cmd<<std::endl;
      if (i != std::string::npos) {
	// std::cout<<"Receving >>>>  cmd with param"<<std::endl;
        param = std::string(cmd, i + 1);
        cmd = std::string(cmd, 0, i);
      }
      
      std::unique_lock<std::mutex> lk(m_mx_qu_cmd);
      m_qu_cmd.push(std::make_pair(cmd, param));
      // std::cout<<"Receving >> pushed "<<std::endl;
      m_cv_not_empty.notify_all();
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

  void CommandReceiver::OnInitialise(){
    SetStatus(Status::STATE_UNCONF, "Initialized");
    EUDAQ_INFO(GetFullName() + " is initialised.");
  }
  
  void CommandReceiver::OnConfigure(){
    SetStatus(Status::STATE_CONF, "Configured");
    EUDAQ_INFO(GetFullName() + " is configured.");
  }
  
  void CommandReceiver::OnStartRun(){
    if(m_fut_runloop.valid()){
      EUDAQ_THROW("Last run is not stoped");
    }
    m_is_runlooping = true;
    m_fut_runloop = std::async(std::launch::async, &CommandReceiver::RunLooping, this);
    SetStatus(Status::STATE_RUNNING, "Started");
    EUDAQ_INFO("RUN #" + std::to_string(GetRunNumber()) + " is started.");
  }
  
  void CommandReceiver::OnStopRun(){
    if(m_fut_runloop.valid()){
      m_is_runlooping = false;
      m_fut_runloop.get();
      //TODO: wait and try, error report/throw
    }
    SetStatus(Status::STATE_CONF, "Stopped");
    EUDAQ_INFO("RUN #" + std::to_string(GetRunNumber()) + " is stopped.");
  }
  
  void CommandReceiver::OnReset(){
    SetStatus(Status::STATE_UNINIT, "Reseted");
    EUDAQ_INFO(GetFullName() + " is reset.");
  }

  void CommandReceiver::OnTerminate(){
    SetStatus(Status::STATE_UNINIT, "Terminated");
    EUDAQ_INFO(GetFullName() + " is terminated.");
  }

  void CommandReceiver::OnStatus(){

  }
  
  void CommandReceiver::OnUnrecognised(const std::string & /*cmd*/, const std::string & /*param*/){

  }

  std::string CommandReceiver::GetFullName() const {
    return m_type+"."+m_name;
  }

  std::string CommandReceiver::GetName() const {
    return m_name;
  }

  uint32_t CommandReceiver::GetRunNumber() const {
    return m_run_number;
  }
      
  ConfigurationSPC CommandReceiver::GetConfiguration() const {
    return m_conf;
  }
  
  ConfigurationSPC CommandReceiver::GetInitConfiguration() const {
    return m_conf_init;
  }

  bool CommandReceiver::IsConnected() const{
    return m_is_connected;
  }
  
  bool CommandReceiver::RunLoop(){
    return 0;
  }

  bool CommandReceiver::RunLooping(){
    bool ret = RunLoop();
    // try{
    // }//TODO: excetption handling
    std::chrono::milliseconds t(500);
    while(m_is_runlooping){
      std::this_thread::sleep_for(t);
      //TODO message
    }
    return ret;
  }
  
  bool CommandReceiver::AsyncReceiving(){
    std::cout<<"AsyncReceiving >>>> enter"<<std::endl;
    try{
      while(m_is_connected){
	m_cmdclient->Process(-1); //how long does it wait?
      }
    } catch (const std::exception &e) {
      std::cout <<"CommandReceiver::AsyncReceiving Error: Uncaught exception: " <<e.what() <<std::endl;
    } catch (...) {
      std::cout <<"CommandReceiver::AsyncReceiving Error: Uncaught unrecognised exception" <<std::endl;
    }
  }

  bool CommandReceiver::AsyncForwarding(){
    std::cout<<"AsyncForwding >>>> enter"<<std::endl;
    while(m_is_connected){
      std::unique_lock<std::mutex> lk(m_mx_qu_cmd);
      if(m_qu_cmd.empty()){
	// std::cout<<"AsyncForwding >>>> waiting empty"<<std::endl;
	while(m_cv_not_empty.wait_for(lk, std::chrono::seconds(1))==std::cv_status::timeout){
	  if(!m_is_connected){
	    std::cout<<"AysncForwarding << << << return"<<std::endl;
	    return 0;
	  }
	}
	// std::cout<<"AsyncForwding >>>> waiting end"<<std::endl;
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
    std::cout<<"AysncForwarding << << << return"<<std::endl;
    
    return 0;
  }
  
  std::string CommandReceiver::Connect(){
    if(!m_fut_deamon.valid())
      m_fut_deamon = std::async(std::launch::async, &CommandReceiver::Deamon, this); 
    std::unique_lock<std::mutex> lk_deamon(m_mx_deamon);

    if(m_cmdclient){
      EUDAQ_THROW("command receiver is not closed before");
    }

    
    auto cmdclient = TransportClient::CreateClient(m_addr_runctrl);
    
    cmdclient->SetCallback(TransportCallback(this, &CommandReceiver::CommandHandler));
    
    if (cmdclient->IsNull()){
      m_addr_client = "null";
      m_cmdclient.reset(cmdclient);
      m_is_connected = true;
      m_fut_async_rcv = std::async(std::launch::async, &CommandReceiver::AsyncReceiving, this); 
      m_fut_async_fwd = std::async(std::launch::async, &CommandReceiver::AsyncForwarding, this);
      return m_addr_client;
    }

    auto tp_start_connect = std::chrono::steady_clock::now();
    std::string packet_0_in;
    std::string packet_1_out = "OK EUDAQ CMD " + m_type + " " + m_name;
    std::string packet_2_in;
    while(!cmdclient->ReceivePacket(&packet_0_in, 1000000)){
      if((std::chrono::steady_clock::now()-tp_start_connect) > std::chrono::seconds(2))
	EUDAQ_THROW("CommandReceiver: No response from RunControl server");
    }

    auto splitted = split(packet_0_in, " ");
    if (splitted.size() < 5) {
      EUDAQ_THROW("CommandReceiver: Invalid response from RunControl: '" + packet_0_in + "'");
    }
    CHECK_RECIVED_PACKET(splitted, 0, "OK");
    CHECK_RECIVED_PACKET(splitted, 1, "EUDAQ");
    CHECK_RECIVED_PACKET(splitted, 2, "CMD");
    CHECK_RECIVED_PACKET(splitted, 3, "RunControl");
    std::string addr_client = splitted[4];
    
    cmdclient->SendPacket(packet_1_out);
    
    auto tp_start_confirm = std::chrono::steady_clock::now();
    while(!cmdclient->ReceivePacket(&packet_2_in, 1000000)){
      if((std::chrono::steady_clock::now()-tp_start_confirm) > std::chrono::seconds(3))
	EUDAQ_THROW("CommandReceiver: Timeout for the confirm message from RunControl");
    }
    
    auto splitted_res = split(packet_2_in, " ");
    CHECK_FOR_REFUSE_CONNECTION(splitted_res, 0, "OK");

    m_addr_client = addr_client;
    m_cmdclient.reset(cmdclient);    
    m_is_connected = true;
    m_fut_async_rcv = std::async(std::launch::async, &CommandReceiver::AsyncReceiving, this); 
    m_fut_async_fwd = std::async(std::launch::async, &CommandReceiver::AsyncForwarding, this);
    return m_addr_client;
  }

  bool CommandReceiver::Deamon(){
    std::cout<<"Deamon >>>> enter"<<std::endl;
    while(!m_is_destructing){
      // std::cout<<">>> Deamon Loop"<<std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
      std::chrono::milliseconds t(10);
      std::unique_lock<std::mutex> lk_deamon(m_mx_deamon);
      if(m_is_connected){
	// std::cout<<">>>>> connected Loop"<<std::endl;
	try{
	  if(m_fut_async_rcv.valid()){
	    m_fut_async_rcv.wait_for(t);
	  }
	  if(m_fut_async_fwd.valid()){
	    m_fut_async_fwd.wait_for(t);
	  }
	  // if(m_fut_runloop.valid()){
	  //   m_fut_runloop.wait_for(t); //TODO: use flag which is set by RUNLOOPING
	  //   //TODO:: -> message running; get -> message  
	  // }
	}
	catch(...){
	  EUDAQ_WARN("connection execption from disconnetion");
	}
      }
      else{
	try{
	  std::cout<<">>>>>>>>>exiting"<<std::endl;
	  if(m_fut_async_rcv.valid()){
	    m_fut_async_rcv.get();
	  }
	  if(m_fut_async_fwd.valid()){
	    m_fut_async_fwd.get();
	  }
	  m_cmdclient.reset();
	  //TODO: clear queue
	  std::cout<<">>>>>>>>>exit"<<std::endl;
	}
	catch(...){
	  EUDAQ_WARN("connection execption from disconnetion");
	}
      }
    }
    try{
      std::cout<<">>>>>>>>>exiting--"<<std::endl;
      std::unique_lock<std::mutex> lk_deamon(m_mx_deamon);
      m_is_connected = false;
      if(m_fut_async_rcv.valid()){
	m_fut_async_rcv.get();
      }
      if(m_fut_async_fwd.valid()){
	m_fut_async_fwd.get();
      }
      m_cmdclient.reset();
      std::cout<<">>>>>>>>>exit--"<<std::endl;
    }
    catch(...){
      EUDAQ_WARN("connection execption from disconnetion");
    }
  }
  
}
