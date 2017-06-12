#ifndef EUDAQ_INCLUDED_RawEvent
#define EUDAQ_INCLUDED_RawEvent

#include "eudaq/Event.hh"

namespace eudaq {

  class DLLEXPORT RawEvent : public Event {
  public:
    RawEvent();
    RawEvent(Deserializer &);    
    static const uint32_t m_id_factory = cstr2hash("RawEvent");
  };
  using RawDataEvent = RawEvent;
}

#endif


