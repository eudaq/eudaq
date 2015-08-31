#include "eudaq/Serializer.hh"
#include "eudaq/Status.hh"
#include "eudaq/Exception.hh"
#include "eudaq/Utils.hh"

namespace eudaq {

  Status::Status(Deserializer &ds) {
    ds.read(m_level);
    ds.read(m_msg);
    ds.read(m_tags);
    ds.read(m_state);
  } 


  void Status::Serialize(Serializer &ser) const {
    ser.write(m_level);
    ser.write(m_msg);
    ser.write(m_tags);
    ser.write(m_state);
  }

  std::string Status::Level2String(int level) {
    static const char *const strings[] = {"DEBUG", "OK",   "THROW", "EXTRA",
                                          "INFO",  "WARN", "ERROR", "USER",
                                          "BUSY",  "NONE"};
    if (level < LVL_DEBUG || level > LVL_NONE)
      return "";
    return strings[level];
  }

  std::string Status::State2String(int state) {
    static const char *const strings[] = {"Unconfigured","Configured","Running","Error"};
    return strings[state];
  }

  int Status::String2Level(const std::string &str) {
    int lvl = 0;
    std::string tmpstr1, tmpstr2 = ucase(str);
    while ((tmpstr1 = Level2String(lvl)) != "") {
      if (tmpstr1 == tmpstr2)
        return lvl;
      lvl++;
    }
    EUDAQ_THROW("Unrecognised level: " + str);
  }

  int Status::String2State(const std::string &str) {
    int lvl = 0;
    std::string tmpstr1, tmpstr2 = ucase(str);
    while ((tmpstr1 = State2String(lvl)) != "") {
      if (tmpstr1 == tmpstr2)
        return lvl;
      lvl++;
    }
    EUDAQ_THROW("Unrecognised level: " + str);
  }

  Status &Status::SetTag(const std::string &name, const std::string &val) {
    m_tags[name] = val;
    return *this;
  }

  std::string Status::GetTag(const std::string &name,
                             const std::string &def) const {
    std::map<std::string, std::string>::const_iterator i = m_tags.find(name);
    if (i == m_tags.end())
      return def;
    return i->second;
  }

  void Status::print(std::ostream &os) const {
    os << State2String(m_state);
    if (m_msg.size() > 0) {
      os << ": " << m_msg;
    }
  }
}
