#include "eudaq/RunControl.hh"

/*
 * @Nov 21: to be udpated to fix the broken run control when controling the kpix producer after sync to the latest central branch update.
 */

class kpixRunControl: public eudaq::RunControl{
public:
  kpixRunControl(const std::string & listenaddress);
  void Configure() override;
  void StartRun() override;
  void StopRun() override;
  void Exec() override;
  void DoStatus(eudaq::ConnectionSPC id,
		std::shared_ptr<const eudaq::Status> status) override;
  static const uint32_t m_id_factory = eudaq::cstr2hash("kpixRunControl");

private:
  uint32_t m_stop_second;
  bool m_flag_running;
  std::chrono::steady_clock::time_point m_tp_start_run;

  std::string m_exit_of_run_kpix;
  std::string m_check_full_name;
  uint32_t m_max_evn;
  uint32_t m_flag_goto_stop_run;
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::RunControl>::
    Register<kpixRunControl, const std::string&>(kpixRunControl::m_id_factory);
}

kpixRunControl::kpixRunControl(const std::string & listenaddress)
  :RunControl(listenaddress){
  m_flag_running = false;
  std::cout<<"HI, I'm kpixRunControl"<<std::endl;
}

void kpixRunControl::DoStatus(eudaq::ConnectionSPC id,
			      std::shared_ptr<const eudaq::Status> status){

  /* Read Status of the connection id
   * -> customized status defined by Producer::OnStatus()
   */

  /* m_max_evn = 5; // to move to config*/
  
  if (id->GetType()+"."+id->GetName() == m_check_full_name){
    /* //--> if you want a max event cut at RC level:
      std::string ev_str = status->GetTag("EventN");
    std::cout << "[CHECK] " << ev_str
    	      << std::endl;
    uint32_t evn = 0;
    if (ev_str.size())
      evn = atol(ev_str.c_str());
    if (m_max_evn && evn > m_max_evn)
      m_flag_goto_stop_run = 1;
    */
    
    m_exit_of_run_kpix = status->GetTag("KpixStopped");
    bool exit_of_run_kpix = m_exit_of_run_kpix=="true" ? true : false;
    if (exit_of_run_kpix) m_flag_goto_stop_run = 1;
    else m_flag_goto_stop_run = 0;
    // std::cout<< "[CHECK]" << m_exit_of_run_kpix
    //	     << std::endl;
    
  }
  
     
}

void kpixRunControl::StartRun(){
  RunControl::StartRun();
  m_tp_start_run = std::chrono::steady_clock::now();
  m_flag_running = true;
}

void kpixRunControl::StopRun(){
  RunControl::StopRun();
  m_flag_running = false;
}

void kpixRunControl::Configure(){
  auto conf = GetConfiguration();
  m_stop_second = conf->Get("KPIX_STOP_RUN_AFTER_N_SECONDS", 0);
  m_check_full_name = conf->Get("PRODUCER_TO_CONTROL", "Producer.lycoris");
  RunControl::Configure();
}

void kpixRunControl::Exec(){
  StartRunControl();
  while(IsActiveRunControl()){
    if (m_flag_running) {
      if (m_flag_goto_stop_run) {
	std::cout<<"\t STOP from Run Control since kpix stopped running" <<std::endl;
	StopRun();
      }

      else if(m_flag_running && m_stop_second){
	auto tp_now = std::chrono::steady_clock::now();
	std::chrono::nanoseconds du_ts(tp_now - m_tp_start_run);
	if(du_ts.count()/1000000000>m_stop_second){
	  std::cout<<"Stopped from Run Control due to TIME OUT!"<<std::endl;
	  StopRun();
	}
      }
      
      else; 
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
}
