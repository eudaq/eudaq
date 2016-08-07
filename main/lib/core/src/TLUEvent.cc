#include "eudaq/TLUEvent.hh"

#include <ostream>

namespace eudaq {

  namespace{
    auto dummy0 = Factory<Event>::Register<TLUEvent, Deserializer&>(Event::str2id("_TLU"));
  }

  
  EUDAQ_DEFINE_EVENT(TLUEvent, str2id("_TLU"));

  TLUEvent::TLUEvent(Deserializer &ds) : Event(ds) { ds.read(m_extratimes); }

  void TLUEvent::Print(std::ostream & os) const {
    Print(os, 0);
  }

  void TLUEvent::Print(std::ostream &os, size_t offset) const
  {
    os << std::string(offset, ' ') << "<TLUEvent> \n";
    Event::Print(os,offset+2);
    if (m_extratimes.size() > 0) {
      os << std::string(offset+2, ' ') << "<extraTimes>" << m_extratimes.size() << "</extraTimes>\n";
    }
    os << std::string(offset, ' ') << "</TLUEvent>\n";
  }

  void TLUEvent::Serialize(Serializer &ser) const {
    Event::Serialize(ser);
    ser.write(m_extratimes);
  }
}
