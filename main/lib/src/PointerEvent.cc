#include "eudaq/PointerEvent.hh"

namespace eudaq{
  EUDAQ_DEFINE_EVENT(PointerEvent, str2id("_POI"));

  PointerEvent::PointerEvent(reference_t filePointer, counter_t counter) : Event(0, 0), m_ref(filePointer), m_counter(counter)
  {
  }

  PointerEvent::PointerEvent(Deserializer & des) :Event(0, 0)
  {
    des.read(m_ref);
    des.read(m_counter);
  }



  void PointerEvent::Serialize(Serializer & ser) const
  {
    ser.write(get_id());
    ser.write(m_ref);
    ser.write(m_counter);
  }

  void PointerEvent::Print(std::ostream & os) const
  {
    os << "\n PointerEvent \n";
    os << "reference = " << getReference() << "\n" << "counter = " << getCounter() << "\n";
  }

  std::string PointerEvent::GetSubType() const
  {
    return "";
  }

  PointerEvent::counter_t PointerEvent::getCounter() const
  {
    return m_counter;
  }

  PointerEvent::reference_t PointerEvent::getReference() const
  {
    return m_ref;
  }

  void PointerEvent::setCounter(unsigned newCounter)
  {
    m_counter = newCounter;
  }

  void PointerEvent::setReference(reference_t ref)
  {
    m_ref = ref;
  }

}