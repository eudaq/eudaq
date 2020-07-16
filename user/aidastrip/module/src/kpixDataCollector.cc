#include "eudaq/DataCollector.hh"
#include <mutex>
#include <deque>
#include <map>
#include <set>

class kpixDataCollector:public eudaq::DataCollector{
public:
  kpixDataCollector(const std::string &name,
		   const std::string &rc);
  void DoConfigure() override;
  void DoConnect(eudaq::ConnectionSPC id) override;
  void DoDisconnect(eudaq::ConnectionSPC id) override;
  void DoReceive(eudaq::ConnectionSPC id, eudaq::EventSP ev) override;

  static const uint32_t m_id_factory = eudaq::cstr2hash("kpixDataCollector");
private:
  uint32_t m_noprint;
  std::mutex m_mtx_map;
  std::map<eudaq::ConnectionSPC, std::deque<eudaq::EventSPC>> m_conn_evque;
  std::set<eudaq::ConnectionSPC> m_conn_inactive;
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::DataCollector>::
    Register<kpixDataCollector, const std::string&, const std::string&>
    (kpixDataCollector::m_id_factory);
}

kpixDataCollector::kpixDataCollector(const std::string &name,
				       const std::string &rc):
  DataCollector(name, rc){
}

void kpixDataCollector::DoConfigure(){
  m_noprint = 0;
  auto conf = GetConfiguration();
  if (conf) {
    conf->Print();
    m_noprint = conf->Get("DISABLE_PRINT", 0);
  }
  
}

void kpixDataCollector::DoConnect(eudaq::ConnectionSPC idx){
  std::unique_lock<std::mutex> lk(m_mtx_map);
  m_conn_evque[idx].clear();
  m_conn_inactive.erase(idx);
}

void kpixDataCollector::DoDisconnect(eudaq::ConnectionSPC idx){
  std::unique_lock<std::mutex> lk(m_mtx_map);
  m_conn_inactive.insert(idx);
}

void kpixDataCollector::DoReceive(eudaq::ConnectionSPC idx, eudaq::EventSP ev){
  if (!m_noprint)
    ev->Print(std::cout);
  WriteEvent(std::move(ev));
  // do sth
}

