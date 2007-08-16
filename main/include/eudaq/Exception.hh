#ifndef EUDAQ_INCLUDED_Exception
#define EUDAQ_INCLUDED_Exception

#include "eudaq/Utils.hh"
#include <exception>
#include <string>

#ifndef EUDAQ_FUNC
# define EUDAQ_FUNC ""
#endif

#define EUDAQ_THROW(msg) throw ::eudaq::LoggedException(msg, __FILE__, __LINE__, EUDAQ_FUNC)

namespace eudaq {

  class Exception : public std::exception {
  public:
    Exception(const std::string & msg, const std::string & file = "",
              unsigned line = 0, const std::string & func = "");
    const char * what() const throw() {
      if (m_text.length() == 0) make_text();
      return m_text.c_str();
    }
    ~Exception() throw() {
    }
  private:
    void make_text() const {
      m_text = m_msg + "\n"
        + "  From " + m_file + ":" + to_string(m_line);
      if (m_func.length() > 0) m_text += "\n  In " + m_func;
    }
    mutable std::string m_text;
    std::string m_msg, m_file, m_func;
    unsigned m_line;
  };

  class LoggedException : public Exception {
  public:
    LoggedException(const std::string & msg, const std::string & file,
                    unsigned line, const std::string & func);
  };

}

#endif // EUDAQ_INCLUDED_Exception
