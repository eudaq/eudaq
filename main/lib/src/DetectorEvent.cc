#include "eudaq/DetectorEvent.hh"
#include "eudaq/RawDataEvent.hh"

#include <ostream>
#include <memory>

namespace eudaq {

  EUDAQ_DEFINE_EVENT(DetectorEvent, str2id(DetectorEventMaintype));

  DetectorEvent::DetectorEvent(Deserializer & ds) :
    Event(ds)
  {
    unsigned n;
    ds.read(n);
    SetFlags(Event::FLAG_PACKET);
    //std::cout << "Num=" << n << std::endl;
    for (size_t i = 0; i < n; ++i) {
      std::shared_ptr<Event> ev(EventFactory::Create(ds));
      m_events.push_back(ev);
    }
  }

  DetectorEvent::DetectorEvent(const DetectorEvent& det) :Event(det.GetRunNumber(), det.GetEventNumber(), det.GetTimestamp(), det.GetFlags()), m_events(det.m_events)
  {
    m_tags = det.m_tags;
    m_timestamp = det.m_timestamp;

  }

  void DetectorEvent::AddEvent(std::shared_ptr<Event> evt) {
    if (!evt.get()) EUDAQ_THROW("Adding null event!");
    SetFlags(evt->GetFlags());
    m_events.push_back(std::move(evt));
    
  }

  void DetectorEvent::Print(std::ostream & os) const {
    Event::Print(os);
    os << " {\n";
    for (size_t i = 0; i < NumEvents(); ++i) {
      os << "  " << *GetEvent(i) << std::endl;
    }
    os << "}";
  }

  void DetectorEvent::Serialize(Serializer & ser) const {
    Event::Serialize(ser);
    ser.write((unsigned)m_events.size());
    for (size_t i = 0; i < m_events.size(); ++i) {
      m_events[i]->Serialize(ser);
    }
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

  event_sp DetectorEvent::ShallowCopy(const DetectorEvent& det)
  {
    return  event_sp(new DetectorEvent(det));
  }

}
