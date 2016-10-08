#include "DetectorEvent.hh"
#include "RawDataEvent.hh"

#include <ostream>
#include <memory>



namespace eudaq {

  namespace{
    auto dummy0 = Factory<Event>::Register<DetectorEvent, Deserializer&>(Event::str2id("_DET"));
  }
  

  DetectorEvent::DetectorEvent(Deserializer &ds) : Event(ds) {
    unsigned n;
    ds.read(n);
    SetFlag(Event::FLAG_PACKET);
    for (size_t i = 0; i < n; ++i) {
      uint32_t id;
      ds.PreRead(id);
      EventSP ev = Factory<Event>::Create<Deserializer&>(id, ds);
      m_events.push_back(ev);
    }
  }

  DetectorEvent::DetectorEvent(unsigned runnumber, unsigned eventnumber, uint64_t timestamp)
    : Event(Event::str2id("_DET"), runnumber, 0){ //stream_n = 0
    SetEventN(eventnumber);
    SetTimestampBegin(timestamp);
    SetTimestampEnd(timestamp);
  }


  void DetectorEvent::Serialize(Serializer & ser) const {
    Event::Serialize(ser);
    ser.write((unsigned)m_events.size());
    for (size_t i = 0; i < m_events.size(); ++i) {
      m_events[i]->Serialize(ser);
    }
  }

  
  void DetectorEvent::AddEvent(EventSP evt) {
    if (!evt.get()) EUDAQ_THROW("Adding null event!");
    SetFlag(evt->GetFlag());
    m_events.push_back(std::move(evt)); 
  }

  void DetectorEvent::Print(std::ostream &os, size_t offset) const
  {
    os << std::string(offset, ' ') << "<DetectorEvent> \n";
    Event::Print(os,offset+2);
    os <<std::string(offset+2,' ') << "<SubEvents> \n";
    for (size_t i = 0; i < NumEvents(); ++i) {
      GetEvent(i)->Print(os,offset+4) ;
    }
    os << std::string(offset+2, ' ') << "</SubEvents> \n";

    os << std::string(offset, ' ') << "</DetectorEvent> \n";
  }


  const Event * DetectorEvent::GetEvent(size_t i) const {
    return m_events[i].get();
  }

  EventSP DetectorEvent::GetEventPtr(size_t i) const {
    return m_events[i];
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


 
  size_t DetectorEvent::NumEvents() const {
    return m_events.size();
  }
}
