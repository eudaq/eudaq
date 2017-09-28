#include "eudaq/DataCollector.hh"
#include <mutex>
#include <deque>
#include <map>
#include <set>

class CaliceTsDataCollector:public eudaq::DataCollector{
public:
  CaliceTsDataCollector(const std::string &name,
		   const std::string &runcontrol);

  void DoStartRun() override;
  void DoConfigure() override;
  void DoConnect(eudaq::ConnectionSPC id) override;
  void DoDisconnect(eudaq::ConnectionSPC id) override;
  void DoReceive(eudaq::ConnectionSPC id, eudaq::EventSP ev) override;

  static const uint32_t m_id_factory = eudaq::cstr2hash("CaliceTsDataCollector");
private:
  void AddEvent_TimeStamp(uint32_t id, eudaq::EventSPC ev);
  void BuildEvent_TimeStamp();
  
  //ts
  std::deque<eudaq::EventSPC> m_que_cal;
  std::deque<eudaq::EventSPC> m_que_bif;
  eudaq::EventSPC m_ev_last_cal;
  eudaq::EventSPC m_ev_last_bif;
  uint64_t m_ts_end_last_cal;
  uint64_t m_ts_end_last_bif;
  bool m_offset_ts_done;
  int64_t m_ts_offset_cal2bif;


  uint32_t m_ev_n;
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::DataCollector>::
    Register<CaliceTsDataCollector, const std::string&, const std::string&>
    (CaliceTsDataCollector::m_id_factory);
}

CaliceTsDataCollector::CaliceTsDataCollector(const std::string &name,
					     const std::string &runcontrol):
  DataCollector(name, runcontrol){
  
}

void CaliceTsDataCollector::DoStartRun(){
  m_que_cal.clear();
  m_que_bif.clear();
  
  m_ts_end_last_cal = 0;
  m_ts_end_last_bif = 0;
  m_offset_ts_done = false;
  m_ts_offset_cal2bif = 0;
  m_ev_n = 0;
}

void CaliceTsDataCollector::DoConfigure(){
  auto conf = GetConfiguration();
  if(conf){
    conf->Print();
    // m_pri_ts = conf->Get("PRIOR_TIMESTAMP", m_pri_ts?1:0);
  }
}


void CaliceTsDataCollector::DoConnect(eudaq::ConnectionSPC idx){
  std::cout<<"connecting "<<idx<<std::endl;
}

void CaliceTsDataCollector::DoDisconnect(eudaq::ConnectionSPC idx){
  std::cout<<"disconnecting "<<idx<<std::endl;
}

void CaliceTsDataCollector::DoReceive(eudaq::ConnectionSPC idx, eudaq::EventSP ev){
  if(ev->IsFlagFake()){
    EUDAQ_WARN("Receive event fake");
    return;
  }
  if(!ev->IsFlagTimestamp()){
    EUDAQ_WARN("Receive event without TimeStamps");
    return;
  }
  std::string con_name = idx->GetName();
  if(con_name == "Producer.Calice1")
    m_que_cal.push_back(std::move(ev));
  else if(con_name == "Producer.caliceahcalbifProducer")
    m_que_bif.push_back(std::move(ev));
  else{
    EUDAQ_WARN("Receive event from unkonwn Producer");
    return;
  }

  if(m_que_cal.empty() || m_que_bif.empty()){
    return;
  }

  // std::cout<<"p 1 \n";
  
  if(!m_offset_ts_done){
    if(!m_que_cal.front()->IsBORE() || !m_que_cal.front()->IsBORE())
      EUDAQ_THROW("the first event is not bore");
    uint64_t start_ts_cal = m_que_cal.front()->GetTag("FirstROCStartTS", uint64_t(0));
    uint64_t start_ts_bif = m_que_bif.front()->GetTag("FirstROCStartTS", uint64_t(0));
    m_ts_offset_cal2bif = (start_ts_cal<<5) - start_ts_bif - 120LL;
    m_offset_ts_done = true;
    // std::cout<<"start_ts_bif "<< start_ts_bif <<std::endl;
    // std::cout<<"start_ts_cal "<< start_ts_cal <<std::endl;
    // std::cout<<"offset "<< m_ts_offset_cal2bif <<std::endl;
  }

  while(!m_que_cal.empty() && !m_que_bif.empty()){
    // std::cout<<">>>>>>>>>>>>>>>>"<<m_que_bif.size()<<" "<<m_que_cal.size()<<std::endl;
    auto ev_front_bif = m_que_bif.front(); 
    auto ev_front_cal = m_que_cal.front();
    auto ev_sync =  eudaq::Event::MakeUnique("CaliceTS");
    ev_sync->SetFlagPacket();
    uint64_t ts_beg_bif = ev_front_bif->GetTimestampBegin();
    uint64_t ts_beg_cal = (ev_front_cal->GetTimestampBegin()<<5) - m_ts_offset_cal2bif;
    uint64_t ts_beg = (ts_beg_bif<ts_beg_cal)?ts_beg_bif:ts_beg_cal;

    uint64_t ts_end_bif = ev_front_bif->GetTimestampEnd();
    uint64_t ts_end_cal = (ev_front_cal->GetTimestampEnd()<<5) - m_ts_offset_cal2bif;
    uint64_t ts_end = (ts_end_bif<ts_end_cal)?ts_end_bif:ts_end_cal;
    
    // std::cout<<"offset "<< m_ts_offset_cal2bif <<std::endl;
    // std::cout<<"ts_beg_cal "<< ts_beg_cal <<std::endl;
    // std::cout<<"ts_beg_bif "<< ts_beg_bif <<std::endl;
    // std::cout<<"ts_end_cal "<< ts_end_cal <<std::endl;
    // std::cout<<"ts_end_bif "<< ts_end_bif <<std::endl;
    
    if(ts_beg_bif < ts_end){
      ev_sync->AddSubEvent(ev_front_bif);
      m_ev_last_bif = ev_front_bif;
      m_ts_end_last_bif = ts_end_bif;
      m_que_bif.pop_front();
    }
    else if(m_ts_end_last_bif > ts_beg){
      ev_sync->AddSubEvent(m_ev_last_bif);
    }

    if(ts_beg_cal < ts_end){
      ev_sync->AddSubEvent(ev_front_cal);
      m_ev_last_cal = ev_front_cal;
      m_ts_end_last_cal = ts_end_cal;
      m_que_cal.pop_front();
    }
    else if(m_ts_end_last_cal > ts_beg){
      ev_sync->AddSubEvent(m_ev_last_cal);
    }
    ev_sync->SetTimestamp(ts_beg, ts_end, false);
    ev_sync->SetEventN(m_ev_n++);
    //ev_sync->Print(std::cout);
    WriteEvent(std::move(ev_sync));
  }
  
}
