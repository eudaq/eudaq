#include "eudaq/PointerEvent.hh"

namespace eudaq{
  EUDAQ_DEFINE_EVENT(PointerEvent, str2id("_POI"));

  PointerEvent::PointerEvent(reference_t filePointer, counter_t counter) : Event(0, counter,filePointer)
  {
    // The Timestamp slot is used for storing the pointer. 
    // since there is only one pointer there 
    // is no need to store a vector of timestamps. 
    // This statement resets the EUDAQ 2 flag an by this only the
    // first timestamp gets stored. 
   // ClearFlags(); 
  }

  PointerEvent::PointerEvent(Deserializer & des) :Event(des)
  {

  }



  void PointerEvent::Serialize(Serializer & ser) const
  {
    Event::Serialize(ser);
  }

}