#include "eudaq/DataCollector.hh"
#include <mutex>
#include <deque>
#include <map>
#include <set>

class CaliceTelDataCollector:public eudaq::DataCollector{
public:
  CaliceTelDataCollector(const std::string &name,
		   const std::string &runcontrol);

  void DoStartRun() override;
  void DoStopRun() override;
  void DoConfigure() override;
  void DoConnect(eudaq::ConnectionSPC id) override;
  void DoDisconnect(eudaq::ConnectionSPC id) override;
  void DoReceive(eudaq::ConnectionSPC id, eudaq::EventSP ev) override;

  static const uint32_t m_id_factory = eudaq::cstr2hash("CaliceTelDataCollector");
private:  
  std::deque<eudaq::EventSPC> m_que_cal;
  std::deque<eudaq::EventSPC> m_que_bif;
  std::deque<eudaq::EventSPC> m_que_tel;
  eudaq::EventSPC m_ev_last_cal;
  eudaq::EventSPC m_ev_last_bif;
  eudaq::EventSPC m_ev_last_tel;
  uint64_t m_ts_end_last_cal;
  uint64_t m_ts_end_last_bif;
  bool m_offset_ts_done;
  int64_t m_ts_offset_cal2bif;
  
  int16_t m_shift_tgn_cal;
  uint32_t m_ev_n;

  bool m_accept_ev;
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::DataCollector>::
    Register<CaliceTelDataCollector, const std::string&, const std::string&>
    (CaliceTelDataCollector::m_id_factory);
}

CaliceTelDataCollector::CaliceTelDataCollector(const std::string &name,
					     const std::string &runcontrol):
  DataCollector(name, runcontrol){
  m_shift_tgn_cal = 0;

  m_accept_ev = false;
}

void CaliceTelDataCollector::DoStartRun(){
  m_que_cal.clear();
  m_que_bif.clear();
  m_que_tel.clear();
  m_ev_last_cal.reset();
  m_ev_last_bif.reset();
  m_ev_last_tel.reset();

  m_ts_end_last_cal = 0;
  m_ts_end_last_bif = 0;
  m_offset_ts_done = false;
  m_ts_offset_cal2bif = 0;
  m_ev_n = 0;

  m_accept_ev = true;
}

void CaliceTelDataCollector::DoStopRun(){
  m_accept_ev = false;
  m_ev_last_cal.reset();
  m_ev_last_bif.reset();
  m_ev_last_tel.reset();
  m_que_cal.clear();
  m_que_bif.clear();
  m_que_tel.clear();

  m_ts_end_last_cal = 0;
  m_ts_end_last_bif = 0;
  m_offset_ts_done = false;
  m_ts_offset_cal2bif = 0;
  m_ev_n = 0;
}

void CaliceTelDataCollector::DoConfigure(){
  auto conf = GetConfiguration();
  if(conf){
    conf->Print();
    m_shift_tgn_cal = conf->Get("SHIFT_TGN_CAL", int16_t(0));
  }
  EUDAQ_INFO("m_shift_tgn_cal "+ std::to_string(m_shift_tgn_cal));
}

void CaliceTelDataCollector::DoConnect(eudaq::ConnectionSPC idx){
  std::cout<<"connecting "<<idx<<std::endl;
}

void CaliceTelDataCollector::DoDisconnect(eudaq::ConnectionSPC idx){
  std::cout<<"disconnecting "<<idx<<std::endl;
}

