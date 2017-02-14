#ifndef EUDAQ_INCLUDED_RawDataEvent
#define EUDAQ_INCLUDED_RawDataEvent

#include <sstream>
#include <string>
#include <vector>
#include "Event.hh"
#include "Platform.hh"
namespace eudaq {


  class DLLEXPORT RawDataEvent : public Event {
  public:
    RawDataEvent(){};
    RawDataEvent(const std::string& dspt, uint32_t dev_n, uint32_t run_n, uint32_t ev_n);
    RawDataEvent(Deserializer &);
    
  public:
    static const uint32_t m_id_factory = cstr2hash("RawDataEvent");

  };
}

#endif // EUDAQ_INCLUDED_RawDataEvent
