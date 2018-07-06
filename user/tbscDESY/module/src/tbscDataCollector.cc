#include "eudaq/DataCollector.hh"
#include "eudaq/Time.hh"
#include <mutex>
#include <deque>
#include <map>
#include <set>

class tbscDataCollector:public eudaq::DataCollector{
public:
  tbscDataCollector(const std::string &name,
		   const std::string &rc);
  void DoConnect(eudaq::ConnectionSPC id) override;
  void DoDisconnect(eudaq::ConnectionSPC id) override;
  void DoReceive(eudaq::ConnectionSPC id, eudaq::EventSP ev) override;
  void DoConfigure() override;
  
  static const uint32_t m_id_factory = eudaq::cstr2hash("tbscDataCollector");
private:
  uint32_t m_noprint;
  std::filebuf m_fb; // output file to print event

  std::mutex m_mtx_map;
  std::map<eudaq::ConnectionSPC, std::deque<eudaq::EventSPC>> m_conn_evque;
  std::set<eudaq::ConnectionSPC> m_conn_inactive;
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::DataCollector>::
    Register<tbscDataCollector, const std::string&, const std::string&>
    (tbscDataCollector::m_id_factory);
}

tbscDataCollector::tbscDataCollector(const std::string &name,
				       const std::string &rc):
  DataCollector(name, rc){
}

void tbscDataCollector::DoConfigure(){
  m_noprint = 0;
  auto conf = GetConfiguration();
  if (conf) {
    conf->Print();
    m_noprint = conf->Get("DISABLE_PRINT", 0);
    m_fb.open("out.txt", std::ios::out);
  }

}

void tbscDataCollector::DoConnect(eudaq::ConnectionSPC idx){
  std::unique_lock<std::mutex> lk(m_mtx_map);
  m_conn_evque[idx].clear();
  m_conn_inactive.erase(idx);
}

void tbscDataCollector::DoDisconnect(eudaq::ConnectionSPC idx){
  std::unique_lock<std::mutex> lk(m_mtx_map);
  m_conn_inactive.insert(idx);
}

void tbscDataCollector::DoReceive(eudaq::ConnectionSPC idx, eudaq::EventSP ev){

  std::cout<<eudaq::Time::Current().Formatted()<<std::endl;

  if (!m_noprint){
    std::ostream os(&m_fb);
    ev->Print(std::cout);
    ev->Print(os, 0);
  }
  WriteEvent(std::move(ev));
  
}

/*
<<<<<<< HEAD
void tbscDataCollector::DoReceive(eudaq::ConnectionSPC idx, eudaq::EventUP ev){  
=======
void tbscDataCollector::DoReceive(eudaq::ConnectionSPC idx, eudaq::EventSP ev){  
>>>>>>> tbsc.standalone
  std::unique_lock<std::mutex> lk(m_mtx_map);
  eudaq::EventSP evsp = std::move(ev);
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
      if(trigger_n_ev< trigger_n)
	trigger_n = trigger_n_ev;
    }
  }

  auto ev_sync = eudaq::Event::MakeUnique("SCRawEvt");
  ev_sync->SetFlagPacket();
  ev_sync->SetTriggerN(trigger_n);
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
  ev_sync->Print(std::cout);
  WriteEvent(std::move(ev_sync));
}
*/
