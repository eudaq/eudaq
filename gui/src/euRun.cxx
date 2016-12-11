#include "eudaq/OptionParser.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Utils.hh"
#include "eudaq/RunControl.hh"
#include <iostream>

#include "config.h" // for version symbols


int main(int argc, char **argv) {
  eudaq::OptionParser op("EUDAQ Run Control", PACKAGE_VERSION,
                         "A Qt version of the Run Control");
  eudaq::Option<std::string> addr(op, "a", "listen-address", "tcp://44000", "address",
				  "The address on which to listen for connections");
  eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level",
				   "The minimum level for displaying log messages locally");
  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL(level.Value());
    auto app=eudaq::Factory<eudaq::RunControl>::MakeShared<const std::string&>(eudaq::cstr2hash("GuiRunControl"), addr.Value());
    app->Exec();
  } catch (...) {
    std::cout << "euRun exception handler" << std::endl;
    std::ostringstream err;
    int result = op.HandleMainException(err);
    std::cerr<<"Exception"<< err.str()<<"\n";
    return result;
  }
  return 0;
}
