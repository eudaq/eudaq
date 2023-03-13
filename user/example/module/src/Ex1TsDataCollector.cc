#include "eudaq/DataCollector.hh"
#include <mutex>
#include <deque>
#include <map>
#include <set>

//----------DOC-MARK-----BEG*DEC-----DOC-MARK----------
class Ex1TsDataCollector:public eudaq::DataCollector{
public:
  Ex1TsDataCollector(const std::string &name,
		   const std::string &runcontrol);
  void DoConnect(eudaq::ConnectionSPC id) override;
  void DoDisconnect(eudaq::ConnectionSPC id) override;
  void DoReceive(eudaq::ConnectionSPC id, eudaq::EventSP ev) override;

  static const uint32_t m_id_factory = eudaq::cstr2hash("Ex1TsDataCollector");
private:
  void BuildEvent();
  
  std::mutex m_mtx_map;
  std::map<eudaq::ConnectionSPC, std::deque<eudaq::EventSPC>> m_que_event_ts;
  std::set<eudaq::ConnectionSPC> m_event_ready_ts;
  std::set<eudaq::ConnectionSPC> m_conn_disconnected;
  uint64_t m_ts_last_end;
  uint64_t m_ts_curr_beg;
  uint64_t m_ts_curr_end;
};
//----------DOC-MARK-----END*DEC-----DOC-MARK----------

namespace{
  auto dummy0 = eudaq::Factory<eudaq::DataCollector>::
    Register<Ex1TsDataCollector, const std::string&, const std::string&>
    (Ex1TsDataCollector::m_id_factory);
}

Ex1TsDataCollector::Ex1TsDataCollector(const std::string &name,
				   const std::string &runcontrol):
  DataCollector(name, runcontrol),m_ts_last_end(0), m_ts_curr_beg(-2), m_ts_curr_end(-1){
  
}

void Ex1TsDataCollector::DoConnect(eudaq::ConnectionSPC idx){
  std::unique_lock<std::mutex> lk(m_mtx_map);
  if(m_que_event_ts.empty()){
    m_ts_last_end = 0;
    m_ts_curr_beg = -2;
    m_ts_curr_end = -1;
    m_conn_disconnected.clear();
    m_que_event_ts.clear();
    m_event_ready_ts.clear();
  }
  m_que_event_ts[idx].clear();
}

void Ex1TsDataCollector::DoDisconnect(eudaq::ConnectionSPC idx){
  std::unique_lock<std::mutex> lk(m_mtx_map);
  if(m_que_event_ts[idx].empty())
    m_que_event_ts.erase(idx);
  else
    m_conn_disconnected.insert(idx);
}

void Ex1TsDataCollector::DoReceive(eudaq::ConnectionSPC idx, eudaq::EventSP evsp){
  if(!evsp->IsFlagTimestamp()){
    EUDAQ_THROW("!evsp->IsFlagTimestamp()");
  }

  auto it_que_p = m_que_event_ts.find(idx);
  if(it_que_p == m_que_event_ts.end())
    EUDAQ_THROW("it_que_p == m_que_event_ts.end()");
  else
    it_que_p->second.push_back(evsp);
  
  uint64_t ts_ev_beg =  evsp->GetTimestampBegin();
  uint64_t ts_ev_end =  evsp->GetTimestampEnd();
  
  if(ts_ev_beg < m_ts_last_end){
    EUDAQ_THROW("ts_ev_beg < m_ts_last_end");
  }
  
  m_event_ready_ts.insert(idx);
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
	 m_ts_curr_end <= que.back()->GetTimestampEnd())
	m_event_ready_ts.insert(que_id);
      else
	m_event_ready_ts.erase(que_id);
    }
  }
  BuildEvent();
}


void Ex1TsDataCollector::BuildEvent(){
  while(!m_event_ready_ts.empty() && m_event_ready_ts.size() == m_que_event_ts.size()){
    uint64_t ts_next_end = -1;
    uint64_t ts_next_beg = ts_next_end - 1;
    auto ev_sync = eudaq::Event::MakeUnique(GetFullName());
    ev_sync->SetTimestamp(m_ts_curr_beg, m_ts_curr_end);
    for(auto &que_p :m_que_event_ts){
      auto &que = que_p.second;
      auto id = que_p.first;
      eudaq::EventSPC subev;
      while(!que.empty()){
	uint64_t ts_sub_beg = que.front()->GetTimestampBegin();
	uint64_t ts_sub_end = que.front()->GetTimestampEnd();
	if(ts_sub_end <= m_ts_curr_beg){
	  que.pop_front();
	  continue;
	}
	else if(ts_sub_beg >= m_ts_curr_end){
	  m_event_ready_ts.insert(id);
	  if(ts_sub_beg < ts_next_beg)
	    ts_next_beg = ts_sub_beg;
	  if(ts_sub_end < ts_next_end)
	    ts_next_end = ts_sub_end;
	  break;
	}

	subev = que.front();
	
	if(ts_sub_end < m_ts_curr_end){
	  que.pop_front();
	  continue;
	}
	else if(ts_sub_end == m_ts_curr_end){
	  que.pop_front();
	  if(que.empty())
	    m_event_ready_ts.erase(id);
	  continue;
	}
	else
	if(que.size()>1){
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
	  m_event_ready_ts.erase(id);
	  break;
	}
      }
      ev_sync->AddSubEvent(subev);
    }
    
    m_ts_last_end = m_ts_curr_end;
    m_ts_curr_beg = ts_next_beg;
    m_ts_curr_end = ts_next_end;
    WriteEvent(std::move(ev_sync));
  }
}
