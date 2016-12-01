#include "DetectorEvent.hh"
#include "RawDataEvent.hh"

#include <ostream>
#include <memory>



namespace eudaq {

  namespace{
    auto dummy1 = Factory<Event>::Register<DetectorEvent, Deserializer&>(cstr2hash("DetectorEvent"));
    auto dummy8 = Factory<Event>::Register<Event, const uint32_t&, const uint32_t&, const uint32_t&>(cstr2hash("DetectorEvent"));  //NOTE: it is not Detector constructor
    
  }
  
  DetectorEvent::DetectorEvent(Deserializer &ds) : Event(ds){
    
  }

  DetectorEvent::DetectorEvent(uint32_t run_n, uint32_t ev_n, uint64_t ts)
    : Event(cstr2hash("DetectorEvent"), run_n, 0){ //stream_n = 0
    SetFlag(Event::FLAG_PACKET);
    SetEventN(ev_n);
    SetTimestamp(ts, ts+1);
  }

  void DetectorEvent::Print(std::ostream &os, size_t offset) const{
    os << std::string(offset, ' ') << "<DetectorEvent> \n";
    Event::Print(os,offset+2);
    os << std::string(offset, ' ') << "</DetectorEvent> \n";
  }
}
