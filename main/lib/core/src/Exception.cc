#include "eudaq/Exception.hh"

#include "eudaq/Logger.hh"

namespace eudaq {

  Exception::Exception(const std::string &msg, const std::string &file,unsigned line,const std::string &func) {
    m_text = msg;
    if (file.length() > 0) {
      m_text += "\n  From " + file;
      if (line > 0) {
	m_text += ":" + to_string(line);
      }
    }
    if (func.length() > 0)
      m_text += "\n  In " + func;
  }

  const char* Exception::what() const noexcept {
    return m_text.c_str();
  }    
  
  LoggedException::LoggedException(const std::string &msg, const std::string &file,unsigned line,const std::string &func) 
    : Exception(msg,file,line,func), m_logged(false) {
    do_log(*this);
  }

  void LoggedException::Log() const {
    if (m_logged)
      return;
    // Only log the message once
    eudaq::GetLogger().SendLogMessage(
        eudaq::LogMessage(m_text, eudaq::LogMessage::LVL_THROW));
    m_logged = true;
  }

  LoggedException::~LoggedException() throw() {
    // Make sure the message has been logged before we die
    Log();
  }
}
