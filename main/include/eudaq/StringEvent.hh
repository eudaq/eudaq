#ifndef EUDAQ_INCLUDED_StringEvent
#define EUDAQ_INCLUDED_StringEvent

#include <vector>
#include "eudaq/Event.hh"

namespace eudaq {

  /** An Event type consisting of just a string.
   *
   */
  class StringEvent : public Event {
    EUDAQ_DECLARE_EVENT(StringEvent);
    public:
    virtual void Serialize(Serializer &) const;
    StringEvent(unsigned run, unsigned event, const std::string & str) :
      Event(run, event),
      m_str(str) {}
    StringEvent(Deserializer &);
    virtual void Print(std::ostream &) const;

    /// Return "StringEvent" as type.
    virtual std::string GetType() const {return "StringEvent";}

    static StringEvent BORE(unsigned run) {
      return StringEvent(run);
    }
    static StringEvent EORE(unsigned run, unsigned event) {
      return StringEvent(run, event);
    }
    private:
    StringEvent(unsigned run, unsigned event = 0)
      : Event(run, event, NOTIMESTAMP, event ? Event::FLAG_EORE : Event::FLAG_BORE)
      {}
    std::string m_str;
  };

}

#endif // EUDAQ_INCLUDED_StringEvent
