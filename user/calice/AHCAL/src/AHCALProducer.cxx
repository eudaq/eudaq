#include "eudaq/Producer.hh"
#include "eudaq/Logger.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/Utils.hh"
#include <iostream>

#include "AHCALProducer.hh"

int main(int /*argc*/, const char ** argv) {
   // You can use the OptionParser to get command-line arguments
   // then they will automatically be described in the help (-h) option
   eudaq::OptionParser op("Calice Producer Producer", "1.0", "hogehoge");
   eudaq::Option<std::string> rctrl(op, "r", "runcontrol",
         "tcp://localhost:44000", "address",
         "The address of the RunControl.");
   eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level",
         "The minimum level for displaying log messages locally");
   eudaq::Option<std::string> name(op, "n", "", "Calice1", "string",
         "The name of this Producer");
   try {
      // This will look through the command-line arguments and set the options
      op.Parse(argv);
      // Set the Log level for displaying messages based on command-line
      EUDAQ_LOG_LEVEL(level.Value());
      // Create a producer
      std::cout << name.Value() << " " << rctrl.Value() << std::endl;
      eudaq::AHCALProducer producer(name.Value(), rctrl.Value());
      //producer.SetReader(new eudaq::SiReader(0));
      // And set it running...
      // producer.MainLoop();
      producer.Exec();
      // When the readout loop terminates, it is time to go
      std::cout << "Quitting" << std::endl;
   } catch (...) {
      // This does some basic error handling of common exceptions
      return op.HandleMainException();
   }
   return 0;
}
