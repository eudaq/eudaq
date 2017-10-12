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

  void Producer::OnStatus(){
    try{
      SetStatusTag("EventN", std::to_string(m_evt_c));
      DoStatus();
    }catch (const std::exception &e) {
      printf("Caught exception: %s\n", e.what());
      SetStatus(Status::STATE_ERROR, "Status Error");
    } catch (...) {
      printf("Unknown exception\n");
      SetStatus(Status::STATE_ERROR, "Status Error");
    }
  }
  
  void Producer::Exec(){
    StartCommandReceiver();
    while(IsActiveCommandReceiver()){
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
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
    for(auto &e: m_senders){
      if(e.second)
	e.second->SendEvent(ev);
      else
	EUDAQ_THROW("Producer::SendEvent, using a null pointer of DataSender");
    }
  }
  
  ProducerSP Producer::Make(const std::string &code_name,
			    const std::string &run_name,
			    const std::string &runcontrol){
    return Factory<Producer>::
      MakeShared<const std::string&,const std::string&>
      (eudaq::str2hash(code_name), run_name, runcontrol);
  }
}

