#include "eudaq/Logger.hh"

namespace eudaq {
  LogSender & GetLogger() {
    static LogSender logger;
    return logger;
  }
}
