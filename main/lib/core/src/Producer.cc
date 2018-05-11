#include "eudaq/TransportClient.hh"
#include "eudaq/Producer.hh"

namespace eudaq {

  template class DLLEXPORT Factory<Producer>;
  template DLLEXPORT  std::map<uint32_t, typename Factory<Producer>::UP_BASE (*)
			       (const std::string&, const std::string&)>&
  Factory<Producer>::Instance<const std::string&, const std::string&>();  
  
  Producer::Producer(const std::string &name, const std::string &runcontrol)
    : CommandReceiver("Producer", name, runcontrol){
    m_evt_c = 0;
    m_pdc_n = str2hash(GetFullName());
    if(!runcontrol.empty())
      m_cli_run=false;
    else
      m_cli_run=true;
  }

  void Producer::OnInitialise(){
    EUDAQ_INFO(GetFullName() + " is to be initialised...");
    try{
      if(!IsStatus(Status::STATE_UNINIT))
	EUDAQ_THROW("OnInitialise can not be called unless in STATE_UNINIT");
      auto conf = GetInitConfiguration();
      if(!conf)
	EUDAQ_THROW("No Configuration Section for OnInitialise");
      EUDAQ_INFO("Initializing ...(" + conf->Name() + ")");
      DoInitialise();
      SetStatus(Status::STATE_UNCONF, "Initialized");
      EUDAQ_INFO(GetFullName() + " is initialised.");
    }catch (const std::exception &e) {
      printf("Caught exception: %s\n", e.what());
      SetStatus(Status::STATE_ERROR, "Init Error");
    }catch (...) {
      printf("Unknown exception\n");
      SetStatus(Status::STATE_ERROR, "Init Error");
    }
  }
  
  void Producer::OnConfigure(){
    EUDAQ_INFO(GetFullName() + " is to be configured...");
    try{
      if(!IsStatus(Status::STATE_UNCONF)&& !IsStatus(Status::STATE_CONF))
	EUDAQ_THROW("OnConfigure can not be called unless in STATE_UNCONF or STATE_CONF");
      SetStatus(Status::STATE_UNCONF, "Configuring");
      auto conf = GetConfiguration();
      if(!conf)
	EUDAQ_THROW("No Configuration Section for OnConfigure");
      m_pdc_n = conf->Get("EUDAQ_ID", m_pdc_n);
      m_fwtype = conf->Get("EUDAQ_FW", "native");
      m_fwpatt = conf->Get("EUDAQ_FW_PATTERN", "$12D_run$6R$X");
      DoConfigure();
      SetStatus(Status::STATE_CONF, "Configured");
      EUDAQ_INFO(GetFullName() + " is configured.");
    }catch (const std::exception &e) {
      printf("Caught exception: %s\n", e.what());
      SetStatus(Status::STATE_ERROR, "Configuration Error");
    }catch (...) {
      printf("Unknown exception\n");
      SetStatus(Status::STATE_ERROR, "Configuration Error");
    }
  }
  
  void Producer::OnStartRun(){
    EUDAQ_INFO("RUN #" + std::to_string(GetRunNumber()) + " is to be started...");
    try{
      if(!IsStatus(Status::STATE_CONF))
	EUDAQ_THROW("OnStartRun can not be called unless in STATE_CONF");
      EUDAQ_INFO("Start Run: "+ std::to_string(GetRunNumber()));
      m_writer = Factory<FileWriter>::Create<std::string&>(str2hash(m_fwtype), m_fwpatt);
      m_evt_c = 0;
      std::string dc_str = GetConfiguration()->Get("EUDAQ_DC", "");
      std::vector<std::string> col_dc_name = split(dc_str, ";,", true);
      std::string cur_backup = GetInitConfiguration()->GetCurrentSectionName();
      GetInitConfiguration()->SetSection("");//NOTE: it is m_conf_init
      for(auto &dc_name: col_dc_name){
	std::string dc_addr =  GetInitConfiguration()->Get("DataCollector."+dc_name, "");
	if(!dc_addr.empty()){
	  m_senders[dc_addr]
	    = std::unique_ptr<DataSender>(new DataSender("Producer", GetName()));
	  m_senders[dc_addr]->Connect(dc_addr);
	}
      }
      GetInitConfiguration()->SetSection(cur_backup);
      SetStatusTag("EventN", std::to_string(m_evt_c));      
      DoStartRun();
      SetStatus(Status::STATE_RUNNING, "Started");
      EUDAQ_INFO("RUN #" + std::to_string(GetRunNumber()) + " is started.");
    }catch (const std::exception &e) {
      printf("Caught exception: %s\n", e.what());
      SetStatus(Status::STATE_ERROR, "Start Error");
    }catch (...) {
      printf("Unknown exception\n");
      SetStatus(Status::STATE_ERROR, "Start Error");
    }
  }

