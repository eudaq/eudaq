#include "eudaq/DataCollector.hh"
#include "eudaq/Event.hh"
#include <mutex>
#include <deque>
#include <map>
#include <set>

//----------DOC-MARK-----BEG*DEC-----DOC-MARK----------
class Ex1TgTsDataCollector:public eudaq::DataCollector{
public:
  Ex1TgTsDataCollector(const std::string &name,
		   const std::string &runcontrol);

  void DoStartRun() override;
  void DoConfigure() override;
  void DoConnect(eudaq::ConnectionSPC id) override;
  void DoDisconnect(eudaq::ConnectionSPC id) override;
  void DoReceive(eudaq::ConnectionSPC id, eudaq::EventSP ev) override;

  static const uint32_t m_id_factory = eudaq::cstr2hash("Ex1TgTsDataCollector");
private:
  void AddEvent_TimeStamp(uint32_t id, eudaq::EventSPC ev);
  void AddEvent_TriggerN(uint32_t id, eudaq::EventSPC ev);

  void BuildEvent_TimeStamp();
  void BuildEvent_Trigger();
  void BuildEvent_Final();

  //conf
  bool m_pri_ts;
  //
  
  std::mutex m_mtx_map;
  std::set<uint32_t> m_con_id;
  std::set<uint32_t> m_con_has_bore;
  bool m_has_all_bore;
  
  //ts
  std::deque<eudaq::EventUP> m_que_event_wrap_ts;
  std::map<uint32_t, std::deque<eudaq::EventSPC>> m_que_event_ts;
  std::set<uint32_t> m_event_ready_ts;
  uint64_t m_ts_last_end;
  uint64_t m_ts_curr_beg;
  uint64_t m_ts_curr_end;

  //tg
  std::deque<eudaq::EventUP> m_que_event_wrap_tg;
  std::map<uint32_t, std::deque<eudaq::EventSPC>> m_que_event_tg;
  std::set<uint32_t> m_event_ready_tg;
  uint32_t m_tg_curr_n;
};
//----------DOC-MARK-----END*DEC-----DOC-MARK----------

namespace{
  auto dummy0 = eudaq::Factory<eudaq::DataCollector>::
    Register<Ex1TgTsDataCollector, const std::string&, const std::string&>
    (Ex1TgTsDataCollector::m_id_factory);
}

Ex1TgTsDataCollector::Ex1TgTsDataCollector(const std::string &name,
				   const std::string &runcontrol):
  DataCollector(name, runcontrol),m_ts_last_end(0), m_ts_curr_beg(-2), m_ts_curr_end(-1){
  
}


void Ex1TgTsDataCollector::DoStartRun(){
  std::unique_lock<std::mutex> lk(m_mtx_map);

  m_has_all_bore = false;
  // m_con_id.clear(); //NOTE: 
  m_con_has_bore.clear();

  m_que_event_wrap_ts.clear();
  m_que_event_ts.clear();
  m_event_ready_ts.clear();
  m_ts_last_end = 0;
  m_ts_curr_beg = -2;
  m_ts_curr_end = -1;

  //tg
  m_que_event_wrap_tg.clear();
  m_que_event_tg.clear();
  m_event_ready_tg.clear();
  m_tg_curr_n = 0;
}

void Ex1TgTsDataCollector::DoConfigure(){
  auto conf = GetConfiguration();
  if(conf){
    conf->Print();
    m_pri_ts = conf->Get("PRIOR_TIMESTAMP", m_pri_ts?1:0);
  }
}


void Ex1TgTsDataCollector::DoConnect(eudaq::ConnectionSPC idx){
  uint32_t id = eudaq::str2hash(idx->GetName());
  std::unique_lock<std::mutex> lk(m_mtx_map);
  m_con_id.insert(id);

  std::cout<<"connecting m_con_id.size() "<< m_con_id.size()<<std::endl;

}

void Ex1TgTsDataCollector::DoDisconnect(eudaq::ConnectionSPC idx){
  uint32_t id = eudaq::str2hash(idx->GetName());
  std::unique_lock<std::mutex> lk(m_mtx_map);
  m_con_id.erase(id);
}

