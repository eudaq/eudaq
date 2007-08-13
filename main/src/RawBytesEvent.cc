#include "eudaq/RawBytesEvent.hh"

#include <ostream>

namespace eudaq {

  EUDAQ_DEFINE_EVENT(RawBytesEvent, str2id("_RAW"));

  RawBytesEvent::RawBytesEvent(Deserializer & ds) :
    Event(ds)
  {
    ds.read(m_data);
  }

  void RawBytesEvent::Print(std::ostream & os) const {
    Event::Print(os);
    os << ", " << m_data.size() << " bytes";
  }

  void RawBytesEvent::Serialize(Serializer & ser) const {
    Event::Serialize(ser);
    ser.write(m_data);
  }

}
