#include <ostream>
#include <iostream>
#include <time.h>

#include "Event.hh"
#include "PluginManager.hh"
#include "BufferSerializer.hh"

namespace eudaq {

  template class DLLEXPORT Factory<Event>;
  template DLLEXPORT
  std::map<uint32_t, typename Factory<Event>::UP_BASE (*)(Deserializer&)>&
  Factory<Event>::Instance<Deserializer&>();
  template DLLEXPORT
  std::map<uint32_t, typename Factory<Event>::UP_BASE (*)()>&
  Factory<Event>::Instance<>();
    
  namespace {
    static const char * const FLAGNAMES[] = {
      "BORE",
      "EORE",
      "HITS",
      "FAKE",
      "SIMU",
      "EUDAQ2",
      "Packet"
    };
  }

  Event::Event(Deserializer & ds) {
    ds.read(m_typeid);
    ds.read(m_version);
    ds.read(m_flags);
    ds.read(m_id_stream);
    ds.read(m_runnumber);
    ds.read(m_eventnumber);
    ds.read(m_ts_begin);
    ds.read(m_ts_end);
    ds.read(m_tags);
  }

  Event::Event():m_id_stream(0), m_flags(0), m_runnumber(0), m_eventnumber(0){
  }

  
  Event::Event(uint32_t id_stream, unsigned run, unsigned event, unsigned flags /*= 0*/)
    : m_id_stream(id_stream), m_flags(flags), m_runnumber(run), m_eventnumber(event){
  }

  void Event::Serialize(Serializer & ser) const {
    ser.write(m_typeid);
    ser.write(m_version);
    ser.write(m_flags);
    ser.write(m_id_stream);
    ser.write(m_runnumber);
    ser.write(m_eventnumber);
    ser.write(m_ts_begin);
    ser.write(m_ts_end);
    ser.write(m_tags);
  }
  
  
  void Event::Print(std::ostream & os) const {
    Print(os, 0);
  }

  void Event::Print(std::ostream & os, size_t offset) const
  {
    os << std::string(offset, ' ') << "<Event> \n";
    os << std::string(offset + 2, ' ') << "<Type>" << id2str(get_id()) << ":" << GetSubType() << "</Type> \n";
    os << std::string(offset + 2, ' ') << "<RunNumber>" << m_runnumber << "</RunNumber>\n";
    os << std::string(offset + 2, ' ') << "<EventNumber>" << m_eventnumber << "</EventNumber>\n";    
    os << std::string(offset + 2, ' ') << "<Time> \n";
    os << std::string(offset +4, ' ') << "0x" << to_hex(m_ts_begin, 16) << ",\n";
    os << std::string(offset +4, ' ') << "0x" << to_hex(m_ts_begin, 16) << ",\n";
    os << std::string(offset + 2, ' ') << "</Time>\n";
    if (m_flags) {
      unsigned f = m_flags;
      bool first = true;
      for (size_t i = 0; f > 0; ++i, f >>= 1) {
        if (f & 1) {
          if (first){
            os << std::string(offset + 2, ' ') << "<Flags> ";
              first = false;
          }
          else{
            os << ", ";
          }
          if (i < sizeof FLAGNAMES / sizeof *FLAGNAMES){
            os << std::string(FLAGNAMES[i]);
          }
          else{
            os << to_string(i);
          }
        }
      }
      if (first==false){
        os  << "</Flags> \n";
      }
    }
    if (m_tags.size() > 0) {
      os << std::string(offset + 2, ' ') << "<Tags> \n";
      for (map_t::const_iterator i = m_tags.begin(); i != m_tags.end(); ++i) {
        os << std::string(offset+4, ' ')  << i->first << "=" << i->second << "\n";
      }
      os << std::string(offset + 2, ' ') << "</Tags> \n";
    }
    os << std::string(offset, ' ') << "</Event> \n";
  }

  unsigned Event::str2id(const std::string & str) {
    uint32_t result = 0;
    for (size_t i = 0; i < 4; ++i) {
      if (i < str.length()) result |= str[i] << (8 * i);
    }
    return result;
  }

  std::string Event::id2str(unsigned id) {
    std::string result(4, '\0');
    for (int i = 0; i < 4; ++i) {
      result[i] = (char)(id & 0xff);
      id >>= 8;
    }
    for (int i = 3; i >= 0; --i) {
      if (result[i] == '\0') {
        result.erase(i);
        break;
      }
    }
    return result;
  }

  
  Event & Event::SetTag(const std::string & name, const std::string & val) {
    m_tags[name] = val;
    return *this;
  }

  std::string Event::GetTag(const std::string & name, const std::string & def) const {
    auto i = m_tags.find(name);
    if (i == m_tags.end()) return def;
    return i->second;
  }

  void Event::SetTimeStampToNow(size_t i)
  {
    m_ts_begin=static_cast<uint64_t>(clock());
    m_ts_end=m_ts_begin+1;
  }

  bool Event::HasTag(const std::string &name) const{
    if (m_tags.find(name) == m_tags.end())
      return false;
    else
      return true;
  }


  unsigned Event::GetRunNumber() const
  {
    return m_runnumber;
  }

  unsigned Event::GetEventNumber() const
  {
    return m_eventnumber;
  }

  Event::timeStamp_t Event::GetTimestamp() const
  {
    return m_ts_begin;
  }

  void Event::SetTimestampBegin(Event::timeStamp_t t){
    m_ts_begin = t;
  }
  
  void Event::SetTimestampEnd(Event::timeStamp_t t){
    m_ts_end = t;
  }
  
  Event::timeStamp_t Event::GetTimestampBegin() const{
    return m_ts_begin;
  }
  
  Event::timeStamp_t Event::GetTimestampEnd() const{
    return m_ts_end;
  }

  EventUP Event::Clone() const{ //TODO: clone directly
    BufferSerializer ser;
    Serialize(ser);
    uint32_t id;
    ser.PreRead(id);
    return Factory<Event>::Create<Deserializer&>(id, ser);
  }
  
}