void Ex1TgTsDataCollector::DoReceive(eudaq::ConnectionSPC idx, eudaq::EventSP evsp){
  uint32_t id = eudaq::str2hash(idx->GetName());
  std::unique_lock<std::mutex> lk(m_mtx_map);
  std::cout<< "hi new enent is comming\n";

  
  if(!m_has_all_bore){
    if(evsp->IsBORE()){
      m_con_has_bore.insert(id);
      std::cout<<"come here  10\n";
    } 

    std::cout<<"m_con_id.size() "<< m_con_id.size()<<std::endl;
    std::cout<<"m_con_has_bore.size() "<< m_con_has_bore.size()<<std::endl;
    
    if(m_con_has_bore.size() == m_con_id.size()){
      m_has_all_bore = true;
    }
  }

  if(evsp->IsFlagTimestamp()){
    AddEvent_TimeStamp(id, evsp);    
  }
  if(evsp->IsFlagTrigger()){
    AddEvent_TriggerN(id, evsp);
  }
  
  if(!m_has_all_bore)
    return;

  
  BuildEvent_TimeStamp();
  
  BuildEvent_Trigger();

  std::cout<<"m_que_event_wrap_ts.size() "<<m_que_event_wrap_ts.size()<<std::endl;
  std::cout<<"m_que_event_wrap_tg.size() "<<m_que_event_wrap_tg.size()<<std::endl;
  BuildEvent_Final();
  
  std::cout<<"m_que_event_wrap_ts.size() "<<m_que_event_wrap_ts.size()<<std::endl;
  std::cout<<"m_que_event_wrap_tg.size() "<<m_que_event_wrap_tg.size()<<std::endl;
}


void Ex1TgTsDataCollector::BuildEvent_Final(){

  if(m_pri_ts)
  while(!m_que_event_wrap_ts.empty()){
    std::cout<< "ly 0\n";
    auto& ev_ts = m_que_event_wrap_ts.front();
    if(!ev_ts->IsFlagTrigger()){
      ev_ts->Print(std::cout);
      WriteEvent(std::move(ev_ts));
      m_que_event_wrap_ts.pop_front();
      continue;
    }//else

    uint32_t tg_n = ev_ts->GetTriggerN();

    std::cout<< "ly 1\n";
    //filter out unused/old trigger
    while(!m_que_event_wrap_tg.empty()){
      auto& ev_tg = m_que_event_wrap_tg.front();
      std::cout<<"ev_tg->GetTriggerN() tg_n " << ev_tg->GetTriggerN() <<"  "<< tg_n<<std::endl;
      if(ev_tg->GetTriggerN() < tg_n){
	m_que_event_wrap_tg.pop_front();
	continue;
      }
      else
	break;
    }
    //
    std::cout<< "ly 2\n";
    if(!m_que_event_tg.empty())
      if(m_que_event_wrap_tg.empty() )
	break; //waiting ev_tg 

    std::cout<< "ly 3\n";
    auto& ev_tg = m_que_event_wrap_tg.front();
    if(ev_tg && ev_tg->GetTriggerN() == tg_n){
      uint32_t n = ev_tg->GetNumSubEvent();
      for(uint32_t i = 0; i< n; i++){
	auto ev_sub_tg = ev_tg->GetSubEvent(i);
	ev_ts->AddSubEvent(ev_sub_tg);
      }
      ev_ts->Print(std::cout);
      WriteEvent(std::move(ev_ts));
      m_que_event_wrap_ts.pop_front();
    }
    else{ //> tg_n
      std::cout<< " ev_tg->GetTriggerN() "<< ev_tg->GetTriggerN() <<std::endl;
      break;
      
    }
  }

  else
  while(!m_que_event_wrap_tg.empty()){
    auto& ev_tg = m_que_event_wrap_tg.front();
    if(!ev_tg->IsFlagTimestamp()){
      ev_tg->Print(std::cout);
      WriteEvent(std::move(ev_tg));
      m_que_event_wrap_tg.pop_front();
      continue;
    }//else

    uint64_t ts_beg = ev_tg->GetTimestampBegin();
    uint64_t ts_end = ev_tg->GetTimestampEnd();
    uint32_t tg_n = ev_tg->GetTriggerN();

    //
    if(!m_que_event_ts.empty()) //for eore
      if(m_que_event_wrap_ts.empty() ||
	 m_que_event_wrap_ts.back()->GetTimestampEnd() < ts_end)
	break; //waiting ev_ts
    
    while(!m_que_event_wrap_ts.empty()){
      auto& ev_ts = m_que_event_wrap_ts.front();
      if(!ev_ts->IsFlagTimestamp() || ev_ts->GetTimestampEnd() <= ts_beg){
	m_que_event_wrap_ts.pop_front();
	continue;
      }

      if(ev_ts->GetTimestampBegin() >= ts_end){
	break;
      }
      
      uint32_t n = ev_ts->GetNumSubEvent();
      for(uint32_t i = 0; i< n; i++){
	auto ev_sub_ts = ev_ts->GetSubEvent(i);
	ev_tg->AddSubEvent(ev_sub_ts);
      }
      m_que_event_wrap_ts.pop_front();
    }
    ev_tg->Print(std::cout);
    WriteEvent(std::move(ev_tg));
    m_que_event_wrap_tg.pop_front();
  }
}

