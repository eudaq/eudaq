#include "eudaq/Configuration.hh"
#include "eudaq/Producer.hh"
#include "eudaq/Logger.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Timer.hh"
#include "eudaq/Utils.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/ExampleHardware.hh"
#include <iostream>
#include <ostream>
#include <vector>

#include "eudaq/Status.hh"

// A name to identify the raw data format of the events generated
// Modify this to something appropriate for your producer.
static const std::string EVENT_TYPE = "Example";

// Declare a new class that inherits from eudaq::Producer
class ExampleProducer : public eudaq::Producer {
  public:

    // The constructor must call the eudaq::Producer constructor with the name
    // and the runcontrol connection string, and initialize any member variables.
    ExampleProducer(const std::string & name, const std::string & runcontrol)
      : eudaq::Producer(name, runcontrol),
      m_run(0), m_ev(0), stopping(false), done(false) {}

    // This gets called whenever the DAQ is initialised
    virtual void OnInitialise(const eudaq::Configuration & init) {
      try {
        std::cout << "Reading: " << init.Name() << std::endl;
        
        // Do any initialisation of the hardware here 
        // "start-up configuration", which is usally done only once in the beginning
        // Configuration file values are accessible as config.Get(name, default)
        m_exampleInitParam = init.Get("InitParameter", 0);
        
        // send information
        // Message as cout in the terminal of your producer
        std::cout << "Initialise with parameter = " << m_exampleInitParam << std::endl;
        // or to the LogCollector, depending which log level you want. These are the possibilities just as an example here:
        EUDAQ_INFO("Initialise with parameter = " + m_exampleInitParam);
        //EUDAQ_DEBUG("Debug Message to the LogCollector from ExampleProducer");
        //EUDAQ_EXTRA("Extra Message to the LogCollector from ExampleProducer");
        //EUDAQ_INFO("Info Message to the LogCollector from ExampleProducer");
        //EUDAQ_WARN("Warn Message to the LogCollector from ExampleProducer");
        //EUDAQ_ERROR("Error Message to the LogCollector from ExampleProducer");
        //EUDAQ_USER("User Message to the LogCollector from ExampleProducer");

        // EUDAQ_THROW throws an error, thus here goes to catch! With that you can simulate errors...
        //EUDAQ_THROW("User Message to the LogCollector from ExampleProducer");

        // send it to your hardware
        hardware.Setup(m_exampleInitParam);
        eudaq::mSleep(5000);

        // At the end, set the ConnectionState that will be displayed in the Run Control.
        // and set the state of the machine.
        SetConnectionState(eudaq::ConnectionState::STATE_UNCONF, "Initialised (" + init.Name() + ")");
      } 
      catch (...) {
        // Message as cout in the terminal of your producer
        std::cout << "Unknown exception" << std::endl;
        // Message to the LogCollector
        EUDAQ_ERROR("Error Message to the LogCollector from ExampleProducer");
        // Otherwise, the State is set to ERROR
        SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Initialisation Error");
      }
    }

    // This gets called whenever the DAQ is configured
    virtual void OnConfigure(const eudaq::Configuration & config) {
      try {
        std::cout << "Reading: " << config.Name() << std::endl;

        // Do any configuration of the hardware here
        // Configuration file values are accessible as config.Get(name, default)
        m_exampleConfParam = config.Get("ConfParameter", 0);
        std::cout << "Example Configuration Parameter = " << m_exampleConfParam << std::endl;
        hardware.Setup(m_exampleConfParam);

        // At the end, set the ConnectionState that will be displayed in the Run Control.
        // and set the state of the machine.
        SetConnectionState(eudaq::ConnectionState::STATE_CONF, "Configured (" + config.Name() + ")");
      } 
      catch (...) {
        // Otherwise, the State is set to ERROR
        printf("Unknown exception\n");
        SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Configuration Error");
      }
    }

