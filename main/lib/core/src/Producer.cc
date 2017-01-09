#include "eudaq/TransportClient.hh"
#include "eudaq/Producer.hh"

namespace eudaq {

  template class DLLEXPORT Factory<Producer>;
  template DLLEXPORT  std::map<uint32_t, typename Factory<Producer>::UP_BASE (*)
			       (const std::string&, const std::string&)>&
  Factory<Producer>::Instance<const std::string&, const std::string&>();  
  
  Producer::Producer(const std::string &name, const std::string &runcontrol)
    : CommandReceiver("Producer", name, runcontrol){
    m_run_n = 0;
    m_evt_c = 0;
    m_pdc_n = str2hash(GetFullName());
  }
  
  void Producer::OnConfigure(const Configuration &conf){
    try{
      SetStatus(eudaq::Status::LVL_OK, "Wait");
      std::cout << "Configuring ...(" << conf.Name() << ")" << std::endl;
      DoConfigure(conf);
      std::cout << "... was Configured " << conf.Name() << " " << std::endl;
      EUDAQ_INFO("Configured (" + conf.Name() + ")");
      m_pdc_n = conf.Get("EUDAQ_ID", m_pdc_n);
      SetStatus(eudaq::Status::LVL_OK, "Configured (" + conf.Name() + ")");
    }catch (const std::exception &e) {
      printf("Caught exception: %s\n", e.what());
      SetStatus(eudaq::Status::LVL_ERROR, "Configuration Error");
    }catch (...) {
      printf("Unknown exception\n");
      SetStatus(eudaq::Status::LVL_ERROR, "Configuration Error");
    }
  }

  void Producer::OnStartRun(uint32_t run_n){
    try{
      SetStatus(eudaq::Status::LVL_OK, "Wait");
      std::cout << "Start Run: " << run_n << std::endl;
      m_run_n = run_n;
      m_evt_c = 0;
      DoStartRun(m_run_n);
      SetStatus(eudaq::Status::LVL_OK, "Started");
    }catch (const std::exception &e) {
      printf("Caught exception: %s\n", e.what());
      SetStatus(eudaq::Status::LVL_ERROR, "Start Error");
    }catch (...) {
      printf("Unknown exception\n");
      SetStatus(eudaq::Status::LVL_ERROR, "Start Error");
    }
  }


  void Producer::OnStopRun(){
    try{
      SetStatus(eudaq::Status::LVL_OK, "Wait");
      std::cout << "Stop Run" << std::endl;
      DoStopRun();
      SetStatus(eudaq::Status::LVL_OK, "Stopped");
    } catch (const std::exception &e) {
      printf("Caught exception: %s\n", e.what());
      SetStatus(eudaq::Status::LVL_ERROR, "Stop Error");
    } catch (...) {
      printf("Unknown exception\n");
      SetStatus(eudaq::Status::LVL_ERROR, "Stop Error");
    }

  }

  void Producer::OnReset(){
    try{
      SetStatus(eudaq::Status::LVL_OK, "Wait");
      m_senders.clear();
      DoReset();
      SetStatus(eudaq::Status::LVL_OK, "Reset");
    } catch (const std::exception &e) {
      printf("Producer Reset:: Caught exception: %s\n", e.what());
      SetStatus(eudaq::Status::LVL_ERROR, "Reset Error");
    } catch (...) {
      printf("Producer Reset:: Unknown exception\n");
      SetStatus(eudaq::Status::LVL_ERROR, "Reset Error");
    }
  };

  
  void Producer::OnTerminate(){
    std::cout << "Terminate...." << std::endl;
    DoTerminate();
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
    //TODO: add config tag to BOREvent
    //TODO: add summary tag to EOREvent
    ev->SetRunN(m_run_n);
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
