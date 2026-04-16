#include "eudaq/RunControl.hh"

class PaviaRunControl: public eudaq::RunControl{
public:
  PaviaRunControl(const std::string & listenaddress);
  void Configure() override;
  void StartRun() override;
  void StopRun() override;
  void Exec() override;
  static const uint32_t m_id_factory = eudaq::cstr2hash("PaviaRunControl");

  void DoStatus() override;

private:
  uint32_t m_stop_second;
  bool m_flag_running;
  std::chrono::steady_clock::time_point m_tp_start_run;
  //std::map<std::string, std::string> module_state; 
  //std::mutex mtx;
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::RunControl>::
    Register<PaviaRunControl, const std::string&>(PaviaRunControl::m_id_factory);
}

PaviaRunControl::PaviaRunControl(const std::string & listenaddress)
  :RunControl(listenaddress){
  m_flag_running = false;
}

void PaviaRunControl::StartRun(){
  RunControl::StartRun();
  m_tp_start_run = std::chrono::steady_clock::now();
  m_flag_running = true;
}

void PaviaRunControl::StopRun(){
  RunControl::StopRun();
  m_flag_running = false;
}

void PaviaRunControl::Configure(){
  auto conf = GetConfiguration();
  m_stop_second = conf->Get("EX0_STOP_RUN_AFTER_N_SECONDS", 0);
  RunControl::Configure();
}

void PaviaRunControl::Exec(){
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

/*void PaviaRunControl::DoStatus(ConnectionSPC con, StatusSPC st){

	std::lock_guard<std::mutex> lock(mtx);
	module_state[con->GetName()] = st->GetStatus();

}*/

