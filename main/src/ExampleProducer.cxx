#include "eudaq/Producer.hh"
#include "eudaq/Logger.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Utils.hh"
#include "eudaq/OptionParser.hh"
#include <iostream>
#include <ostream>
#include <vector>

// This is just a dummy class representing the hardware
// It only here so that the example code below will compile
class ExampleHardware {
public:
  void Setup(int) {
  }
  bool EventsPending() const {
    return false;
  }
  std::vector<unsigned char> ReadEvent() {
    return std::vector<unsigned char>(0);
  }
};
ExampleHardware hardware;

// A name to identify the raw data format of the events generated
// Modify this to something appropriate for your producer.
static const std::string EVENT_TYPE = "Example";

// Declare a new class that inherits from eudaq::Producer
class ExampleProducer : public eudaq::Producer {
public:

  // The constructor must call the eudaq::Producer constructor with the name
  // and the runcontrol connection string, and initialize any member variables.
  ExampleProducer(const std::string & name, const std::string & runcontrol)
    : eudaq::Producer(name, runcontrol), m_run(0), m_ev(0), stopping(false), done(false) {}

  // This gets called whenever the DAQ is configured
  virtual void OnConfigure(const eudaq::Configuration & config) {
    std::cout << "Configuring: " << config.Name() << std::endl;

    // Do any configuration of the hardware here
    // You can get values from the configuration file with config.Get(name, default)
    m_exampleparam = config.Get("Parameter", 0);
    std::cout << "Example Parameter = " << m_exampleparam << std::endl;
    hardware.Setup(m_exampleparam);

    // At the end, set the status that will be displayed in the Run Control.
    SetStatus(eudaq::Status::LVL_OK, "Configured (" + config.Name() + ")");
  }

  // This gets called whenever a new run is started
  // It receives the new run number as a parameter
  virtual void OnStartRun(unsigned param) {
    m_run = param;
    m_ev = 0;
    std::cout << "Start Run: " << m_run << std::endl;

    // It must send a BORE to the Data Collector
    eudaq::RawDataEvent bore(eudaq::RawDataEvent::BORE(EVENT_TYPE, m_run));
    // You can set tags on the BORE that will be saved in the data file
    // and can be used later to help decoding
    bore.SetTag("EXAMPLE", eudaq::to_string(m_exampleparam));
    // Send the event to the Data Collector
    SendEvent(bore);

    // At the end, set the status that will be displayed in the Run Control.
    SetStatus(eudaq::Status::LVL_OK, "Running");
  }

  // This gets called whenever a run is stopped
  virtual void OnStopRun() {
    std::cout << "Stopping Run" << std::endl;

    // Set a flag to signal to the polling loop that the run is over
    stopping = true;

    // wait until all events have been read out from the hardware
    while (stopping) {
      eudaq::mSleep(20);
    }

    // Send an EORE after all the real events have been sent
    // You can also set tags on it (as with the BORE) if necessary
    SendEvent(eudaq::RawDataEvent::EORE("Test", m_run, ++m_ev));
  }

  // This gets called when the Run Control is terminating,
  // we should also exit.
  virtual void OnTerminate() {
    std::cout << "Terminating..." << std::endl;
    done = true;
  }

  // This is just an example, adapt it to your hardware
  void ReadoutLoop() {
    // Loop until Run Control tells us to terminate
    while (!done) {
      if (!hardware.EventsPending()) {
        // No events are pending, so check if the run is stopping
        if (stopping) {
          // if so, signal that there are no events left
          stopping = false;
        }
        // Now sleep for a bit, to prevent chewing up all the CPU
        eudaq::mSleep(20);
        // Then restart the loop
        continue;
      }

      // If we get here, there must be data to read out
      // Create a RawDataEvent to contain the event data to be sent
      eudaq::RawDataEvent ev(EVENT_TYPE, m_run, m_ev);

      // Read out a block of raw data from the hardware
      std::vector<unsigned char> buffer = hardware.ReadEvent();
      // Each block of data has an ID that will be used for ordering the planes later
      // If you have more than one sensor, you should number them incrementally
      int id = 0;
      // Add the block of raw data to the event
      ev.AddBlock(id, buffer);
      // Send the event to the Data Collector      
      SendEvent(ev);
      // Now increment the event number
      m_ev++;
    }
  }

private:
  unsigned m_run, m_ev, m_exampleparam;
  bool stopping, done;
};

// The main function that will create a Producer instance and run it
int main(int /*argc*/, const char ** argv) {
  // You can use the OptionParser to get command-line arguments
  // then they will automatically be described in the help (-h) option
  eudaq::OptionParser op("EUDAQ Example Producer", "1.0", "Just an example, modify it to suit your own needs");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol", "tcp://localhost:44000", "address",
                                   "The address of the RunControl application");
  eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level",
                                   "The minimum level for displaying log messages locally");
  eudaq::Option<std::string> name (op, "n", "name", "Example", "string",
                                   "The name of this Producer");
  try {
    // This will look through the command-line arguments and set the options
    op.Parse(argv);
    // Set the Log level for displaying messages based on command-line
    EUDAQ_LOG_LEVEL(level.Value());
    // Create a producer
    ExampleProducer producer(name.Value(), rctrl.Value());
    // And set it running...
    producer.ReadoutLoop();
    // When the readout loop terminates, it is time to go
    std::cout << "Quitting" << std::endl;
  } catch (...) {
    // This does some basic error handling of common exceptions
    return op.HandleMainException();
  }
  return 0;
}