    // This gets called whenever a new run is started
    // It receives the new run number as a parameter
    // And sets the event number to 0 (internally)
    virtual void OnStartRun(unsigned param) {
      try {

        m_run = param;
        m_ev = 0;
      
        std::cout << "Start Run: " << m_run << std::endl;

        // It must send a BORE (Begin-Of-Run Event) to the Data Collector
        eudaq::RawDataEvent bore(eudaq::RawDataEvent::BORE(EVENT_TYPE, m_run));
        // You can set tags on the BORE that will be saved in the data file
        // and can be used later to help decoding
        bore.SetTag("EXAMPLE", eudaq::to_string(m_exampleConfParam));
        // Starting your hardware
        hardware.PrepareForRun();
        // Send the event to the Data Collector
        SendEvent(bore);

        // At the end, set the ConnectionState that will be displayed in the Run Control.
        SetConnectionState(eudaq::ConnectionState::STATE_RUNNING, "Running");
      } 
      catch (...) {
        // Otherwise, the State is set to ERROR
        printf("Unknown exception\n");
        SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Starting Error");
      }
    }

    // This gets called whenever a run is stopped
    virtual void OnStopRun() {
      try {
        // Set a flag to signal to the polling loop that the run is over and it is in the stopping process
        stopping = true;

        // wait until all events have been read out from the hardware
        while (stopping) {
          eudaq::mSleep(20);
          //std::cout<<"Does hardware have pending? "<<hardware.EventsPending()<<"\n";
        }
        // Send an EORE after all the real events have been sent
        // You can also set tags on it (as with the BORE) if necessary
        SendEvent(eudaq::RawDataEvent::EORE("Test", m_run, ++m_ev));
        
        // At the end, set the ConnectionState that will be displayed in the Run Control.
        // Due to the definition of FSM, it should go to STATE_CONF. 
        if (m_connectionstate.GetState() != eudaq::ConnectionState::STATE_ERROR)
          SetConnectionState(eudaq::ConnectionState::STATE_CONF);
      } 
      catch (...) {
        // Otherwise, the State is set to ERROR
        printf("Unknown exception\n");
        SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Stopping Error");
      }
    }

    // This gets called when the Run Control is terminating,
    // we should also exit.
    virtual void OnTerminate() {
      std::cout << "Terminating..." << std::endl;
      done = true;
    }

    // This loop is running in the main
    // This is just an example, adapt it to your hardware
    void ReadoutLoop() {
      try {
        // Loop until Run Control tells us to terminate using the done flag
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

          // If the Producer is not in STATE_RUNNING, it will restart the loop
          if (GetConnectionState() != eudaq::ConnectionState::STATE_RUNNING) {
            // Now sleep for a bit, to prevent chewing up all the CPU
            eudaq::mSleep(20);
            // Then restart the loop
            continue;
          }

          // If we get here, there must be data to read out
          // Create a RawDataEvent to contain the event data to be sent
          eudaq::RawDataEvent ev(EVENT_TYPE, m_run, m_ev);

          for (unsigned plane = 0; plane < hardware.NumSensors(); ++plane) {
            // Read out a block of raw data from the hardware
            std::vector<unsigned char> buffer = hardware.ReadSensor(plane);
            // Each data block has an ID that is used for ordering the planes later
            // If there are multiple sensors, they should be numbered incrementally

            // Add the block of raw data to the event
            ev.AddBlock(plane, buffer);
          }
          hardware.CompletedEvent();
          // Send the event to the Data Collector      
          SendEvent(ev);
          // Now increment the event number
          m_ev++;
        } 
      }
      catch (...) {
        // Otherwise, the State is set to ERROR
        printf("Unknown exception\n");
        SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Error during running");
      }
    }

  private:
    // This is just a dummy class representing the hardware
    // It here basically that the example code will compile
    // but it also generates example raw data to help illustrate the decoder
    eudaq::ExampleHardware hardware;
    unsigned m_run, m_ev, m_exampleConfParam, m_exampleInitParam;
    bool stopping, done;
};

// The main function that will create a Producer instance and run it
int main(int /*argc*/, const char ** argv) {
  // You can use the OptionParser to get command-line arguments
  // then they will automatically be described in the help (-h) option
  eudaq::OptionParser op("EUDAQ Example Producer", "1.0",
      "Just an example, modify it to suit your own needs");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol",
      "tcp://localhost:44000", "address",
      "The address of the RunControl.");
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
