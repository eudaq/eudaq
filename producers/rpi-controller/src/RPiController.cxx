#include "eudaq/Controller.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Timer.hh"
#include "eudaq/Utils.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/Configuration.hh"

#include "RPiController.hh"
#include <wiringPi.h>

#include <iostream>
#include <ostream>
#include <vector>
#include <sched.h>

using namespace std;


RPiController::RPiController(const std::string &name,
			     const std::string &runcontrol)
  : eudaq::Controller(name, runcontrol), m_terminated(false), m_name(name), m_pinnr(0), m_waiting_time(4000) { }

void RPiController::OnConfigure(const eudaq::Configuration &config) {

  std::cout << "Configuring: " << config.Name() << std::endl;

  // Read and store configured pin from eudaq config:
  m_pinnr = config.Get("pinnr", 0);
  std::cout << m_name << ": Configured pin " << std::to_string(m_pinnr) << " to be set to high." << std::endl;
  EUDAQ_INFO(string("Configured pin " + std::to_string(m_pinnr) +
                    " of the Raspberry Pi controller to be set to high."));

  // Store waiting time in ms before the pin is set to high in OnRunStart():
  m_waiting_time = config.Get("waiting_time", 4000);
  std::cout << m_name << ": Waiting " << std::to_string(m_waiting_time) << "ms for pin action." << std::endl;
  EUDAQ_INFO(string("Waiting " + std::to_string(m_waiting_time) +
                    "ms before enabling output pin at run start."));

  if(wiringPiSetupGpio() == -1) {
    std::cout << "WiringPi could not be set up" << std::endl;
    throw eudaq::LoggedException("WiringPi could not be set up");
  }
  // Set pin mode to output:
  pinMode(m_pinnr, OUTPUT);

  // Initialize as low:
  digitalWrite(m_pinnr, 0);

  try {
    SetStatus(eudaq::Status::LVL_OK, "Configured (" + config.Name() + ")");
  } catch (...) {
    EUDAQ_ERROR(string("Unknown exception."));
    SetStatus(eudaq::Status::LVL_ERROR, "Unknown exception.");
  }
}

void RPiController::OnStartRun(unsigned runnumber) {

  try {
    // Wait defined time before returning OK.
    std::cout << "Waiting configured time of "
	      << std::to_string(m_waiting_time) << "ms" << std::endl;
    eudaq::mSleep(m_waiting_time);
    // Set configured pin to high:
    std::cout << "Calling digitalWrite() to set pin high" << std::endl;
    digitalWrite(m_pinnr, 1);
    EUDAQ_INFO(string("GPIO pin " + std::to_string(m_pinnr) + " now high."));


    SetStatus(eudaq::Status::LVL_OK, "Running");
  } catch (...) {
    EUDAQ_ERROR(string("Unknown exception."));
    SetStatus(eudaq::Status::LVL_ERROR, "Unknown exception.");
  }
}

// This gets called whenever a run is stopped
void RPiController::OnStopRun() {

  try {
    // Set configured pin to low:
    std::cout << "Calling digitalWrite() to set pin low" << std::endl;
    digitalWrite(m_pinnr, 0);
    EUDAQ_INFO(string("GPIO pin " + std::to_string(m_pinnr) + " now low."));
    
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
  std::cout << "RPiController " << m_name << " terminated." << std::endl;
}

void RPiController::Loop() {

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
  eudaq::Option<std::string> name(op, "n", "name", "RPiController", "string",
                                  "The name of this Producer");
  try {
    // This will look through the command-line arguments and set the options
    op.Parse(argv);
    // Set the Log level for displaying messages based on command-line
    EUDAQ_LOG_LEVEL(level.Value());

    // Create the instance
    RPiController controller(name.Value(), rctrl.Value());
    // And set it running...
    controller.Loop();

    // When the keep-alive loop terminates, it is time to go
    std::cout << "Quitting" << std::endl;
  } catch (...) {
    // This does some basic error handling of common exceptions
    return op.HandleMainException();
  }
  return 0;
}
