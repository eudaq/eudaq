#include "eudaq/DataCollector.hh"
#include "eudaq/Event.hh"
#include <mutex>
#include <deque>
#include <map>

namespace eudaq {
  class TimestampSyncDataCollector :public DataCollector{
  public:
    TimestampSyncDataCollector(const std::string &name,
			       const std::string &runcontrol);

    void DoStartRun() override;
    void DoConnect(ConnectionSPC id /*id*/) override;
    void DoDisconnect(ConnectionSPC id /*id*/) override;
    void DoReceive(ConnectionSPC id, EventSP ev) override;
    
    static const uint32_t m_id_factory = eudaq::cstr2hash("TimestampSyncDataCollector");
  private:
    std::map<std::string, std::deque<EventSPC>> m_que_event;
    std::map<std::string, bool> m_event_ready;
    std::mutex m_mtx_map;
    uint64_t m_ts_last_end;
    uint64_t m_ts_curr_beg;
    uint64_t m_ts_curr_end;
  };

  namespace{
    auto dummy0 = Factory<DataCollector>::
      Register<TimestampSyncDataCollector, const std::string&, const std::string&>
      (TimestampSyncDataCollector::m_id_factory);
  }

  TimestampSyncDataCollector::TimestampSyncDataCollector(const std::string &name,
							 const std::string &runcontrol):
    DataCollector(name, runcontrol),m_ts_last_end(0), m_ts_curr_beg(-2), m_ts_curr_end(-1){
  }

  void TimestampSyncDataCollector::DoStartRun(){
    std::unique_lock<std::mutex> lk(m_mtx_map);
    m_ts_last_end = 0;
    m_ts_curr_beg = -2;
    m_ts_curr_end = -1;
    for(auto &que :m_que_event){
      que.second.clear();
    }
    for(auto &ready :m_event_ready){
      ready.second = false;
    }    
  }

  
  void TimestampSyncDataCollector::DoConnect(ConnectionSPC id){
    std::unique_lock<std::mutex> lk(m_mtx_map);
    std::string pdc_name = id->GetName();
    if(m_que_event.find(pdc_name) != m_que_event.end())
      EUDAQ_THROW("DataCollector::Doconnect, multiple producers are sharing a same name");
    m_que_event[pdc_name].clear();
    m_event_ready[pdc_name] = false;
  }

  void TimestampSyncDataCollector::DoDisconnect(ConnectionSPC id){
    std::unique_lock<std::mutex> lk(m_mtx_map);
    std::string pdc_name = id->GetName();
    if(m_que_event.find(pdc_name) == m_que_event.end())
      EUDAQ_THROW("DataCollector::DisDoconnect, the disconnecting producer was not existing in list");
    EUDAQ_WARN("Producer."+pdc_name+" is disconnected, the remaining events are erased. ("+std::to_string(m_que_event[pdc_name].size())+ " Events)");
    m_que_event.erase(pdc_name);
    m_event_ready.erase(pdc_name);
  }
  
  void TimestampSyncDataCollector::DoReceive(ConnectionSPC id, EventSP ev){
    std::unique_lock<std::mutex> lk(m_mtx_map);
    std::string pdc_name = id->GetName();
    m_que_event[pdc_name].push_back(ev);
    uint64_t ts_ev_beg =  ev->GetTimestampBegin();
    uint64_t ts_ev_end =  ev->GetTimestampEnd();

    if(m_ts_last_end == -1){
      EUDAQ_ERROR("Timestamp is already overflow, please restart run.");
      EUDAQ_THROW("Timestamp is already overflow, please restart run.");
    }
    
    if(ts_ev_beg < m_ts_last_end){
      EUDAQ_THROW("ts_ev_beg < m_ts_last_end");
    }
    
    if(ts_ev_beg >= ts_ev_end){
      EUDAQ_THROW("ts_ev_beg >= ts_ev_end");
    }
    
    m_event_ready[pdc_name] = true;
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
      for(auto &que :m_que_event){
	auto &ready = m_event_ready[que.first];
	ready = false;
	if(!que.second.empty()){
	  if(m_ts_curr_beg >= que.second.front()->GetTimestampBegin() &&
	     m_ts_curr_end <= que.second.back()->GetTimestampEnd())
	    ready = true;
	}
      }
    }

    uint32_t ready_c = 0;
    for(auto &ready :m_event_ready){
      if(ready.second)
	ready_c ++;
    }
    
    while(ready_c == m_event_ready.size()){
      uint64_t ts_next_end = -1;
      uint64_t ts_next_beg = ts_next_end - 1;
      auto ev_wrap = Event::MakeUnique(GetFullName());
      ev_wrap->SetFlagPacket();
      ev_wrap->SetTimestamp(m_ts_curr_beg, m_ts_curr_end);
      for(auto &que :m_que_event){
	auto &ready = m_event_ready[que.first];
	EventSPC subev;
	while(!que.second.empty()){
	  uint64_t ts_sub_beg = que.second.front()->GetTimestampBegin();
	  uint64_t ts_sub_end = que.second.front()->GetTimestampEnd();
	  if(ts_sub_end <= m_ts_curr_beg){
	    que.second.pop_front();
	    if(que.second.empty())
	      EUDAQ_THROW("There should be more data!");
	    //It is not empty after pop.
	    continue;
	  }
	  if(ts_sub_beg >= m_ts_curr_end){
	    ready = true;
	    if(ts_sub_beg < ts_next_beg)
	      ts_next_beg = ts_sub_beg;
	    if(ts_sub_end < ts_next_end)
	      ts_next_end = ts_sub_end;
	    break;
	  }
	  subev = que.second.front();
	  
	  if(ts_sub_end < m_ts_curr_end){
	    que.second.pop_front();
	    if(que.second.empty())
	      EUDAQ_THROW("There should be more data!");
	    //It is not empty after pop.
	    continue;
	  }

	  if(ts_sub_end == m_ts_curr_end){
	    que.second.pop_front();
	    //If it is not empty after pop,
	    //next loop will come to the case ts_sub_beg >= m_ts_curr_end,
	    //and update ts_next_beg/end and ready flag;
	    continue;
	  }

	  //ts_sub_end > m_ts_curr_end, subev is not popped.
	  //There is at least 1 event inside que. If thre are more ...
	  if(que.second.size()>1){
	    ready = true;
	    uint64_t ts_sub1_beg = que.second.at(1)->GetTimestampBegin();
	    uint64_t ts_sub1_end = que.second.at(1)->GetTimestampEnd();
	    if(ts_sub1_beg < ts_next_beg)
	      ts_next_beg = ts_sub1_beg;
	    if(ts_sub1_end < ts_next_end)
	      ts_next_end = ts_sub1_end;
	    break;
	  }
	  else{//(que.second.size()==1){
	    ready = false;
	    break;
	  }
	}
	if(subev)
	  ev_wrap->AddSubEvent(subev);
      }
      WriteEvent(std::move(ev_wrap));
      m_ts_last_end = m_ts_curr_end;
      m_ts_curr_beg = ts_next_beg;
      m_ts_curr_end = ts_next_end;

      ready_c = 0;
      for(auto &ready :m_event_ready){
	if(ready.second)
	  ready_c ++;
      }      
    }
  }
}
