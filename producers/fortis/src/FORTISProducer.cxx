#include "fortis/FORTISProducer.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/Logger.hh"

int main(int /*argc*/, const char ** argv) {
  eudaq::OptionParser op("EUDAQ FORTISProducer", "1.0", "The Producer task for reading out FORTIS chip via OptoDAQ");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol", "tcp://localhost:44000", "address",
                                   "The address of the RunControl application");
  eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level",
                                   "The minimum level for displaying log messages locally");
  eudaq::Option<std::string> name (op, "n", "name", "FORTIS", "string",
                                   "The name of this Producer");
  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL(level.Value());
    FORTISProducer producer(name.Value(), rctrl.Value());

    do {
      producer.Process();
      //      usleep(100);
    } while (!producer.done);
    std::cout << "Quitting" << std::endl;
  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}
