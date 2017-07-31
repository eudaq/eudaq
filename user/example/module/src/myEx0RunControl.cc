#include "eudaq/RunControl.hh"

class myEx0RunControl: public eudaq::RunControl{
public:
  myEx0RunControl(const std::string & listenaddress);
  void Configure() override;
  void StartRun() override;
  void StopRun() override;
  void Exec() override;
  static const uint32_t m_id_factory = eudaq::cstr2hash("myEx0RunControl");

private:
  uint32_t m_stop_second;
  bool m_flag_running;
  std::chrono::steady_clock::time_point m_tp_start_run;
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::RunControl>::
    Register<myEx0RunControl, const std::string&>(myEx0RunControl::m_id_factory);
}

myEx0RunControl::myEx0RunControl(const std::string & listenaddress)
  :RunControl(listenaddress){
  m_flag_running = false;
}

void myEx0RunControl::StartRun(){
  RunControl::StartRun();
  m_tp_start_run = std::chrono::steady_clock::now();
  m_flag_running = true;
}

void myEx0RunControl::StopRun(){
  RunControl::StopRun();
  m_flag_running = false;
}

void myEx0RunControl::Configure(){
  auto conf = GetConfiguration();
  m_stop_second = conf->Get("EX0_STOP_RUN_AFTER_N_SECONDS", 0);
  RunControl::Configure();
}

void myEx0RunControl::Exec(){
  StartRunControl();
  while(IsActiveRunControl()){
    if(m_flag_running && m_stop_second){
      auto tp_now = std::chrono::steady_clock::now();
      std::chrono::nanoseconds du_ts(tp_now - m_tp_start_run);
      if(du_ts.count()/1000000000>m_stop_second)
	StopRun();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
}
