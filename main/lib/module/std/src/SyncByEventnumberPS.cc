#define NOMINMAX
#include "Processor.hh"
#include "Event.hh"

#include <map>
#include <deque>
#include <stdexcept>

using namespace eudaq;
class SyncByEventNumberPS: public Processor{
public:
  SyncByEventNumberPS();
  ~SyncByEventNumberPS() override;
  void ProcessEvent(EventSPC ev) final override;
  void ProcessCommand(const std::string& key, const std::string& val) final override;

  static constexpr const char* m_description = "SyncByEventNumberPS";
private:
  std::map<uint32_t, std::deque<EventSPC>> m_fifos;
  uint32_t m_main_dev;
  uint32_t m_n_dev;
};

namespace{
  auto dummy0 = Factory<Processor>::
    Register<SyncByEventNumberPS>(cstr2hash(SyncByEventNumberPS::m_description));
}


SyncByEventNumberPS::SyncByEventNumberPS():Processor(SyncByEventNumberPS::m_description){
  m_main_dev = 0;
}

SyncByEventNumberPS::~SyncByEventNumberPS(){
  //TODO:: build the remain events
  //Hub may be deleted. update hub  Processor.cc
}

void SyncByEventNumberPS::ProcessEvent(EventSPC ev){  
  uint32_t dev_n = ev->GetStreamN();
  auto it = m_fifos.find(dev_n);
  if(it==m_fifos.end())
    return;

  it->second.push_back(ev);

  size_t n_ready = 0;
  for(auto &e: m_fifos){
    if(!e.second.empty())
      n_ready ++;
  }
  if(n_ready < m_fifos.size() || n_ready < m_n_dev)
    return;

  uint32_t min_ev_n = 0;
  for(auto &e: m_fifos){
    auto &fifo = e.second;
    min_ev_n = std::min(min_ev_n, fifo.front()->GetEventN());
  }

  EventSP sync_ev = Factory<Event>::MakeShared<const uint32_t&, const uint32_t&, const uint32_t&>(cstr2hash("SYNC"), cstr2hash("SYNC"), 0, GetInstanceN());
  sync_ev->SetFlagPacket();

  for(auto &e: m_fifos){
    auto &fifo = e.second;
    auto devn = e.first;
    if(fifo.front()->GetEventN() == min_ev_n){
      sync_ev->AddSubEvent(fifo.front());
      if(devn == m_main_dev){
	auto &main_ev = fifo.front();
	sync_ev->SetRunN(main_ev->GetRunN());
	sync_ev->SetEventN(main_ev->GetEventN());
	sync_ev->SetTimestamp(main_ev->GetTimestampBegin(), main_ev->GetTimestampEnd());
      }
      fifo.pop_front();
    }
  }
  RegisterEvent(sync_ev);
  return;
}

void SyncByEventNumberPS::ProcessCommand(const std::string& key, const std::string& val){
  switch(str2hash(key)){
  case cstr2hash("STREAM:SIZE"):{
    uint32_t m_n_dev = std::stoul(val);
    break;
  }
  case cstr2hash("STREAM:ADD"):{
    uint32_t dev_n = std::stoul(val);
    auto it = m_fifos.find(dev_n);
    if(it == m_fifos.end()){
      m_fifos[dev_n];
    }
    break;
  }
  case cstr2hash("STREAM:DEL"):{
    uint32_t dev_n = std::stoul(val);
    auto it = m_fifos.find(dev_n);
    if(it != m_fifos.end()){
      m_fifos.erase(dev_n);
    }
    break;
  }
  case cstr2hash("STREAM:CLEAR"):{
    m_fifos.clear();
    break;
  }
  case cstr2hash("STREAM:MAIN"):{
    m_main_dev = std::stoul(val);
    break;
  }
  case cstr2hash("EVENT:CLEAR"):{
    for(auto &e: m_fifos)
      e.second.clear();
    break;
  }
  default:
    std::cout<<"unkonw user cmd"<<std::endl;
  }
}
