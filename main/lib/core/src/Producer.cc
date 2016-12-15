#include "eudaq/TransportClient.hh"
#include "eudaq/Producer.hh"

namespace eudaq {

  template class DLLEXPORT Factory<Producer>;
  template DLLEXPORT  std::map<uint32_t, typename Factory<Producer>::UP_BASE (*)
			       (const std::string&, const std::string&)>&
  Factory<Producer>::Instance<const std::string&, const std::string&>();  
  
  Producer::Producer(const std::string &name, const std::string &runcontrol)
    : CommandReceiver("Producer", name, runcontrol), m_name(name),m_done(false){
    m_run_n = 0;
    m_evt_c = 0;
    m_pdc_n = cstr2hash(name.c_str());
  }
  
  void Producer::OnConfigure(const Configuration &conf){
    try{
      std::cout << "Configuring ...(" << conf.Name() << ")" << std::endl;
      DoConfigure(conf);
      std::cout << "... was Configured " << conf.Name() << " " << std::endl;
      EUDAQ_INFO("Configured (" + conf.Name() + ")");
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
      m_run_n = run_n;
      std::cout << "Start Run: " << m_run_n << std::endl;
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
      std::cout << "Stop Run" << std::endl;
      DoStopRun();
      m_senders.clear();
      SetStatus(eudaq::Status::LVL_OK, "Stopped");
    } catch (const std::exception &e) {
      printf("Caught exception: %s\n", e.what());
      SetStatus(eudaq::Status::LVL_ERROR, "Stop Error");
    } catch (...) {
      printf("Unknown exception\n");
      SetStatus(eudaq::Status::LVL_ERROR, "Stop Error");
    }

  }
  
  void Producer::OnTerminate(){
    std::cout << "Terminate...." << std::endl;
    m_done = true;
    DoTerminate();
  }
  
  void Producer::OnData(const std::string &server){
    auto it = m_senders.find(server);
    if(it==m_senders.end()){
      std::unique_ptr<DataSender> sender(new DataSender("Producer", m_name));
      m_senders[server]= std::move(sender);
    }
    m_senders[server]->Connect(server);
  }

  void Producer::Exec(){
    try {
      while (!m_done){
	Process();
	//TODO: sleep here is needed.
      }
    } catch (const std::exception &e) {
      std::cout <<"Producer::Exec() Error: Uncaught exception: " <<e.what() <<std::endl;
    } catch (...) {
      std::cout <<"Producer::Exec() Error: Uncaught unrecognised exception" <<std::endl;
    }
  }  

  void Producer::SendEvent(EventUP ev){
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
