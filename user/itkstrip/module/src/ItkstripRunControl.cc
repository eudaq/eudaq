#include "eudaq/RunControl.hh"

class ItkstripRunControl: public eudaq::RunControl{
public:
  ItkstripRunControl(const std::string & listenaddress);
  void DoStatus(eudaq::ConnectionSPC id,
  		std::shared_ptr<const eudaq::Status> status) override;
  void Configure() override;
  void Exec() override;
  static const uint32_t m_id_factory = eudaq::cstr2hash("ItkstripRunControl");

private:
  std::string m_next_conf_path;
  std::string m_check_full_name;
  uint32_t m_max_evn;
  uint32_t m_min_second;
  uint32_t m_flag_goto_stop_run;
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::RunControl>::
    Register<ItkstripRunControl, const std::string&>(ItkstripRunControl::m_id_factory);
}

ItkstripRunControl::ItkstripRunControl(const std::string & listenaddress)
  :RunControl(listenaddress){
  m_flag_goto_stop_run = 0;
  std::cout<< "hi, I'm ItkstripRunControl\n";
}

void ItkstripRunControl::DoStatus(eudaq::ConnectionSPC id,
				std::shared_ptr<const eudaq::Status> status){
  if(id->GetType()+"."+id->GetName() == m_check_full_name){
    std::string ev_str = status->GetTag("EventN");
    uint32_t evn = 0;
    if(ev_str.size())
      evn = atol(ev_str.c_str());
    if(m_max_evn && evn > m_max_evn &&
       status->GetState() == eudaq::Status::STATE_RUNNING){
      m_flag_goto_stop_run = 1;
    }
    else{
      m_flag_goto_stop_run = 0;
    }
  }
}

void ItkstripRunControl::Configure(){
  auto conf = GetConfiguration();
  m_next_conf_path = conf->Get("ITSRC_NEXT_RUN_CONF_FILE", "");  
  m_check_full_name = conf->Get("ITSRC_STOP_RUN_CHECK_FULLNAME", "");
  m_max_evn =  conf->Get("ITSRC_STOP_RUN_MAX_EVENT", 0);
  m_min_second = conf->Get("ITSRC_STOP_RUN_MIN_SECOND", 60);
  std::cout<<"ITSRC_NEXT_RUN_CONF_FILE "<<m_next_conf_path<<std::endl;
  std::cout<<"ITSRC_STOP_RUN_CHECK_FULLNAME "<<m_check_full_name<<std::endl;
  std::cout<<"ITSRC_STOP_RUN_MAX_EVENT "<<m_max_evn<<std::endl;
  std::cout<<"ITSRC_STOP_RUN_MIN_SECOND "<<m_min_second<<std::endl;
  RunControl::Configure();
}

void ItkstripRunControl::Exec(){
  StartRunControl();
  while(IsActiveRunControl()){
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    if(m_flag_goto_stop_run){
      StopRun();
      std::this_thread::sleep_for(std::chrono::seconds(2));
      while(1){
	bool waiting = false;
	auto map_conn_status = GetActiveConnectionStatusMap();
	for(auto &conn_status: map_conn_status){
	  auto state_conn = conn_status.second->GetState();
	  if(state_conn != eudaq::Status::STATE_CONF){
	    waiting = true;
	  }
	}
	if(!waiting)
	  break;
	std::this_thread::sleep_for(std::chrono::milliseconds(500));    
	EUDAQ_INFO("Waiting for end of stop run");
      }
      if(m_next_conf_path.size() == 0){
	continue;
      }
      //TODO: check if file exists
      ReadConfigureFile(m_next_conf_path);
      Configure();
      std::this_thread::sleep_for(std::chrono::seconds(2));
      while(1){
	bool waiting = false;
	auto map_conn_status = GetActiveConnectionStatusMap();
	for(auto &conn_status: map_conn_status){
	  auto state_conn = conn_status.second->GetState();
	  if(state_conn != eudaq::Status::STATE_CONF){
	    waiting = true;
	  }
	}
	if(!waiting)
	  break;
	std::this_thread::sleep_for(std::chrono::milliseconds(500));
	EUDAQ_INFO("Waiting for end of configure");
      }
      StartRun();
      std::this_thread::sleep_for(std::chrono::seconds(m_min_second));
    }
  }
}
