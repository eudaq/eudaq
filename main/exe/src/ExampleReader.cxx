#include "eudaq/FileReader.hh"
#include "eudaq/PluginManager.hh"
#include "eudaq/OptionParser.hh"
#include <iostream>

static const std::string EVENT_TYPE = "Example";

int main(int /*argc*/, const char ** argv) {
  // You can use the OptionParser to get command-line arguments
  // then they will automatically be described in the help (-h) option
  eudaq::OptionParser op("EUDAQ Example File Reader", "1.0",
      "Just an example, modify it to suit your own needs",
      1);
  eudaq::OptionFlag doraw(op, "r", "raw", "Display raw data from events");
  eudaq::OptionFlag docon(op, "c", "converted", "Display converted events");
  try {
    // This will look through the command-line arguments and set the options
    op.Parse(argv);

    // Loop over all filenames
    for (size_t i = 0; i < op.NumArgs(); ++i) {

      // Create a reader for this file
      eudaq::FileReader reader(op.GetArg(i));

      // Display the actual filename (argument could have been a run number)
      std::cout << "Opened file: " << reader.Filename() << std::endl;

      // The BORE is now accessible in reader.GetDetectorEvent()
      if (docon.IsSet()) {
        // The PluginManager should be initialized with the BORE
        eudaq::PluginManager::Initialize(reader.GetDetectorEvent());
      }

      // Now loop over all events in the file
      while (reader.NextEvent()) {
        if (reader.GetDetectorEvent().IsEORE()) {
          std::cout << "End of run detected" << std::endl;
          // Don't try to process if it is an EORE
          break;
        }

        if (doraw.IsSet()) {
          // Display summary of raw event
          //std::cout << reader.GetDetectorEvent() << std::endl;

          try {
            // Look for a specific RawDataEvent, will throw an exception if not found
            const eudaq::RawDataEvent & rev =
              reader.GetDetectorEvent().GetRawSubEvent(EVENT_TYPE);
            // Display summary of the Example RawDataEvent
            std::cout << rev << std::endl;
          } catch (const eudaq::Exception & e) {
            std::cout << "No " << EVENT_TYPE << " subevent in event "
              << reader.GetDetectorEvent().GetEventNumber()
              << std::endl;
          }
        }

        if (docon.IsSet()) {
          // Convert the RawDataEvent into a StandardEvent
          eudaq::StandardEvent sev =
            eudaq::PluginManager::ConvertToStandard(reader.GetDetectorEvent());

          // Display summary of converted event
          std::cout << sev << std::endl;
        }
      }
    }

  } catch (...) {
    // This does some basic error handling of common exceptions
    return op.HandleMainException();
  }
  return 0;
}