void CaliceTelDataCollector::DoReceive(eudaq::ConnectionSPC idx, eudaq::EventSP ev){
  if(ev->IsFlagFake()){
    EUDAQ_WARN("Receive event fake, dropped");
    return;
  }
  
  if(!m_accept_ev){
    return;
  }
  if(ev->GetRunN()!=GetRunNumber()){
    EUDAQ_WARN("Receive event with unmatched run number, dropped, conns is " + idx->GetName());
    return;
  }

  std::string con_name = idx->GetName();
  if(con_name == "Producer.Calice1"){
 
    if(m_que_cal.size() >= 100000){
      EUDAQ_WARN("Too many events buffered in queue cal, dropped");
      m_que_cal.clear();
      return;
    }
    if(m_shift_tgn_cal>0){
      uint32_t tgn = ev->GetTriggerN() + m_shift_tgn_cal;
      ev->SetTriggerN(tgn);
    }
    m_que_cal.push_back(ev);
  }
  else if(con_name == "Producer.caliceahcalbifProducer"){
    if(m_que_bif.size() >= 10000){
      EUDAQ_WARN("Too many events buffered in queue bif, dropped");
      m_que_bif.clear();
      return;
    }
    m_que_bif.push_back(std::move(ev));
  }
  else if(con_name == "Producer.ni"){
    if(m_que_tel.size() >= 10000){
      EUDAQ_WARN("Too many events buffered in queue tel, dropped");
      m_que_tel.clear();
      return;
    }
    if(m_shift_tgn_cal<0){
      uint32_t tgn = ev->GetEventN() - m_shift_tgn_cal;
      ev->SetEventN(tgn);
    }
    m_que_tel.push_back(std::move(ev));
  }
  else{
    EUDAQ_WARN("Receive event from unkonwn Producer");
    return;
  }
  
  std::cout<<">>>>>>>>>>>>>>>>"
	   <<m_que_cal.size()<<" "
	   <<m_que_bif.size()<<" "
	   <<m_que_tel.size()<<std::endl;   

  if(m_que_cal.empty() || m_que_bif.empty() || m_que_tel.empty()){
    return;
  }
  
  if(!m_offset_ts_done){
    std::stringstream ss;
    ss<<"the first event in queues: run(cal:bif:tel)=("
      <<m_que_cal.front()->GetRunN()<<":"
      <<m_que_bif.front()->GetRunN()<<":"
      <<m_que_tel.front()->GetRunN()<<")  "
      <<"evn(cal:bif:tel)=("
      <<m_que_cal.front()->GetEventN()<<":"
      <<m_que_bif.front()->GetEventN()<<":"
      <<m_que_tel.front()->GetEventN()<<")  "
      <<"tgn(cal:bif:tel)=("
      <<m_que_cal.front()->GetTriggerN()<<":"
      <<m_que_bif.front()->GetTriggerN()<<":"
      <<m_que_tel.front()->GetTriggerN()<<")";
    EUDAQ_WARN(ss.str().c_str());
    if(!m_que_cal.front()->IsBORE() || !m_que_bif.front()->IsBORE() || !m_que_tel.front()->IsBORE()){
      EUDAQ_THROW("the first event is not bore");
    }
    uint64_t start_ts_cal = m_que_cal.front()->GetTag("FirstROCStartTS", uint64_t(0));
    uint64_t start_ts_bif = m_que_bif.front()->GetTag("FirstROCStartTS", uint64_t(0));
    m_ts_offset_cal2bif = (start_ts_cal<<5) - start_ts_bif - 120LL;
    m_offset_ts_done = true;
  }

  std::cout<<"############\n"
	   <<m_que_cal.front()->GetTriggerN()<<" -> "<< m_que_cal.back()->GetTriggerN()<<std::endl
    	   <<m_que_cal.front()->GetEventN()<<" -> "<< m_que_cal.back()->GetEventN()<<std::endl
	   <<m_que_tel.front()->GetEventN()<<" -> "<<m_que_tel.back()->GetEventN()<<std::endl;

  while(!m_que_cal.empty() && !m_que_bif.empty() && !m_que_tel.empty()
	&& m_que_tel.back()->GetEventN() >= m_que_cal.front()->GetTriggerN()){
    
    auto ev_front_bif = m_que_bif.front();
    auto ev_front_cal = m_que_cal.front();
    auto ev_sync =  eudaq::Event::MakeUnique("CaliceTel");
    ev_sync->SetFlagPacket();
    uint64_t ts_beg_bif = ev_front_bif->GetTimestampBegin();
    uint64_t ts_beg_cal = (ev_front_cal->GetTimestampBegin()<<5) - m_ts_offset_cal2bif;
    uint64_t ts_beg = (ts_beg_bif<ts_beg_cal)?ts_beg_bif:ts_beg_cal;

    uint64_t ts_end_bif = ev_front_bif->GetTimestampEnd();
    uint64_t ts_end_cal = (ev_front_cal->GetTimestampEnd()<<5) - m_ts_offset_cal2bif;
    uint64_t ts_end = (ts_end_bif<ts_end_cal)?ts_end_bif:ts_end_cal;
    
    if(ts_beg_bif < ts_end){
      ev_sync->AddSubEvent(ev_front_bif);
      m_ev_last_bif = ev_front_bif;
      m_ts_end_last_bif = ts_end_bif;
      m_que_bif.pop_front();
    }
    else if(m_ts_end_last_bif > ts_beg){
      ev_sync->SetTriggerN(m_ev_last_bif->GetTriggerN());
      ev_sync->AddSubEvent(m_ev_last_bif);
    }

    if(ts_beg_cal < ts_end){
      ev_sync->SetTriggerN(ev_front_cal->GetTriggerN());
      ev_sync->AddSubEvent(ev_front_cal);
      m_ev_last_cal = ev_front_cal;
      m_ts_end_last_cal = ts_end_cal;
      m_que_cal.pop_front();
    }
    else if(m_ts_end_last_cal > ts_beg){
      ev_sync->SetTriggerN(m_ev_last_cal->GetTriggerN());
      ev_sync->AddSubEvent(m_ev_last_cal);
    }

    ev_sync->SetTimestamp(ts_beg, ts_end);

    if(!ev_sync->IsFlagTrigger())
      continue;
    
    uint32_t tg_n_sync = ev_sync->GetTriggerN();
    while(!m_que_tel.empty() && m_que_tel.front()->GetEventN() < tg_n_sync){
      auto ev_sync_only_tel =  eudaq::Event::MakeUnique("CaliceTel");
      ev_sync_only_tel->SetFlagPacket();
      ev_sync_only_tel->SetTriggerN(m_que_tel.front()->GetEventN());
      ev_sync_only_tel->AddSubEvent(m_que_tel.front());
      m_ev_last_tel = m_que_tel.front();
      m_que_tel.pop_front();
      ev_sync_only_tel->SetTriggerN(m_ev_n++);
      WriteEvent(std::move(ev_sync_only_tel));
    }
    if(!m_que_tel.empty() && m_que_tel.front()->GetEventN() == tg_n_sync){
      ev_sync->AddSubEvent(m_que_tel.front());
      m_ev_last_tel = m_que_tel.front();
      m_que_tel.pop_front();
    }
    else if(m_ev_last_tel && m_ev_last_tel->GetEventN() == tg_n_sync){
      ev_sync->AddSubEvent(m_ev_last_tel);
    }
    ev_sync->SetEventN(m_ev_n++);
    WriteEvent(std::move(ev_sync));
  }
}
