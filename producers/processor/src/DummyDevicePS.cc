#include "Processor.hh"
#include "Event.hh"

namespace eudaq{

  class DummyDevicePS: public Processor{
  public:
    DummyDevicePS(std::string cmd);
    virtual void ProcessUserEvent(EVUP ev);
    // virtual void ProcessUsrCmd(const std::string cmd_name, const std::string cmd_par);
    
  private:
    uint32_t m_event_n;
    uint32_t m_duration;
    uint64_t m_ts_last_end;
  };


  namespace{
  auto dummy0 = Factory<Processor>::Register<DummyDevicePS, std::string&>(eudaq::cstr2hash("DummyDevicePS"));
  auto dummy1 = Factory<Processor>::Register<DummyDevicePS, std::string&&>(eudaq::cstr2hash("DummyDevicePS"));
  }

  DummyDevicePS::DummyDevicePS(std::string cmd)
    :Processor("DummyDevicePS", ""){
    ProcessCmd(cmd);
    m_event_n = 0;
    m_duration = 1;
  }

  void DummyDevicePS::ProcessUserEvent(EVUP ev){
    EVUP devev = Factory<Event>::Create<const uint32_t&, const uint32_t&, const uint32_t&>(cstr2hash("DUMMYDEV"), cstr2hash("DUMMYDEV"), 0, GetID());
    if(ev->IsBORE() || ev->IsEORE()){
      ForwardEvent(std::move(ev));
      return;
    }
    
    uint64_t ts = ev->GetTimestampBegin();
    devev->SetEventN(m_event_n++);
    devev->SetTimestampBegin(ts);
    devev->SetTimestampEnd(ts+m_duration);
    ForwardEvent(std::move(devev));
  }
  
}
