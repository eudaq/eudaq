#include "TLUEvent.hh"

#include <ostream>

namespace eudaq {

  namespace{
    auto dummy1 = Factory<Event>::Register<TLUEvent, Deserializer&>(cstr2hash("TLUEvent"));
  }

  TLUEvent::TLUEvent(Deserializer &ds) : RawDataEvent(ds){}  
  TLUEvent::TLUEvent(uint32_t dev_n, uint32_t run_n, uint32_t ev_n)
    :RawDataEvent("TLUEvent", dev_n, run_n, ev_n){
  }
  
  void TLUEvent::Print(std::ostream &os, size_t offset) const{
    os << std::string(offset, ' ') << "<TLUEvent> \n";
    Event::Print(os,offset+2);
    os << std::string(offset, ' ') << "</TLUEvent>\n";
  }

  void TLUEvent::Serialize(Serializer &ser) const {
    Event::Serialize(ser);
  }
}