  void Producer::OnStopRun(){
    EUDAQ_INFO("RUN #" + std::to_string(GetRunNumber()) + " is to be stopped...");
    try{
      if(!IsStatus(Status::STATE_RUNNING))
	EUDAQ_THROW("OnStopRun can not be called unless in STATE_RUNNING");
      EUDAQ_INFO("Stopping Run");
      DoStopRun();
      m_senders.clear();
      SetStatus(Status::STATE_CONF, "Stopped");
      EUDAQ_INFO("RUN #" + std::to_string(GetRunNumber()) + " is stopped.");
    } catch (const std::exception &e) {
      printf("Caught exception: %s\n", e.what());
      SetStatus(Status::STATE_ERROR, "Stop Error");
    } catch (...) {
      printf("Unknown exception\n");
      SetStatus(Status::STATE_ERROR, "Stop Error");
    }
  }

  void Producer::OnReset(){
    EUDAQ_INFO(GetFullName() + " is to be reset...");
    try{
      DoReset();
      m_senders.clear();
      SetStatus(Status::STATE_UNINIT, "Reset");
      EUDAQ_INFO(GetFullName() + " is reset.");
    } catch (const std::exception &e) {
      printf("Producer Reset:: Caught exception: %s\n", e.what());
      SetStatus(Status::STATE_ERROR, "Reset Error");
    } catch (...) {
      printf("Producer Reset:: Unknown exception\n");
      SetStatus(Status::STATE_ERROR, "Reset Error");
    }
  }
  
  void Producer::OnTerminate(){
    EUDAQ_INFO(GetFullName() + " is to be terminated...");
    try{
      DoTerminate();
      SetStatus(Status::STATE_UNINIT, "Terminated");
      EUDAQ_INFO(GetFullName() + " is terminated.");
    }catch (const std::exception &e) {
      printf("Caught exception: %s\n", e.what());
      SetStatus(Status::STATE_ERROR, "Terminate Error");
    } catch (...) {
      printf("Unknown exception\n");
      SetStatus(Status::STATE_ERROR, "Terminate Error");
    }
  }
  
  void Producer::Exec(){
    /*    StartCommandReceiver();
    while(IsActiveCommandReceiver()){
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
      }*/
    if(!m_cli_run){
      StartCommandReceiver();
      while(IsActiveCommandReceiver()){
	std::this_thread::sleep_for(std::chrono::milliseconds(500));
      }
    }
    else{
      m_exit=false;
      while(!m_exit){
	std::string pathini;
	std::string pathconfig;
	uint32_t nrun;
	std::string param;
	std::string cmd;
	std::getline(std::cin, cmd);
	size_t i = cmd.find('\0');
	if (i != std::string::npos) {
	  param = std::string(cmd, i + 1);
	  cmd = std::string(cmd,0,i);
	}
	if (cmd == "init") {
	  std::cout << " Path to Initialize file : " << std::endl;
	  std::cin >> pathini;
	  ReadInitializeFile(pathini);
	  //  ReadInitializeFile("Ex0.ini");
	  OnInitialise();
	} else if (cmd == "config"){
	  std::cout << " Path to Configure file : " << std::endl;
	  std::cin >> pathconfig;
	  ReadConfigureFile(pathconfig);
	  //  ReadConfigureFile("Ex0.conf");
	  OnConfigure();
	} else if (cmd == "start") {
	  std::cout << "Please Enter Run Number " << std::endl;
	  std::cin >> nrun; 
	  SetRunN(from_string(param,nrun));
	  OnStartRun();
	} else if (cmd == "stop") {
	  OnStopRun();
	} else if (cmd == "quit") {
	  OnTerminate();
	  m_exit = true;
	} else if (cmd == "reset") {
	  OnReset();
	  std::cout << " Status reset to UnInitialized." << std::endl;
	} else if (cmd == "help") {
	  std::cout << "--- Commands ---\n"
		    << "init [file] Initialize clients (with file 'file')\n"
		    << "config [file] Configure clients (with file 'file')\n"
		    << "reset        Reset\n"
		    << "start [n]    Begin Run (with run number)\n"
		    << "stop        End Run\n"
		    << "quit        Quit\n"
		    << "help        Display this help\n"
		    << "----------------" << std::endl;
	} else if (cmd == "status") {
	  // TODO. 
	} else {
	  OnUnrecognised(cmd, param);
	}
      }
      
    }

  }

  void Producer::SendEvent(EventSP ev){
    if(ev->IsBORE()){
      if(GetConfiguration())
	ev->SetTag("EUDAQ_CONFIG", to_string(*GetConfiguration()));
      if(GetInitConfiguration())
	ev->SetTag("EUDAQ_CONFIG_INIT", to_string(*GetInitConfiguration()));
    }
    ev->SetRunN(GetRunNumber());
    ev->SetEventN(m_evt_c);
    m_evt_c ++;
    ev->SetDeviceN(m_pdc_n);
    EventSP evsp(std::move(ev));
    if(m_cli_run){
    if(m_writer)
      m_writer->WriteEvent(evsp);
    else
      EUDAQ_THROW("FileWriter is not created before writing.");
    }

    for(auto &e: m_senders){
      if(e.second)
	e.second->SendEvent(*(evsp.get()));
      else
	EUDAQ_THROW("Producer::SendEvent, using a null pointer of DataSender");
    }
    SetStatusTag("EventN", std::to_string(m_evt_c));
  }
}
