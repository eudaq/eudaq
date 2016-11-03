#ifndef EUDAQ_INCLUDED_TLUEvent
#define EUDAQ_INCLUDED_TLUEvent

#include "eudaq/RawDataEvent.hh"
#include "eudaq/Platform.hh"

namespace eudaq {
  class DLLEXPORT TLUEvent : public RawDataEvent {
  public:
    explicit TLUEvent(uint32_t dev_n, uint32_t run_n, uint32_t ev_n);
    explicit TLUEvent(Deserializer &);
    void Serialize(Serializer &) const override;
    void Print(std::ostream &os, size_t offset = 0) const override;    
  };
}

#endif // EUDAQ_INCLUDED_TLUEvent
