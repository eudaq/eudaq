#ifndef EUDAQ_INCLUDED_Exception
#define EUDAQ_INCLUDED_Exception

#include "eudaq/Utils.hh"
#include <exception>
#include <string>
#include "Platform.hh"

#ifndef EUDAQ_FUNC
#define EUDAQ_FUNC ""
#endif

#define EUDAQ_THROWX(exc, msg)                                                 \
  throw exc(msg,__FILE__, __LINE__, EUDAQ_FUNC)
#define EUDAQ_THROW(msg) EUDAQ_THROWX(::eudaq::LoggedException,msg)
#define EUDAQ_THROW_NOLOG(msg) EUDAQ_THROWX(::eudaq::Exception,msg)

#define EUDAQ_EXCEPTIONX(name, base)                                           \
  class name : public base {                                                   \
  public:					                               \
    name(const std::string &msg, const std::string &file = "",unsigned line = 0,const std::string &func = "") : base(msg, file, line, func) {} \
  }

#define EUDAQ_EXCEPTION(name) EUDAQ_EXCEPTIONX(name, ::eudaq::Exception)

#if EUDAQ_PLATFORM_IS(WIN32)
#pragma warning(push)
#pragma warning(disable: 4275)
#endif

namespace eudaq {

  class DLLEXPORT Exception : public std::exception {
  public:
    Exception(const std::string &msg, const std::string &file = "",unsigned line = 0,const std::string &func = "");
    const char* what() const noexcept override;
    virtual ~Exception() throw() {}

  protected:
    std::string m_text;
  };

  class DLLEXPORT LoggedException : public Exception {
  public:
    LoggedException(const std::string &msg, const std::string &file = "",unsigned line = 0,const std::string &func = "");    
    void Log() const;
    virtual ~LoggedException() throw();

  private:
    mutable bool m_logged;
  };

  namespace {
    void do_log(const Exception &) {}
    void do_log(const LoggedException &e) { e.Log(); }
  }

  // Some useful predefined exceptions
  EUDAQ_EXCEPTION(FileNotFoundException);
  EUDAQ_EXCEPTION(FileExistsException);
  EUDAQ_EXCEPTION(FileNotWritableException);
  EUDAQ_EXCEPTION(FileReadException);
  EUDAQ_EXCEPTION(FileWriteException);
  EUDAQ_EXCEPTION(FileFormatException);
  EUDAQ_EXCEPTION(SyncException);
  EUDAQ_EXCEPTION(CommunicationException);
  EUDAQ_EXCEPTIONX(BusError, CommunicationException);
}

#if EUDAQ_PLATFORM_IS(WIN32)
#pragma warning(pop)
#endif

#endif // EUDAQ_INCLUDED_Exception
