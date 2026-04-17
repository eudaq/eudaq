#include "eudaq/RunControl.hh"

class HidraRunControl: public eudaq::RunControl{
public:
  HidraRunControl(const std::string & listenaddress);
  void Configure() override;
  void StartRun() override;
  void StopRun() override;
  void Exec() override;
  static const uint32_t m_id_factory = eudaq::cstr2hash("HidraRunControl");

  //void DoStatus(eudaq::ConnectionSPC con, eudaq::StatusSPC st) override;

private:
  uint32_t m_stop_second;
  bool m_flag_running;
  std::chrono::steady_clock::time_point m_tp_start_run;
  //std::map<std::string, std::string> module_state; 
  //std::mutex mtx;
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::RunControl>::
    Register<HidraRunControl, const std::string&>(HidraRunControl::m_id_factory);
}

HidraRunControl::HidraRunControl(const std::string & listenaddress)
  :RunControl(listenaddress){
  m_flag_running = false;
}

void HidraRunControl::StartRun(){
  RunControl::StartRun();
  m_tp_start_run = std::chrono::steady_clock::now();
  m_flag_running = true;
}

void HidraRunControl::StopRun(){
  RunControl::StopRun();
  m_flag_running = false;
}

void HidraRunControl::Configure(){
  auto conf = GetConfiguration();
  m_stop_second = conf->Get("EX0_STOP_RUN_AFTER_N_SECONDS", 0);
  RunControl::Configure();
}

void HidraRunControl::Exec(){
  StartRunControl();
  while(IsActiveRunControl()){
    if(m_flag_running && m_stop_second){
      auto tp_now = std::chrono::steady_clock::now();
      std::chrono::nanoseconds du_ts(tp_now - m_tp_start_run);
      if(du_ts.count()/1000000000>m_stop_second)
	StopRun();
    }
    /*for(auto &p : module_state) {

	    std::lock_guard<std::mutex> lock(mtx);
	    std::cout << "[DEVICE]: " << p.first << " [STATUS]: " << p << std::endl;
    
    }*/	    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
}

//void HidraRunControl::DoStatus(eudaq::ConnectionSPC con, eudaq::StatusSPC st){
//return; } 
/*
	std::lock_guard<std::mutex> lock(mtx);
	module_state[con->GetName()] = st->GetStatus();

}*/

