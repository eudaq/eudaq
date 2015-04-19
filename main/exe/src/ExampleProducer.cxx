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

// A name to identify the raw data format of the events generated
// Modify this to something appropriate for your producer.
static const std::string EVENT_TYPE = "Example";

// Declare a new class that inherits from eudaq::Producer
class ExampleProducer : public eudaq::Producer {
public:
  enum status_enum{
    unconfigured,
    configured,
    starting,
    started,
    stopping,
    doTerminat

  };
  // The constructor must call the eudaq::Producer constructor with the name
  // and the runcontrol connection string, and initialize any member variables.
  ExampleProducer(const std::string & name, const std::string & runcontrol)
    : eudaq::Producer(name, runcontrol),
    m_run(0), m_ev(0) {}

  // This gets called whenever the DAQ is configured
  virtual void OnConfigure(const eudaq::Configuration & config) {
    std::cout << "Configuring: " << config.Name() << std::endl;

    // Do any configuration of the hardware here
    // Configuration file values are accessible as config.Get(name, default)
    m_exampleparam = config.Get("Parameter", 0);
    m_send_bore_delay = config.Get("boreDelay", 0);
    m_maxEventNR = config.Get("numberOfEvents", 100);
    m_ID = config.Get("ID", 0);
    m_Skip = config.Get("skip", 0);
    std::cout << "Example Parameter = " << m_exampleparam << std::endl;
    hardware.Setup(m_exampleparam);
    m_TLU = config.Get("TLU", 1);
    // At the end, set the status that will be displayed in the Run Control.
    SetStatus(eudaq::Status::LVL_OK, "Configured (" + config.Name() + ")");
    m_stat = configured;
    std::cout << "...Configured" << std::endl;
  }

  // This gets called whenever a new run is started
  // It receives the new run number as a parameter
  virtual void OnStartRun(unsigned param) {
    m_run = param;
    m_ev = 0;
    eudaq::mSleep(m_send_bore_delay);
    std::cout << "Start Run: " << m_run << std::endl;

    // It must send a BORE to the Data Collector
    eudaq::RawDataEvent bore(eudaq::RawDataEvent::BORE(EVENT_TYPE, m_run));
    // You can set tags on the BORE that will be saved in the data file
    // and can be used later to help decoding
    bore.SetTag("EXAMPLE", eudaq::to_string(m_exampleparam));
    bore.SetTag("TLU", m_TLU);
    bore.SetTag("ID", m_ID);
    bore.SetTimeStampToNow();
    // Send the event to the Data Collector
    SendEvent(bore);
    hardware.Start();
    // At the end, set the status that will be displayed in the Run Control.
    m_stat = started;
    SetStatus(eudaq::Status::LVL_OK, "Running");
    
  }

  // This gets called whenever a run is stopped
  virtual void OnStopRun() {
    std::cout << "Stopping Run" << std::endl;
    hardware.Stop();
    m_stat = stopping;
    // wait until all events have been read out from the hardware
    while (m_stat == stopping) {
      eudaq::mSleep(20);
    }

    // Send an EORE after all the real events have been sent
    // You can also set tags on it (as with the BORE) if necessary
    auto BOREvent = eudaq::RawDataEvent::EORE(EVENT_TYPE, m_run, ++m_ev);
    BOREvent.SetTag("TLU", m_TLU);
    BOREvent.SetTag("ID", m_ID);
    SendEvent(BOREvent);
    std::cout << "Stopped" << std::endl;
  }

  // This gets called when the Run Control is terminating,
  // we should also exit.
  virtual void OnTerminate() {
    std::cout << "Terminating..." << std::endl;
    m_stat = doTerminat;
  }

  // This is just an example, adapt it to your hardware
  void ReadoutLoop() {
    // Loop until Run Control tells us to terminate
    while (m_stat != doTerminat) {
      if (!hardware.EventsPending()) {
        // No events are pending, so check if the run is stopping
        if (m_stat=stopping) {
          // if so, signal that there are no events left
          m_stat = configured;
        }
        // Now sleep for a bit, to prevent chewing up all the CPU
        eudaq::mSleep(20);
        // Then restart the loop
        continue;
      }
      if (m_stat!=started&&m_stat!=stopping)
      {
        // Now sleep for a bit, to prevent chewing up all the CPU
        eudaq::mSleep(20);
        // Then restart the loop
        continue;
      }
      ++m_ev;
      if (m_maxEventNR<m_ev)
      {
        eudaq::mSleep(1000);
        continue;
      }
      // If we get here, there must be data to read out
      // Create a RawDataEvent to contain the event data to be sent
      eudaq::RawDataEvent ev(EVENT_TYPE, m_run, m_ev);
      ev.SetTag("TLU", m_TLU);
      ev.SetTag("ID", m_ID);
      
      for (unsigned plane = 0; plane < hardware.NumSensors(); ++plane) {
        // Read out a block of raw data from the hardware
        std::vector<unsigned char> buffer = hardware.ReadSensor(plane);
        // Each data block has an ID that is used for ordering the planes later
        // If there are multiple sensors, they should be numbered incrementally

        // Add the block of raw data to the event
        ev.AddBlock(plane, buffer);
      }
      ev.SetTimeStampToNow();
      hardware.CompletedEvent();
      // Send the event to the Data Collector 
      if (m_ev%1000==0)
      {
        std::cout << "sending Event: " << m_ev << std::endl;
      }
      if (m_Skip!=0)
      {
        if (m_ev%m_Skip==0)
        {
          
          continue;
        }
      }
      SendEvent(ev);
      // Now increment the event number
     
    }
  }

private:
  // This is just a dummy class representing the hardware
  // It here basically that the example code will compile
  // but it also generates example raw data to help illustrate the decoder
  eudaq::ExampleHardware hardware;
  unsigned m_run, m_ev, m_exampleparam, m_send_bore_delay, m_ID, m_maxEventNR, m_Skip;
  bool m_TLU;
  status_enum m_stat = unconfigured;

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
  eudaq::Option<std::string> name(op, "n", "name", "Example", "string",
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
  }
  catch (...) {
    // This does some basic error handling of common exceptions
    return op.HandleMainException();
  }
  return 0;
}
