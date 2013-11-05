#include "depfet/DEPFETProducerTCP.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/Logger.hh"

int main(int /*argc*/, char ** argv) {
  eudaq::OptionParser op("DEPFET Producer", "1.0", "The DEPFET Producer");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol", "tcp://localhost:44000", "address",
                                   "The address of the RunControl application");
  eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level",
                                   "The minimum level for displaying log messages locally");
  eudaq::Option<std::string> name (op, "n", "name", "DEPFET", "string",
                                   "The name of this Producer");
  //eudaq::Option<int>         port (op, "p", "port", 20248, "num",
  //                                 "The TCP port to listen to for data");
  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL(level.Value());
    DEPFETProducerTCP producer(name.Value(), rctrl.Value());

    do {
      producer.Process();
    } while (!producer.done);
    std::cout << "Quitting" << std::endl;
  } catch (...) {
    return op.HandleMainException();
  }  return 0;
}
