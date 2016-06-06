#include "eudaq/CommandReceiver.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Timer.hh"
#include "eudaq/Utils.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/Configuration.hh"

#include "RPiController.hh"

#include <iostream>
#include <ostream>
#include <vector>
#include <sched.h>

using namespace std;


RPiController::RPiController(const std::string &name,
			     const std::string &runcontrol)
  : eudaq::CommandReceiver("Producer", name, runcontrol), m_terminated(false), m_name("") { }

void RPiController::OnConfigure(const eudaq::Configuration &config) {

  std::cout << "Configuring: " << config.Name() << std::endl;

  try {
    SetStatus(eudaq::Status::LVL_OK, "Configured (" + config.Name() + ")");
  } catch (...) {
    EUDAQ_ERROR(string("Unknown exception."));
    SetStatus(eudaq::Status::LVL_ERROR, "Unknown exception.");
  }
}

void RPiController::OnStartRun(unsigned runnumber) {

  try {
    // Wait 4 sec before returning OK.
    //eudaq::mSleep(4000);

    SetStatus(eudaq::Status::LVL_OK, "Running");
  } catch (...) {
    EUDAQ_ERROR(string("Unknown exception."));
    SetStatus(eudaq::Status::LVL_ERROR, "Unknown exception.");
  }
}

// This gets called whenever a run is stopped
void RPiController::OnStopRun() {

  try {
    SetStatus(eudaq::Status::LVL_OK, "Stopped");
  } catch (const std::exception &e) {
    printf("While Stopping: Caught exception: %s\n", e.what());
    SetStatus(eudaq::Status::LVL_ERROR, "Stop Error");
  } catch (...) {
    printf("While Stopping: Unknown exception\n");
    SetStatus(eudaq::Status::LVL_ERROR, "Stop Error");
  }
}

void RPiController::OnTerminate() {

  std::cout << "RPiController terminating..." << std::endl;

  m_terminated = true;
  std::cout << "RPiController " << m_name << " terminated."
            << std::endl;
}

void RPiController::ReadoutLoop() {

  // Loop until Run Control tells us to terminate
  while (!m_terminated) {
    // Move this thread to the end of the scheduler queue:
    sched_yield();
    continue;
  }

}

// The main function that will create a Producer instance and run it
int main(int /*argc*/, const char **argv) {
  // You can use the OptionParser to get command-line arguments
  // then they will automatically be described in the help (-h) option
  eudaq::OptionParser op("Raspberry Pi Controller", "0.0", "Run options");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol",
                                   "tcp://localhost:44000", "address",
                                   "The address of the RunControl.");
  eudaq::Option<std::string> level(
      op, "l", "log-level", "NONE", "level",
      "The minimum level for displaying log messages locally");
  eudaq::Option<std::string> name(op, "n", "name", "RpiControl", "string",
                                  "The name of this Producer");
  try {
    // This will look through the command-line arguments and set the options
    op.Parse(argv);
    // Set the Log level for displaying messages based on command-line
    EUDAQ_LOG_LEVEL(level.Value());

    // Create a producer
    RPiController producer(name.Value(), rctrl.Value());
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
