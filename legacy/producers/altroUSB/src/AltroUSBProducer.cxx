#include "AltroUSBProducer.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/Logger.hh"

int main(int /*argc*/, char ** argv) {
  eudaq::OptionParser op("AltroUSB Producer", "0.0", "The AltroUSB Producer");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol", "tcp://localhost:44000", "address",
                                   "The address of the RunControl application");
  eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level",
                                   "The minimum level for displaying log messages locally");
  eudaq::Option<std::string> name (op, "n", "name", "AltroUSB", "string",
                                   "The name of this Producer");
  //eudaq::Option<int>         port (op, "p", "port", 20248, "num",
  //                                 "The TCP port to listen to for data");
  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL(level.Value());
    AltroUSBProducer altrousbproducer(name.Value(), rctrl.Value());

    altrousbproducer.Exec();
    
    return 0;

  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}
