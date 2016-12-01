#include "eudaq/Producer.hh"
#include "eudaq/Logger.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/Utils.hh"
#include <iostream>

int main(int /*argc*/, const char **argv) {
  std::cout << "Start Producer \n" << std::endl;

  eudaq::OptionParser op("EUDAQ NI Producer", "0.1",
                         "The Producer task for the NI crate");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol",
                                   "tcp://localhost:44000", "address",
                                   "The address of the RunControl application");
  eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level",
				   "The minimum level for displaying log messages locally");
  eudaq::Option<std::string> name(op, "n", "name", "MimosaNI", "string",
                                  "The name of this Producer");
  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL(level.Value());
    auto app=eudaq::Factory<eudaq::Producer>::MakeShared<const std::string&,const std::string&>
      (eudaq::cstr2hash("NiProducer"), name.Value(), rctrl.Value());
    if(app)
      app->Exec();
    else
      std::cerr<<"Error: No NiProducer class registed in system\n";
    eudaq::mSleep(500);
  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}
