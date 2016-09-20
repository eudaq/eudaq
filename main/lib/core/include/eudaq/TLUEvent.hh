#ifndef EUDAQ_INCLUDED_TLUEvent
#define EUDAQ_INCLUDED_TLUEvent

#include <vector>
#include "eudaq/Event.hh"
#include "eudaq/Platform.hh"

namespace eudaq {

  class DLLEXPORT TLUEvent : public Event {
  public:
    typedef std::vector<uint64_t> vector_t;
    virtual void Serialize(Serializer &) const;
    explicit TLUEvent(uint32_t id_stream, unsigned run, unsigned event)
      :Event(id_stream, run, event){
      m_typeid = Event::str2id("_TLU");
    }
    explicit TLUEvent(Deserializer &);
    virtual void Print(std::ostream &) const;
    virtual void Print(std::ostream &os, size_t offset) const;
    /// Return "TLUEvent" as type.
    virtual std::string GetType() const { return "TLUEvent"; }

  private:
    vector_t m_extratimes;
  };
}

#endif // EUDAQ_INCLUDED_TLUEvent
