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
      auto conf = GetInitConfiguration();
      if(conf)
	EUDAQ_INFO("Initializing ...(" + conf->Name() + ")");
      DoInitialise();
      EUDAQ_INFO("Initialized");
      SetStatus(Status::STATE_UNCONF, "Initializd");
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
      auto conf = GetConfiguration();
      if(conf){
	EUDAQ_INFO("Configuring ...("+ conf->Name()+")");
	m_pdc_n = conf->Get("EUDAQ_ID", m_pdc_n);
      }
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
      EUDAQ_INFO("Start Run: "+ std::to_string(GetRunNumber()));
      m_evt_c = 0;
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
      EUDAQ_INFO("Stopping Run");
      DoStopRun();
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
      m_senders.clear();
      DoReset();
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
  
  void Producer::OnData(const std::string &server){
    auto it = m_senders.find(server);
    if(it==m_senders.end()){
      m_senders[server]= std::unique_ptr<DataSender>(new DataSender("Producer", GetFullName()));
    }
    m_senders[server]->Connect(server);
  }
  
  void Producer::Exec(){
    StartCommandReceiver();
    while(IsActiveCommandReceiver()){
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
  }

  void Producer::SendEvent(EventUP ev){
    if(ev->IsBORE()){
      if(GetConfiguration())
	ev->SetTag("EUDAQ_CONFIG", to_string(*GetConfiguration()));
      if(GetInitConfiguration())
	ev->SetTag("EUDAQ_CONFIG_INIT", to_string(*GetInitConfiguration()));
    }else if(ev->IsEORE()){
      //TODO: add summary tag to EOREvent
      ;
    }
    ev->SetRunN(GetRunNumber());
    ev->SetEventN(m_evt_c);
    m_evt_c ++;
    ev->SetStreamN(m_pdc_n);
    EventSP evsp(std::move(ev));
    for(auto &e: m_senders){
      if(e.second)
	e.second->SendEvent(*(evsp.get()));
      else
	EUDAQ_THROW("Producer::SendEvent, using a null pointer of DataSender");
    }
  }
}
