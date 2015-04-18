#include "eudaq/PointerEvent.hh"

namespace eudaq{
  EUDAQ_DEFINE_EVENT(PointerEvent, str2id("_POI"));

  PointerEvent::PointerEvent(reference_t filePointer, counter_t counter) : Event(0, counter,filePointer)
  {

  }

  PointerEvent::PointerEvent(Deserializer & des) :Event(des)
  {

  }



  void PointerEvent::Serialize(Serializer &) const
  {

  }

}