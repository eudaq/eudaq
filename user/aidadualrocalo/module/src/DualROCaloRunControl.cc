#include "eudaq/RunControl.hh"

class DualROCaloRunControl: public eudaq::RunControl{
public:
  DualROCaloRunControl(const std::string & listenaddress);
  void Configure() override;
  void StartRun() override;
  void StopRun() override;
  void Exec() override;
  static const uint32_t m_id_factory = eudaq::cstr2hash("DualROCaloRunControl");

private:
  uint32_t m_stop_second;
  bool m_flag_running;
  std::chrono::steady_clock::time_point m_tp_start_run;
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::RunControl>::
    Register<DualROCaloRunControl, const std::string&>(DualROCaloRunControl::m_id_factory);
}

DualROCaloRunControl::DualROCaloRunControl(const std::string & listenaddress)
  :RunControl(listenaddress){
  m_flag_running = false;
}

void DualROCaloRunControl::StartRun(){
  RunControl::StartRun();
  m_tp_start_run = std::chrono::steady_clock::now();
  m_flag_running = true;
}

void DualROCaloRunControl::StopRun(){
  RunControl::StopRun();
  m_flag_running = false;
}

void DualROCaloRunControl::Configure(){
  auto conf = GetConfiguration();
  m_stop_second = conf->Get("DUALROCALO_STOP_RUN_AFTER_N_SECONDS", 0);
  RunControl::Configure();
}

void DualROCaloRunControl::Exec(){
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