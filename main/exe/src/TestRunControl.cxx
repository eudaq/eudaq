#include "eudaq/RunControl.hh"
#include "eudaq/Utils.hh"
#include "eudaq/OptionParser.hh"
#include <iostream>

int main(int /*argc*/, const char ** argv) {
  eudaq::OptionParser op("EUDAQ Run Control", "1.0", "A command-line version of the Run Control");
  eudaq::Option<std::string> addr (op, "a", "listen-address", "tcp://44000", "address",
      "The address on which to listen for connections");
  eudaq::Option<std::string> level(op, "l", "log-level",      "NONE", "level",
      "The minimum level for displaying log messages locally");

  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL(level.Value());
    auto app=eudaq::Factory<eudaq::RunControl>::MakeShared<const std::string&>(eudaq::cstr2hash("TestRunControl"), addr.Value());
    app->Exec();
  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}
