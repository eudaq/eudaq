#include "eudaq/DataCollector.hh"
#include <mutex>
#include <deque>
#include <map>
#include <set>

class DualROCaloDataCollector:public eudaq::DataCollector{
public:
  DualROCaloDataCollector(const std::string &name,const std::string &rc);

  void DoConnect(eudaq::ConnectionSPC id) override;
  void DoDisconnect(eudaq::ConnectionSPC id) override;
  void DoReceive(eudaq::ConnectionSPC id, eudaq::EventSP ev) override;
  static const uint32_t m_id_factory = eudaq::cstr2hash("DualROCaloDataCollector");

private:
  std::mutex m_mtx_map;
  std::map<eudaq::ConnectionSPC, std::deque<eudaq::EventSPC>> m_conn_evque;
  std::set<eudaq::ConnectionSPC> m_conn_inactive;
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::DataCollector>::
  Register<DualROCaloDataCollector, const std::string&, const std::string&>(DualROCaloDataCollector::m_id_factory);
}

DualROCaloDataCollector::DualROCaloDataCollector(const std::string &name,
 const std::string &rc):
 DataCollector(name, rc){
}

void DualROCaloDataCollector::DoConnect(eudaq::ConnectionSPC idx){
  std::unique_lock<std::mutex> lk(m_mtx_map);
  m_conn_evque[idx].clear();
  m_conn_inactive.erase(idx);
}

void DualROCaloDataCollector::DoDisconnect(eudaq::ConnectionSPC idx){
  std::unique_lock<std::mutex> lk(m_mtx_map);
  m_conn_inactive.insert(idx);
  if(m_conn_inactive.size() == m_conn_evque.size()){
    m_conn_inactive.clear();
    m_conn_evque.clear();
  }
}

void DualROCaloDataCollector::DoReceive(eudaq::ConnectionSPC idx, eudaq::EventSP evsp){
  std::unique_lock<std::mutex> lk(m_mtx_map);
  if(!evsp->IsFlagTrigger()){
    EUDAQ_THROW("!evsp->IsFlagTrigger()");
  }

  m_conn_evque[idx].push_back(evsp);
  uint32_t trigger_n = -1;

  for(auto &conn_evque: m_conn_evque){
    if(conn_evque.second.empty())
      return;
    else{
      uint32_t trigger_n_ev = conn_evque.second.front()->GetTriggerN();
        if(trigger_n_ev< trigger_n) trigger_n = trigger_n_ev;
    }
  }

  auto ev_sync = eudaq::Event::MakeUnique("DualROCalo");
  ev_sync->SetFlagPacket();
  ev_sync->SetTriggerN(trigger_n);
  //EUDAQ_DEBUG("DualROCaloDataCollector:: Set Trigger Number");

  for(auto &conn_evque: m_conn_evque){
    auto &ev_front = conn_evque.second.front();
    if(ev_front->GetTriggerN() == trigger_n){
      ev_sync->AddSubEvent(ev_front);
      conn_evque.second.pop_front();
    }
  }

  if(!m_conn_inactive.empty()){
    std::set<eudaq::ConnectionSPC> conn_inactive_empty;

    for(auto &conn: m_conn_inactive){
      if(m_conn_evque.find(conn) != m_conn_evque.end() &&
      m_conn_evque[conn].empty()){
        m_conn_evque.erase(conn);
        conn_inactive_empty.insert(conn);
      }
    }

    for(auto &conn: conn_inactive_empty){
      m_conn_inactive.erase(conn);
    }

  }

  //ev_sync->Print(std::cout);
  WriteEvent(std::move(ev_sync));
}