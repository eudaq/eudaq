#ifndef EUDAQ_INCLUDED_StringEvent
#define EUDAQ_INCLUDED_StringEvent

#include <vector>
#include "Event.hh"

namespace eudaq {

/** An Event type consisting of just a string.
 *
 */
class DLLEXPORT StringEvent : public Event {
public:
  virtual void Serialize(Serializer &) const;
  StringEvent(uint32_t id_stream, unsigned run, unsigned event, const std::string & str);
  StringEvent(Deserializer &);
  virtual void Print(std::ostream &) const;
  virtual void Print(std::ostream &os, size_t offset) const;

  virtual std::string GetType() const;

private:
  std::string m_str;
};

}

#endif // EUDAQ_INCLUDED_StringEvent
