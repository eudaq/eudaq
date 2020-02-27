#include "eudaq/DataCollector.hh"
#include "eudaq/Time.hh"
#include <mutex>
#include <deque>
#include <map>
#include <set>

class WienerDataCollector:public eudaq::DataCollector{
public:
  WienerDataCollector(const std::string &name,
		   const std::string &rc);
  void DoConnect(eudaq::ConnectionSPC id) override;
  void DoDisconnect(eudaq::ConnectionSPC id) override;
  void DoReceive(eudaq::ConnectionSPC id, eudaq::EventSP ev) override;
  void DoConfigure() override;
  
  static const uint32_t m_id_factory = eudaq::cstr2hash("WienerDataCollector");
private:
  uint32_t m_noprint;
  std::filebuf m_fb; // output file to print event

  std::mutex m_mtx_map;
  std::map<eudaq::ConnectionSPC, std::deque<eudaq::EventSPC>> m_conn_evque;
  std::set<eudaq::ConnectionSPC> m_conn_inactive;
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::DataCollector>::
    Register<WienerDataCollector, const std::string&, const std::string&>
    (WienerDataCollector::m_id_factory);
}

WienerDataCollector::WienerDataCollector(const std::string &name,
				       const std::string &rc):
  DataCollector(name, rc){
}

void WienerDataCollector::DoConfigure(){
  m_noprint = 0;
  auto conf = GetConfiguration();
  if (conf) {
    conf->Print();
    m_noprint = conf->Get("DISABLE_PRINT", 0);
    m_fb.open("out.txt", std::ios::out);
  }

}

void WienerDataCollector::DoConnect(eudaq::ConnectionSPC idx){
  std::unique_lock<std::mutex> lk(m_mtx_map);
  m_conn_evque[idx].clear();
  m_conn_inactive.erase(idx);
}

void WienerDataCollector::DoDisconnect(eudaq::ConnectionSPC idx){
  std::unique_lock<std::mutex> lk(m_mtx_map);
  m_conn_inactive.insert(idx);
}

void WienerDataCollector::DoReceive(eudaq::ConnectionSPC idx, eudaq::EventSP ev){

  std::cout<<eudaq::Time::Current().Formatted()<<std::endl;

  if (!m_noprint){
    std::ostream os(&m_fb);
    ev->Print(std::cout);
    ev->Print(os, 0);
  }
  WriteEvent(std::move(ev));
  
}
