#include "eudaq/DataCollector.hh"
#include "eudaq/RawDataEvent.hh"

#include <deque>
#include <map>
#include <string>

namespace eudaq {
  class EventnumberSyncDataCollector :public DataCollector{
  public:
    using DataCollector::DataCollector;
    void DoStartRun() override;
    void DoConnect(uint32_t /*id*/) override;
    void DoDisconnect(uint32_t /*id*/) override;
    void DoReceive(uint32_t id, EventUP ev) override;
    static const uint32_t m_id_factory = eudaq::cstr2hash("EventnumberSyncDataCollector");
  private:
    std::map<std::string, std::deque<EventSPC>> m_que_event;
    std::mutex m_mtx_map;
  };

  namespace{
    auto dummy0 = Factory<DataCollector>::
      Register<EventnumberSyncDataCollector, const std::string&, const std::string&>
      (EventnumberSyncDataCollector::m_id_factory);
  }

  void EventnumberSyncDataCollector::DoStartRun(){
    std::unique_lock<std::mutex> lk(m_mtx_map);
    for(auto &que :m_que_event){
      que.second.clear();
    }
  }
  
  void EventnumberSyncDataCollector::DoConnect(uint32_t id){
    std::unique_lock<std::mutex> lk(m_mtx_map);
    std::string pdc_name = std::to_string(id);
    EUDAQ_INFO("Producer."+pdc_name+" is connecting");
    if(m_que_event.find(pdc_name) != m_que_event.end())
      EUDAQ_THROW("DataCollector::Doconnect, multiple producers are sharing a same name");
    m_que_event[pdc_name];
  }
  
  void EventnumberSyncDataCollector::DoDisconnect(uint32_t id){
    std::unique_lock<std::mutex> lk(m_mtx_map);
    std::string pdc_name = std::to_string(id);
    if(m_que_event.find(pdc_name) == m_que_event.end())
      EUDAQ_THROW("DataCollector::DisDoconnect, the disconnecting producer was not existing in list");
    EUDAQ_WARN("Producer."+pdc_name+" is disconnected, the remaining events are erased. ("+std::to_string(m_que_event[pdc_name].size())+ " Events)");
    m_que_event.erase(pdc_name);
  }
  
  void EventnumberSyncDataCollector::DoReceive(uint32_t id, EventUP ev){
    std::unique_lock<std::mutex> lk(m_mtx_map);
    std::string pdc_name = std::to_string(id);
    m_que_event[pdc_name].push_back(std::move(ev));
    uint32_t n = 0;
    for(auto &que :m_que_event){
      if(!que.second.empty())
	n++;
    }
    if(n==m_que_event.size()){
      auto ev_wrap = RawDataEvent::MakeUnique(GetFullName());
      ev_wrap->SetFlagPacket();
      uint32_t ev_c = m_que_event.begin()->second.front()->GetEventN();
      bool match = true;
      for(auto &que :m_que_event){
	if(ev_c != que.second.front()->GetEventN())
	  match = false;
	ev_wrap->AddSubEvent(que.second.front());
	que.second.pop_front();
      }
      if(!match){
	EUDAQ_WARN("EventNumbers are Mismatched");
      }
      WriteEvent(std::move(ev_wrap));
    }
  }
}
