#ifndef EUDAQ_INCLUDED_StringEvent
#define EUDAQ_INCLUDED_StringEvent

#include <vector>
#include "eudaq/Event.hh"

namespace eudaq {

/** An Event type consisting of just a string.
 *
 */
class DLLEXPORT StringEvent : public Event {
  EUDAQ_DECLARE_EVENT(StringEvent);

public:
  virtual void Serialize(Serializer &) const;
  StringEvent(unsigned run, unsigned event, const std::string & str);
  StringEvent(Deserializer &);
  virtual void Print(std::ostream &) const;
  virtual void Print(std::ostream &os, size_t offset) const;

  virtual std::string GetType() const;

  static StringEvent BORE(unsigned run);
  static StringEvent EORE(unsigned run, unsigned event);
private:
  StringEvent(unsigned run, unsigned event = 0);
  std::string m_str;
};

}

#endif // EUDAQ_INCLUDED_StringEvent
