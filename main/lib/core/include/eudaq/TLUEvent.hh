#ifndef EUDAQ_INCLUDED_TLUEvent
#define EUDAQ_INCLUDED_TLUEvent

#include <vector>
#include "Event.hh"
#include "Platform.hh"

namespace eudaq {

  class DLLEXPORT TLUEvent : public Event {
  public:
    virtual void Serialize(Serializer &) const;
    explicit TLUEvent(uint32_t id_stream, unsigned run, unsigned event)
      :Event(Event::str2id("_TLU"), run, id_stream){
      SetEventN(event);
    }
    explicit TLUEvent(Deserializer &);
    virtual void Print(std::ostream &os, size_t offset = 0) const;

  private:
    std::vector<uint64_t> m_extratimes;
  };
}

#endif // EUDAQ_INCLUDED_TLUEvent