void Ex1TgTsDataCollector::AddEvent_TimeStamp(uint32_t id, eudaq::EventSPC ev){
  if(ev->IsBORE()){
    m_que_event_ts[id].clear();
    m_que_event_ts[id].push_back(ev);
  }
  else{
    auto it_que_p = m_que_event_ts.find(id);
    if(it_que_p == m_que_event_ts.end())
      return;
    else
      it_que_p->second.push_back(ev);
  }
  
  uint64_t ts_ev_beg =  ev->GetTimestampBegin();
  uint64_t ts_ev_end =  ev->GetTimestampEnd();
  
  if(ts_ev_beg < m_ts_last_end){
    ev->Print(std::cout);
    std::cout<< "m_ts_last_end "<< m_ts_last_end<<std::endl;
    EUDAQ_THROW("ts_ev_beg < m_ts_last_end");
  }
  
  m_event_ready_ts.insert(id);
  bool curr_ts_updated = false;
  if(ts_ev_beg < m_ts_curr_beg){
    m_ts_curr_beg = ts_ev_beg;
    curr_ts_updated = true;
  }
  if(ts_ev_end < m_ts_curr_end){
    m_ts_curr_end = ts_ev_end;
    curr_ts_updated = true;
  }

  if(curr_ts_updated){
    for(auto &que_p :m_que_event_ts){
      auto &que = que_p.second;
      auto &que_id = que_p.first;
      if(!que.empty() &&
	 (m_ts_curr_end <= que.back()->GetTimestampEnd() || que.back()->IsEORE()))
	m_event_ready_ts.insert(que_id);
      else
	m_event_ready_ts.erase(que_id);
    }
  }
}

void Ex1TgTsDataCollector::BuildEvent_TimeStamp(){
  std::cout<< "m_event_ready_ts.size() "<< m_event_ready_ts.size()<<std::endl;
  std::cout<< "m_que_event_ts.size() "<< m_que_event_ts.size()<<std::endl;
  while(!m_event_ready_ts.empty() && m_event_ready_ts.size() == m_que_event_ts.size()){
    std::cout<<"ts building main, loop\n";
    uint64_t ts_next_end = -1;
    uint64_t ts_next_beg = ts_next_end - 1;
    auto ev_wrap = eudaq::Event::MakeUnique(GetFullName());
    ev_wrap->SetTimestamp(m_ts_curr_beg, m_ts_curr_end);
    std::set<uint32_t> has_eore;
    for(auto &que_p :m_que_event_ts){
      std::cout<<"que_p, loop\n";
      auto &que = que_p.second;
      auto id = que_p.first;
      eudaq::EventSPC subev;
      while(!que.empty()){
	std::cout<<"!que.empty(), loop\n";
	uint64_t ts_sub_beg = que.front()->GetTimestampBegin();
	uint64_t ts_sub_end = que.front()->GetTimestampEnd();
	if(ts_sub_end <= m_ts_curr_beg){
	  std::cout<<"if 1\n";
	  if(que.front()->IsEORE())
	    has_eore.insert(id);
	  que.pop_front();
	  // if(que.empty())
	  //   EUDAQ_THROW("There should be more data!");
	  //It is not empty after pop. except eore
	  continue;
	}
	else if(ts_sub_beg >= m_ts_curr_end){
	  std::cout<<"if 2\n";
	  m_event_ready_ts.insert(id);
	  if(ts_sub_beg < ts_next_beg)
	    ts_next_beg = ts_sub_beg;
	  if(ts_sub_end < ts_next_end)
	    ts_next_end = ts_sub_end;
	  break;
	}

	subev = que.front();
	
	if(ts_sub_end < m_ts_curr_end){
	  std::cout<<"if 3\n";
	  if(que.front()->IsEORE())
	    has_eore.insert(id);
	  que.pop_front();
	  // if(que.empty())
	  //   EUDAQ_THROW("There should be more data!");
	  //It is not empty after pop. except eore
	  continue;
	}
	else if(ts_sub_end == m_ts_curr_end){
	  std::cout<<"if 4\n";
	  if(que.front()->IsEORE())
	    has_eore.insert(id);
	  que.pop_front();
	  if(que.empty())
	    m_event_ready_ts.erase(id);
	  //If it is not empty after pop,
	  //next loop will come to the case ts_sub_beg >= m_ts_curr_end,
	  //and update ts_next_beg/end and ready flag;
	  continue;
	}

	//ts_sub_end > m_ts_curr_end, subev is not popped.
	//There is at least 1 event inside que. If thre are more ...
	else
	if(que.size()>1){
	  std::cout<<"if 5\n";
	  m_event_ready_ts.insert(id);
	  uint64_t ts_sub1_beg = que.at(1)->GetTimestampBegin();
	  uint64_t ts_sub1_end = que.at(1)->GetTimestampEnd();
	  if(ts_sub1_beg < ts_next_beg)
	    ts_next_beg = ts_sub1_beg;
	  if(ts_sub1_end < ts_next_end)
	    ts_next_end = ts_sub1_end;
	  break;
	}
	else{//(que.size()==1){
	  //long ts event, maybe eore
	  if(!que.front()->IsEORE())
	    m_event_ready_ts.erase(id);
	  break;
	}
      }
      if(subev){
	if(!ev_wrap->IsFlagTrigger() && subev->IsFlagTrigger()){
	  ev_wrap->SetTriggerN(subev->GetTriggerN());
	}
	ev_wrap->AddSubEvent(subev);
      }
    }

    for(auto e: has_eore){
      m_event_ready_ts.erase(e);
      m_que_event_ts.erase(e);
    }
    
    m_ts_last_end = m_ts_curr_end;
    m_ts_curr_beg = ts_next_beg;
    m_ts_curr_end = ts_next_end;
    m_que_event_wrap_ts.push_back(std::move(ev_wrap));
  }
}

