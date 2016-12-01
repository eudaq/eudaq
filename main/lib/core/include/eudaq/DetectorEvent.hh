#ifndef EUDAQ_INCLUDED_DetectorEvent
#define EUDAQ_INCLUDED_DetectorEvent

#include <vector>
#include <memory>

#include "eudaq/Event.hh"
#include "eudaq/RawDataEvent.hh"

namespace eudaq {
  class RawDataEvent;

  class DLLEXPORT DetectorEvent:public Event {
  public:
    explicit DetectorEvent(uint32_t run_n, uint32_t ev_n,
                           uint64_t ts);
    explicit DetectorEvent(Deserializer &);
    void Print(std::ostream &os, size_t offset = 0) const override;

    static const uint32_t m_id_factory = cstr2hash("StandardEvent");

  };
}

#endif
