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
    void Serialize(Serializer &) const override;
    void Print(std::ostream &os, size_t offset = 0) const override;
    void AddEvent(EventSPC evt);
    size_t NumEvents() const {return GetNumSubEvent();};
    const Event *GetEvent(size_t i) const;
    EventSP GetEventPtr(size_t i) const {return std::const_pointer_cast<Event>(GetSubEvent(i));};
    const RawDataEvent &GetRawSubEvent(const std::string &subtype,
                                       int n = 0) const;
  };
}

#endif
