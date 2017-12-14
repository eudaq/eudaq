#ifndef EUDAQ_INCLUDED_Logger
#define EUDAQ_INCLUDED_Logger

#include "eudaq/LogSender.hh"
#include "eudaq/LogMessage.hh"
#include "eudaq/Platform.hh"

namespace eudaq {
  DLLEXPORT LogSender &GetLogger();
}

#define EUDAQ_LOG_LEVEL(level) ::eudaq::GetLogger().SetLevel(level)
#define EUDAQ_ERR_LEVEL(level) ::eudaq::GetLogger().SetErrLevel(level)
#define EUDAQ_IS_LOGGED(level) ::eudaq::GetLogger().IsLogged(level)
#define EUDAQ_LOG_CONNECT(type, name, server)                                  \
  ::eudaq::GetLogger().Connect(type, name, server)

#define EUDAQ_LOG(level, msg)                                                  \
  ::eudaq::GetLogger().SendLogMessage(                                         \
      ::eudaq::LogMessage(msg, ::eudaq::LogMessage::LVL_##level)               \
          .SetLocation(__FILE__, __LINE__, EUDAQ_FUNC))
  
#define EUDAQ_DEBUG(msg) EUDAQ_LOG(DEBUG, msg)
#define EUDAQ_EXTRA(msg) EUDAQ_LOG(EXTRA, msg)
#define EUDAQ_INFO(msg) EUDAQ_LOG(INFO, msg)
#define EUDAQ_WARN(msg) EUDAQ_LOG(WARN, msg)
#define EUDAQ_ERROR(msg) EUDAQ_LOG(ERROR, msg)
#define EUDAQ_USER(msg) EUDAQ_LOG(USER, msg)

#define EUDAQ_LOG_STREAMOUT(level, msg, outStream, error_stream)               \
  ::eudaq::GetLogger().SendLogMessage(                                         \
      ::eudaq::LogMessage(msg, ::eudaq::LogMessage::LVL_##level)               \
          .SetLocation(__FILE__, __LINE__, EUDAQ_FUNC),                        \
      outStream, error_stream)
#define EUDAQ_DEBUG_STREAMOUT(msg, outStream, error_stream)                    \
  EUDAQ_LOG_STREAMOUT(DEBUG, msg, outStream, error_stream)
#define EUDAQ_EXTRA_STREAMOUT(msg, outStream, error_stream)                    \
  EUDAQ_LOG_STREAMOUT(EXTRA, msg, outStream, error_stream)
#define EUDAQ_INFO_STREAMOUT(msg, outStream, error_stream)                     \
  EUDAQ_LOG_STREAMOUT(INFO, msg, outStream, error_stream)
#define EUDAQ_WARN_STREAMOUT(msg, outStream, error_stream)                     \
  EUDAQ_LOG_STREAMOUT(WARN, msg, outStream, error_stream)
#define EUDAQ_ERROR_STREAMOUT(msg, outStream, error_stream)                    \
  EUDAQ_LOG_STREAMOUT(ERROR, msg, outStream, error_stream)
#define EUDAQ_USER_STREAMOUT(msg, outStream, error_stream)                     \
  EUDAQ_LOG_STREAMOUT(USER, msg, outStream, error_stream)

// #define EUDAQ_DEBUG(msg, outStream)  EUDAQ_DEBUG(DEBUG, msg, outStream,
// outStream)
// #define EUDAQ_EXTRA(msg, outStream)  EUDAQ_EXTRA(EXTRA, msg, outStream,
// outStream)
// #define EUDAQ_INFO( msg, outStream)  EUDAQ_INFO( INFO,  msg, outStream,
// outStream)
// #define EUDAQ_WARN( msg, outStream)  EUDAQ_WARN( WARN,  msg, outStream,
// outStream)
// #define EUDAQ_ERROR(msg, outStream)  EUDAQ_ERROR(ERROR, msg, outStream,
// outStream)
// #define EUDAQ_USER( msg, outStream)  EUDAQ_USER( USER,  msg, outStream,
// outStream)

#endif // EUDAQ_INCLUDED_Logger
