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
    try{
      if(!IsStatus(Status::STATE_UNINIT))
	EUDAQ_THROW("OnInitialise can not be called unless in STATE_UNINIT");
      auto conf = GetInitConfiguration();
      if(!conf)
	EUDAQ_THROW("No Configuration Section for OnInitialise");
      EUDAQ_INFO("Initializing ...(" + conf->Name() + ")");
      DoInitialise();
      EUDAQ_INFO("Initialized");
      SetStatus(Status::STATE_UNCONF, "Initialized");
    }catch (const std::exception &e) {
      printf("Caught exception: %s\n", e.what());
      SetStatus(Status::STATE_ERROR, "Init Error");
    }catch (...) {
      printf("Unknown exception\n");
      SetStatus(Status::STATE_ERROR, "Init Error");
    }
  }
  
  void Producer::OnConfigure(){
    try{
      if(!IsStatus(Status::STATE_UNCONF)&& !IsStatus(Status::STATE_CONF))
	EUDAQ_THROW("OnConfigure can not be called unless in STATE_UNCONF or STATE_CONF");
      SetStatus(Status::STATE_UNCONF, "Configuring");
      auto conf = GetConfiguration();
      if(!conf)
	EUDAQ_THROW("No Configuration Section for OnConfigure");
      EUDAQ_INFO("Configuring ...("+ conf->Name()+")");
      m_pdc_n = conf->Get("EUDAQ_ID", m_pdc_n);
      DoConfigure();
      EUDAQ_INFO("Configured");
      SetStatus(Status::STATE_CONF, "Configured");
    }catch (const std::exception &e) {
      printf("Caught exception: %s\n", e.what());
      SetStatus(Status::STATE_ERROR, "Configuration Error");
    }catch (...) {
      printf("Unknown exception\n");
      SetStatus(Status::STATE_ERROR, "Configuration Error");
    }
  }
  
  void Producer::OnStartRun(){
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
    }catch (const std::exception &e) {
      printf("Caught exception: %s\n", e.what());
      SetStatus(Status::STATE_ERROR, "Start Error");
    }catch (...) {
      printf("Unknown exception\n");
      SetStatus(Status::STATE_ERROR, "Start Error");
    }
  }

  void Producer::OnStopRun(){
    try{
      if(!IsStatus(Status::STATE_RUNNING))
	EUDAQ_THROW("OnStopRun can not be called unless in STATE_RUNNING");
      EUDAQ_INFO("Stopping Run");
      DoStopRun();
      m_senders.clear();
      SetStatus(Status::STATE_CONF, "Stopped");
    } catch (const std::exception &e) {
      printf("Caught exception: %s\n", e.what());
      SetStatus(Status::STATE_ERROR, "Stop Error");
    } catch (...) {
      printf("Unknown exception\n");
      SetStatus(Status::STATE_ERROR, "Stop Error");
    }
  }

  void Producer::OnReset(){
    try{
      EUDAQ_INFO("Resetting");
      DoReset();
      m_senders.clear();
      SetStatus(Status::STATE_UNINIT, "Reset");
    } catch (const std::exception &e) {
      printf("Producer Reset:: Caught exception: %s\n", e.what());
      SetStatus(Status::STATE_ERROR, "Reset Error");
    } catch (...) {
      printf("Producer Reset:: Unknown exception\n");
      SetStatus(Status::STATE_ERROR, "Reset Error");
    }
  }
  
  void Producer::OnTerminate(){
    try{
      EUDAQ_INFO("Terminating");
      DoTerminate();
      SetStatus(Status::STATE_UNINIT, "Terminated");
    }catch (const std::exception &e) {
      printf("Caught exception: %s\n", e.what());
      SetStatus(Status::STATE_ERROR, "Terminate Error");
    } catch (...) {
      printf("Unknown exception\n");
      SetStatus(Status::STATE_ERROR, "Terminate Error");
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
    ev->SetStreamN(m_pdc_n);
    for(auto &e: m_senders){
      if(e.second)
	e.second->SendEvent(*(ev.get()));
      else
	EUDAQ_THROW("Producer::SendEvent, using a null pointer of DataSender");
    }
    SetStatusTag("EventN", std::to_string(m_evt_c));
  }
}
