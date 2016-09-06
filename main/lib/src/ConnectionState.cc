#include "eudaq/Serializer.hh"
#include "eudaq/ConnectionState.hh"
#include "eudaq/Exception.hh"
#include "eudaq/Utils.hh"

namespace eudaq {

  ConnectionState::ConnectionState(Deserializer &ds) {
    ds.read(m_msg);
    ds.read(m_tags);
    ds.read(m_state);
  } 


  void ConnectionState::Serialize(Serializer &ser) const {
    ser.write(m_msg);
    ser.write(m_tags);
    ser.write(m_state);
  }

  std::string ConnectionState::State2String(int state) {
    static const char *const strings[] = {"Uninitialised","Unconfigured","Configured","Running","Error"};
    return strings[state];
  }

  int ConnectionState::String2State(const std::string &str) {
    int st = 0;
    std::string tmpstr1, tmpstr2 = ucase(str);
    while ((tmpstr1 = State2String(st)) != "") {
      if (tmpstr1 == tmpstr2)
        return st;
      st++;
    }
    EUDAQ_THROW("Unrecognised state: " + str);
  }

  ConnectionState &ConnectionState::SetTag(const std::string &name, const std::string &val) {
    m_tags[name] = val;
    return *this;
  }

  std::string ConnectionState::GetTag(const std::string &name,
                             const std::string &def) const {
    std::map<std::string, std::string>::const_iterator i = m_tags.find(name);
    if (i == m_tags.end())
      return def;
    return i->second;
  }

  void ConnectionState::print(std::ostream &os) const {
    os << State2String(m_state);
    if (m_msg.size() > 0) {
      os << ": " << m_msg;
    }
  }
}