void Ex1TgTsDataCollector::AddEvent_TriggerN(uint32_t id, eudaq::EventSPC ev){
  if(ev->IsBORE()){
    m_que_event_tg[id].clear();
    m_que_event_tg[id].push_back(ev);
  }
  else{
    auto it_que_p = m_que_event_tg.find(id);
    if(it_que_p == m_que_event_tg.end())
      return;
    else
      it_que_p->second.push_back(ev);
  }

  if(ev->GetTriggerN() > m_tg_curr_n){
    m_event_ready_tg.insert(id);
  }
  
  std::cout<< "que_size "<< m_que_event_tg[id].size()<<std::endl;
  std::cout<< "GetTriggerN() :: m_tg_curr_n   "<<  ev->GetTriggerN() << "   "<<m_tg_curr_n<<std::endl;
}


void Ex1TgTsDataCollector::BuildEvent_Trigger(){
  while(!m_event_ready_tg.empty() && m_event_ready_tg.size() ==m_que_event_tg.size()){
    std::set<uint32_t> has_eore;
    auto ev_wrap = eudaq::Event::MakeUnique(GetFullName());
    ev_wrap->SetTriggerN(m_tg_curr_n);
    for(auto &que_p :m_que_event_tg){
      auto &que = que_p.second;
      auto id = que_p.first;
      while(!que.empty() && que.front()->GetTriggerN() <= m_tg_curr_n){
	if(que.front()->GetTriggerN() == m_tg_curr_n){
	  if(!ev_wrap->IsFlagTimestamp() &&  que.front()->IsFlagTimestamp()){
	    ev_wrap->SetTimestamp(que.front()->GetTimestampBegin(), que.front()->GetTimestampEnd());
	  }
	  ev_wrap->AddSubEvent(que.front());
	}
	if(que.front()->IsEORE()){
	  has_eore.insert(id);
	}
	que.pop_front();
      }
      if(!que.empty() && (que.back()->GetTriggerN() > m_tg_curr_n+1 || que.back()->IsEORE()))
	m_event_ready_tg.insert(id);
      else
	m_event_ready_tg.erase(id);
    }
    m_tg_curr_n ++;

    for(auto e: has_eore){
      m_event_ready_tg.erase(e);
      m_que_event_tg.erase(e);   
    }
    
    if(ev_wrap->GetNumSubEvent()){
      // ev_wrap->Print(std::cout);
      m_que_event_wrap_tg.push_back(std::move(ev_wrap));
    }
  }
}
