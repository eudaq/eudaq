#include "eudaq/Utils.hh"
#include "eudaq/LogCollector.hh"
#include "eudaq/OptionParser.hh"

#include <iostream>


int main(int /*argc*/, const char ** argv) {
  eudaq::OptionParser op("EUDAQ Log Collector", "1.0", "A comand-line version of the Log Collector");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol", "tcp://localhost:44000", "address",
      "The address of the RunControl application");
  eudaq::Option<std::string> addr (op, "a", "listen-address", "tcp://44002", "address",
      "The address on which to listen for Log connections");
  eudaq::Option<std::string> directory (op, "d", "directory", "logs", "directory",
				   "The path in which the log files should be stored");
  eudaq::Option<std::string> level(op, "l", "log-level", "INFO", "level",
      "The minimum level for displaying log messages");
  eudaq::mSleep(2000);
  try {
    op.Parse(argv);
    std::cout << "Log Collector  Connected to \"" << rctrl.Value() << "\"" << std::endl;
    auto app=eudaq::Factory<eudaq::LogCollector>::MakeShared<const std::string&, const std::string&,
							     const std::string&, const int&>
      (eudaq::cstr2hash("TestLogCollector"), rctrl.Value(), addr.Value(), directory.Value(),
       eudaq::Status::String2Level(level.Value()));
    app->Exec();
    std::cout << "Quitting" << std::endl;
  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}
