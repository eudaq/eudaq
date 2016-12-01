#include "eudaq/Utils.hh"
#include "eudaq/DataCollector.hh"
#include "eudaq/Logger.hh"
#include "eudaq/OptionParser.hh"
#include <iostream>

int main(int /*argc*/, const char ** argv) {
  eudaq::OptionParser op("EUDAQ Data Collector", "1.0", "A command-line version of the Data Collector");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol", "tcp://localhost:44000", "address",
      "The address of the RunControl application");
  eudaq::Option<std::string> addr (op, "a", "listen-address", "tcp://44001", "address",
      "The address on which to listen for Data connections");
  eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level",
      "The minimum level for displaying log messages locally");
  eudaq::Option<std::string> name (op, "n", "name", "", "string",
      "The name of this DataCollector");
  eudaq::Option<std::string> runnumberfile (op, "f", "runnumberfile", "../data/runnumber.dat", "string",
      "The path and name of the file containing the run number of the last run.");
  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL(level.Value());
    auto app=eudaq::Factory<eudaq::DataCollector>::MakeShared<const std::string&,const std::string&,
							      const std::string&,const std::string&>
      (eudaq::cstr2hash("TestDataCollector"), name.Value(), rctrl.Value(), addr.Value(), runnumberfile.Value());
    app->Exec();
    std::cout << "Quitting" << std::endl;
  } catch (...) {
    std::cout << "DataCollector exception handler" << std::endl;
    return op.HandleMainException();
  }
  return 0;
}
