#include "DetectorEvent.hh"
#include "RawDataEvent.hh"

#include <ostream>
#include <memory>



namespace eudaq {

  namespace{
    auto dummy1 = Factory<Event>::Register<DetectorEvent, Deserializer&>(cstr2hash("DetectorEvent"));
  }
  
  DetectorEvent::DetectorEvent(Deserializer &ds) : Event(ds) {
    SetFlag(Event::FLAG_PACKET);
  }

  DetectorEvent::DetectorEvent(uint32_t run_n, uint32_t ev_n, uint64_t ts)
    : Event(cstr2hash("DetectorEvent"), run_n, 0){ //stream_n = 0
    SetEventN(ev_n);
    SetTimestamp(ts, ts+1);
  }


  void DetectorEvent::Serialize(Serializer & ser) const {
    Event::Serialize(ser);
  }
  
  void DetectorEvent::AddEvent(EventSPC evt) {
    if (!evt) EUDAQ_THROW("Adding null event!");
    SetFlag(evt->GetFlag());
    AddSubEvent(evt);
  }

  void DetectorEvent::Print(std::ostream &os, size_t offset) const{
    os << std::string(offset, ' ') << "<DetectorEvent> \n";
    Event::Print(os,offset+2);
    os << std::string(offset, ' ') << "</DetectorEvent> \n";
  }

  const Event* DetectorEvent::GetEvent(size_t i) const {
    return GetSubEvent(i).get();
  }

  const RawDataEvent & DetectorEvent::GetRawSubEvent(const std::string & subtype, int n) const {
    for (size_t i = 0; i < NumEvents(); i++) {
      const RawDataEvent * sev = dynamic_cast<const RawDataEvent*>(GetEvent(i));
      if (sev && sev->GetSubType() == subtype) {
        if (n > 0) {
          --n;
        } else {
          return *sev;
        }
      }
    }
    EUDAQ_THROW("DetectorEvent::GetRawSubEvent: could not find " + subtype + ":" + to_string(n));
  }
}
