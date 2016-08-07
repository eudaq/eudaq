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
    // std::cout << "Num=" << n << std::endl;
    SetFlags(Event::FLAG_PACKET);
    for (size_t i = 0; i < n; ++i) {
      uint32_t id;
      ds.read(id);
      EventSP ev = Factory<Event>::Create<Deserializer&>(id, ds);
      m_events.push_back(ev);
    }
  }

  DetectorEvent::DetectorEvent(const DetectorEvent& det) :Event(det.GetRunNumber(), det.GetEventNumber(), det.GetTimestamp(), det.GetFlags()), m_events(det.m_events)
  {
    m_tags = det.m_tags;
    m_timestamp = det.m_timestamp;

  }

  DetectorEvent::DetectorEvent(unsigned runnumber, unsigned eventnumber, uint64_t timestamp) : Event(runnumber, eventnumber, timestamp) {

  }

  void DetectorEvent::AddEvent(std::shared_ptr<Event> evt) {
    if (!evt.get()) EUDAQ_THROW("Adding null event!");
    SetFlags(evt->GetFlags());
    m_events.push_back(std::move(evt));
    
  }

  void DetectorEvent::Print(std::ostream & os) const {
    Print(os, 0);
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

  void DetectorEvent::Serialize(Serializer & ser) const {
    Event::Serialize(ser);
    ser.write((unsigned)m_events.size());
    for (size_t i = 0; i < m_events.size(); ++i) {
      m_events[i]->Serialize(ser);
    }
  }

  const Event * DetectorEvent::GetEvent(size_t i) const {
    return m_events[i].get();
  }

  Event * DetectorEvent::GetEvent(size_t i) {
    return m_events[i].get();
  }

  std::shared_ptr<Event> DetectorEvent::GetEventPtr(size_t i) const {
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

  void DetectorEvent::clearEvents() {
    m_events.clear();
  }

  event_sp DetectorEvent::ShallowCopy(const DetectorEvent& det)
  {
    return  event_sp(new DetectorEvent(det));
  }

  std::string DetectorEvent::GetType() const {
    return "DetectorEvent";
  }

  size_t DetectorEvent::NumEvents() const {
    return m_events.size();
  }

}
