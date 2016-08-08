#include "StringEvent.hh"

#include <ostream>

namespace eudaq {

  namespace{
    auto dummy0 = Factory<Event>::Register<StringEvent, Deserializer&>(Event::str2id("_STR"));
  }


StringEvent::StringEvent(Deserializer & ds) :
  Event(ds) {
  ds.read(m_str);
}

StringEvent::StringEvent(unsigned run,
                         unsigned event,
                         const std::string & str
                         ) : Event(run, event), m_str(str) {
  m_typeid = Event::str2id("_STR");
}

StringEvent::StringEvent(unsigned run,
                         unsigned event /*= 0*/
                         ) : Event(run, event, NOTIMESTAMP, event ? Event::FLAG_EORE : Event::FLAG_BORE) {
  m_typeid = Event::str2id("_STR");
}

void StringEvent::Print(std::ostream & os) const {
  Print(os, 0);
}

void StringEvent::Print(std::ostream &os, size_t offset) const {
  Event::Print(os, offset);
  static const size_t max = 32;
  std::string s(m_str, 0, max);
  os << std::string(offset, ' ') << ", '" << s << (m_str.length() > max ? ("' (+" + to_string(m_str.length() - max) + ")") : "'");
}

std::string StringEvent::GetType() const {
  return "StringEvent";
}

void StringEvent::Serialize(Serializer & ser) const {
  Event::Serialize(ser);
  ser.write(m_str);
}

eudaq::StringEvent StringEvent::EORE(unsigned run, unsigned event) {
  return StringEvent(run, event);
}

eudaq::StringEvent StringEvent::BORE(unsigned run) {
  return StringEvent(run);
}

}
