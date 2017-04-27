#define NOMINMAX
#include "Processor.hh"
#include "Event.hh"
#include <random>
#include <algorithm> 
#include <fstream>

namespace eudaq{

  class DummySystemPS: public Processor{
  public:
    DummySystemPS();
    void ProduceEvent() final override;

  private:
    uint32_t m_event_n;
    uint32_t m_n_stm;
    std::vector<uint64_t>  m_ts_last_begin;
    std::vector<uint64_t>  m_ts_last_end;
    std::vector<uint32_t>  m_stm_event_n;
    uint64_t  m_ts_tg_last_end;
    
  };


  namespace{
  auto dummy0 = Factory<Processor>::Register<DummySystemPS>(eudaq::cstr2hash("DummySystemPS"));
  }

  DummySystemPS::DummySystemPS()
    :Processor("DummySystemPS"){
    m_event_n = 0;
    m_n_stm = 10;
    m_ts_last_begin.resize(m_n_stm , 0);
    m_ts_last_end.resize(m_n_stm , 0);
    m_stm_event_n.resize(m_n_stm, 0);
    m_ts_tg_last_end = 0;
  }
  
  void DummySystemPS::ProduceEvent(){
    std::ofstream file("DummySystem.xml", std::ofstream::out | std::ofstream::trunc);

    std::random_device rd;
    std::mt19937 gen(rd());

    uint32_t min_ev_duration = 10;
    uint32_t max_ev_duration = 60;
    uint32_t max_ev_gap = 70;
    
    std::uniform_int_distribution<uint32_t> dis_duration(min_ev_duration, max_ev_duration);
    std::uniform_int_distribution<uint32_t> dis_stm_n(0, m_n_stm-1);


    // trigger event is always the earliest new event after last end_of_trigger
    // the end_of_trigger is defined by new events' earliest ts_last 
    for(; m_event_n < 1000; m_event_n ++){

      EventSP ev = Factory<Event>::MakeShared<const uint32_t&, const uint32_t&, const uint32_t&>(cstr2hash("SYNC"), cstr2hash("SYNC"), 0, GetInstanceN());
      
      std::vector<uint64_t> ts_begin(m_n_stm, 0);
      std::vector<uint64_t> ts_end(m_n_stm, 0);
      
      uint64_t ts_tg_begin;
      uint64_t ts_tg_end;
      uint32_t tg_begin_stm_n;
      uint32_t tg_end_stm_n;
      
      tg_begin_stm_n = dis_stm_n(gen);
      auto ts_tg_begin_earliest = std::max(m_ts_last_end[tg_begin_stm_n], m_ts_tg_last_end); //possiable no subevent from this stream from last trigger
      std::uniform_int_distribution<uint64_t> dis_ts_tg_begin(ts_tg_begin_earliest, ts_tg_begin_earliest +max_ev_gap);
      ts_tg_begin = dis_ts_tg_begin(gen);
      ts_tg_end = -1;
      for(uint32_t i=0; i< m_n_stm; i++ ){
	auto &ts_last_begin = m_ts_last_begin[i];
	auto &ts_last_end = m_ts_last_end[i];
	auto &stm_event_n = m_stm_event_n[i]; 
	if(i == tg_begin_stm_n){
	  ts_begin[i] = ts_tg_begin;
	}
	else{
	  auto ts_begin_earliest= std::max(ts_last_end,  ts_tg_begin); //not earlier than trigger
	  std::uniform_int_distribution<uint64_t> dis_ts_begin(ts_begin_earliest, ts_begin_earliest+max_ev_gap);
	  ts_begin[i] = dis_ts_begin(gen);
	}
	ts_end[i] = ts_begin[i] + dis_duration(gen);
	if(ts_end[i]< ts_tg_end){
	  ts_tg_end = ts_end[i];
	  tg_end_stm_n = i;
	}
      }
      
      for(uint32_t i=0; i< m_n_stm; i++ ){

	if(ts_begin[i]< ts_tg_end){ // new
	  EventUP stmev = Factory<Event>::Create<const uint32_t&, const uint32_t&, const uint32_t&>(cstr2hash("DUMMYDEV"), cstr2hash("DUMMYDEV"), 0, 1000+i);
	  stmev->SetTimestamp(ts_begin[i], ts_end[i]);
	  stmev->SetEventN(m_stm_event_n[i]);
	  stmev->SetTag("Status", "New");
	  m_ts_last_begin[i] = ts_begin[i];
	  m_ts_last_end[i] = ts_end[i];
	  m_stm_event_n[i]++;
	  ev->AddEvent(std::move(stmev));
	}
	else if(m_ts_last_end[i]> ts_tg_begin){  //no new, but old
	  EventUP stmev = Factory<Event>::Create<const uint32_t&, const uint32_t&, const uint32_t&>(cstr2hash("DUMMYDEV"), cstr2hash("DUMMYDEV"), 0, 1000+i);
	  stmev->SetTimestamp(m_ts_last_begin[i], m_ts_last_end[i]);
	  stmev->SetEventN(m_stm_event_n[i]-1);
	  stmev->SetTag("Status", "Old");
	  ev->AddEvent(std::move(stmev));
	}
      }
      ev->SetTimestamp(ts_tg_begin, ts_tg_end);
      ev->SetEventN(m_event_n);
      ev->SetTag("StreamN_Trigger_Begin", tg_begin_stm_n);
      ev->SetTag("StreamN_Trigger_End", tg_end_stm_n);
      
      ev->Print(file);
      
      m_ts_tg_last_end = ts_tg_end;
      RegisterEvent(ev);
    }
    
  }
  
}

