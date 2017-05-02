#include "eudaq/RawEvent.hh"

namespace eudaq {  
  namespace{
    auto dummy0 = Factory<Event>::Register<RawEvent, Deserializer&>(RawEvent::m_id_factory);
    auto dummy3 = Factory<Event>::Register<RawEvent>(RawEvent::m_id_factory);
  }
  
  RawEvent::RawEvent(){
    SetType(m_id_factory);
  }
  
  RawEvent::RawEvent(Deserializer &ds)
    :Event(ds){
  }  
}

  // RawDataEvent::RawDataEvent(const std::string& dspt, uint32_t dev_n, uint32_t run_n, uint32_t ev_n)
  //   : Event(cstr2hash("RawDataEvent"), run_n, dev_n){
  //   SetEventN(ev_n);
  //   SetExtendWord(eudaq::str2hash(dspt));
  //   SetDescription(dspt);
  // }
