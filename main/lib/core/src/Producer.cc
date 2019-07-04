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
      DoInitialise();
      CommandReceiver::OnInitialise();
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
      if(!IsStatus(Status::STATE_UNCONF)&& !IsStatus(Status::STATE_CONF) && !IsStatus(Status::STATE_STOPPED))
    EUDAQ_THROW("OnConfigure can not be called unless in STATE_UNCONF or STATE_CONF or STATE_STOPPED");
      SetStatus(Status::STATE_UNCONF, "Configuring");
      auto conf = GetConfiguration();
      if(!conf)
	EUDAQ_THROW("No Configuration Section for OnConfigure");
      m_pdc_n = conf->Get("EUDAQ_ID", m_pdc_n);
      DoConfigure();
      CommandReceiver::OnConfigure();
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
      if(!IsStatus(Status::STATE_CONF) && !IsStatus(Status::STATE_STOPPED))
	EUDAQ_THROW("OnStartRun can not be called unless in STATE_CONF");
      std::map<std::string, std::shared_ptr<DataSender>> senders;
      std::string dc_str = GetConfiguration()->Get("EUDAQ_DC", "");
      std::vector<std::string> col_dc_name = split(dc_str, ";,", true);
      std::string cur_backup = GetConfiguration()->GetCurrentSectionName();
      GetConfiguration()->SetSection("");
      for(auto &dc_name: col_dc_name){
	std::string dc_addr =  GetConfiguration()->Get("DataCollector."+dc_name, "");
	if(!dc_addr.empty()){
	  senders[dc_addr]
	    = std::unique_ptr<DataSender>(new DataSender("Producer", GetName()));
	  senders[dc_addr]->Connect(dc_addr);
	}
      }
      GetConfiguration()->SetSection(cur_backup);
      std::unique_lock<std::mutex> lk(m_mtx_sender);
      m_senders = senders;
      lk.unlock();
      m_evt_c = 0;
      SetStatusTag("EventN", "0");
      DoStartRun();
      CommandReceiver::OnStartRun();
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
      DoStopRun();      
      CommandReceiver::OnStopRun();
      std::unique_lock<std::mutex> lk(m_mtx_sender);
      m_senders.clear();
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
      CommandReceiver::OnReset();
      std::unique_lock<std::mutex> lk(m_mtx_sender);
      m_senders.clear();
    } catch (const std::exception &e) {
      printf("Producer Reset:: Caught exception: %s\n", e.what());
      SetStatus(Status::STATE_ERROR, "Reset Error");
    } catch (...){
      printf("Producer Reset:: Unknown exception\n");
      SetStatus(Status::STATE_ERROR, "Reset Error");
    }
  }
  
  void Producer::OnTerminate(){
    EUDAQ_INFO(GetFullName() + " is to be terminated...");
    try{
      DoTerminate();
      CommandReceiver::OnTerminate();
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
    std::unique_lock<std::mutex> lk(m_mtx_sender);
    auto senders = m_senders; //hold on the ptrs
    lk.unlock();
    for(auto &e: senders){
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

