#include "eudaq/LogMessage.hh"
#include "eudaq/Utils.hh"

#include <ostream>

namespace eudaq {

  LogMessage::LogMessage(const std::string & msg, Level level, const Time & time)
    : Status(level, msg), m_line(0), m_time(time), m_createtime(Time::Current())
  {}

  LogMessage::LogMessage(Deserializer & ds)
    : Status(ds), m_time(0, 0), m_createtime(Time::Current()) {
    ds.read(m_file);
    ds.read(m_func);
    ds.read(m_line);
    ds.read(m_time);
  }

  void LogMessage::Serialize(Serializer & ser) const {
    Status::Serialize(ser);
    ser.write(m_file);
    ser.write(m_func);
    ser.write(m_line);
    ser.write(m_time);
  }

  void LogMessage::Print(std::ostream & os) const {
    os << Level2String(m_level) << ": " << m_msg
       << " " << m_time.Formatted();
    if (m_sendertype != "") os << " " << GetSender();
    if ((m_level <= LVL_DEBUG || m_level >= LVL_ERROR) && m_file != "") {
      os << " [in " << m_file << ":" << m_line;
      if (m_func != "") os << ", " << m_func;
      os << "]";
    }
  }

  void LogMessage::Write(std::ostream & os) const {
    os << Level2String(m_level) << "\t"
       << m_msg << "\t"
       << m_time.Formatted() << "\t"
       << GetSender() << "\t"
       << m_file << ":" << m_line << "\t"
       << m_func << "\n";
  }

  LogMessage & LogMessage::SetLocation(const std::string & file, unsigned line, const std::string & func) {
    m_file = file;
    m_line = line;
    m_func = func;
    return *this;
  }

  LogMessage & LogMessage::SetSender(const std::string & name) {
    size_t i = name.find('.');
    if (i == std::string::npos) {
      m_sendertype = name;
      m_sendername = "";
    } else {
      m_sendertype = name.substr(0, i);
      m_sendername = name.substr(i+1);
    }
    return *this;
  }

}
