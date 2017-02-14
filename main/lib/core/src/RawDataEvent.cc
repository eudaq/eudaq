#include "eudaq/RawDataEvent.hh"
#include "eudaq/Logger.hh"
#include <string>

namespace eudaq {

  namespace{
    auto dummy0 = Factory<Event>::Register<RawDataEvent, Deserializer&>(RawDataEvent::m_id_factory);
    auto dummy3 = Factory<Event>::Register<RawDataEvent>(RawDataEvent::m_id_factory);
    // auto &cccout = (std::cout<<"Resisted to Factory<"
    // 		    <<std::hex<<dummy2<<std::dec
    // 		    <<"> ID "<<RawDataEvent::m_id_factory<<std::endl);
  }
  
  RawDataEvent::RawDataEvent(const std::string& dspt, uint32_t dev_n, uint32_t run_n, uint32_t ev_n)
    : Event(cstr2hash("RawDataEvent"), run_n, dev_n){
    SetEventN(ev_n);
    SetExtendWord(eudaq::str2hash(dspt));
    SetDescription(dspt);
  }

  RawDataEvent::RawDataEvent(Deserializer &ds) : Event(ds) {
  }

  
}
