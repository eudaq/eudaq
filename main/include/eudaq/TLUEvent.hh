#ifndef EUDAQ_INCLUDED_TLUEvent
#define EUDAQ_INCLUDED_TLUEvent

#include <vector>
#include "eudaq/Event.hh"
#include "eudaq/Platform.hh"

namespace eudaq {

  class DLLEXPORT TLUEvent : public Event {
    EUDAQ_DECLARE_EVENT(TLUEvent);

  public:
    typedef std::vector<uint64_t> vector_t;
    virtual void Serialize(Serializer &) const;
    explicit TLUEvent(unsigned run, unsigned event, uint64_t timestamp,
                      const vector_t &extratimes = vector_t())
        : Event(run, event, timestamp), m_extratimes(extratimes) {}
    explicit TLUEvent(Deserializer &);
    virtual void Print(std::ostream &) const;

    /// Return "TLUEvent" as type.
    virtual std::string GetType() const { return "TLUEvent"; }

    static TLUEvent BORE(unsigned run) { return TLUEvent(run); }
    static TLUEvent EORE(unsigned run, unsigned event) {
      return TLUEvent(run, event);
    }

  private:
    TLUEvent(unsigned run, unsigned event = 0)
        : Event(run, event, NOTIMESTAMP,
                event ? Event::FLAG_EORE : Event::FLAG_BORE) {}
    vector_t m_extratimes;
  };
}

#endif // EUDAQ_INCLUDED_TLUEvent
