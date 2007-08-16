#include "eudaq/Exception.hh"

#include "eudaq/Logger.hh"

namespace eudaq {

  Exception::Exception(const std::string & msg, const std::string & file,
                       unsigned line, const std::string & func)
    : m_msg(msg), m_file(file), m_func(func), m_line(line)
  {}

  LoggedException::LoggedException(const std::string & msg, const std::string & file,
                                   unsigned line, const std::string & func)
    : Exception(msg, file, line, func)
  {
    eudaq::GetLogger().SendLogMessage(eudaq::LogMessage(msg, eudaq::LogMessage::LVL_THROW)
                                      .SetLocation(file, line, func));
  }

}
