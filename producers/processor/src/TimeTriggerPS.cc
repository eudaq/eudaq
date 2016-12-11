#include "Processor.hh"

#include <ctime>
#include <ratio>
#include <chrono>
#include <thread>
#include <random>

namespace eudaq{

  class TimeTriggerPS: public Processor{
  public:
    TimeTriggerPS();
    void ProcessCommand(const std::string& cmd_name, const std::string& cmd_par) final override;
    void ProduceEvent() final override;

  private:
    std::chrono::steady_clock::time_point m_tp_start_run;
    uint32_t m_ts_resulution;
    uint32_t m_min_period;
    uint32_t m_duration;
    uint32_t m_trigger_rate;
    uint32_t m_event_n;
  };

  namespace{
  auto dummy0 = Factory<Processor>::Register<TimeTriggerPS>(eudaq::cstr2hash("TimeTriggerPS"));
  }

  TimeTriggerPS::TimeTriggerPS()
    :Processor("TimeTriggerPS"){
    m_ts_resulution = 1; // ns
    m_trigger_rate = 10000;
    m_event_n = 0;
    m_duration = 200;
    m_min_period = 500;
  }
  
  void TimeTriggerPS::ProcessCommand(const std::string& cmd_name, const std::string& cmd_par){
    switch(cstr2hash(cmd_name.c_str())){
    case cstr2hash("RESULUTION"):{
      m_ts_resulution = std::stoul(cmd_par);     
      break;
    }
    case cstr2hash("RATE"):{
      m_trigger_rate = std::stoul(cmd_par);     
      break;
    }
    default:
      std::cout<<"unkonw user cmd"<<std::endl;
    }
  }

  void TimeTriggerPS::ProduceEvent(){
    std::random_device rd;
    std::mt19937 gen(rd());
    uint32_t mean_period = 1000000000/m_trigger_rate;
    uint32_t max_period_uni2 = mean_period*2-m_min_period;
    if(max_period_uni2 <= m_min_period) std::cerr<<"ERROR period \n";
    if(m_min_period < m_duration) std::cerr<<"ERROR period \n";
    std::uniform_int_distribution<uint32_t> dis(m_min_period, max_period_uni2);
    auto tp_trigger = std::chrono::steady_clock::now();
    
    // while(1){
    for(uint32_t i=0; i<110; i++ ){
      EventUP ev = Factory<Event>::Create(cstr2hash("TRIGGER"), cstr2hash("TRIGGER"), 0, GetInstanceN());
      ev->SetEventN(m_event_n++);
      std::chrono::nanoseconds ns_sleep(dis(gen));
      tp_trigger += ns_sleep;
      std::this_thread::sleep_until(tp_trigger);
      auto tp_trigger_now = std::chrono::steady_clock::now();
      std::chrono::nanoseconds du_begin(tp_trigger_now - m_tp_start_run);
      ev->SetTimestampBegin(du_begin.count());
      ev->SetTimestampEnd(du_begin.count()+m_duration);
      RegisterEvent(std::move(ev));
    }
    EventUP ev = Factory<Event>::Create<const uint32_t&, const uint32_t&, const uint32_t&>(cstr2hash("TRIGGER"), cstr2hash("TRIGGER"), 0, GetInstanceN());
    ev->SetFlagBit(Event::FLAG_EORE);
    RegisterEvent(std::move(ev));
  }
  
}
