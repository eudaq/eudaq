#include "eudaq/DetectorEvent.hh"

#include <ostream>

namespace eudaq {

  EUDAQ_DEFINE_EVENT(DetectorEvent, str2id("_DET"));

  DetectorEvent::DetectorEvent(Deserializer & ds) :
    Event(ds)
  {
    unsigned n;
    ds.read(n);
    //std::cout << "Num=" << n << std::endl;
    for (size_t i = 0; i < n; ++i) {
      counted_ptr<Event> ev(EventFactory::Create(ds));
      m_events.push_back(ev);
    }
  }

  void DetectorEvent::AddEvent(counted_ptr<Event> evt) {
    m_events.push_back(evt);
    SetFlags(evt->GetFlags());
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


}
