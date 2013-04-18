#include "eudaq/CommandReceiver.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Utils.hh"

#include <iostream>
#include <ostream>

class RunListener : public eudaq::CommandReceiver {
  public:
    RunListener(const std::string & name, const std::string & runcontrol) :
      CommandReceiver("Listener", name, runcontrol),
      done(false) {
      }
    void Run() {
      while (!done) {
        eudaq::mSleep(100);
      }
    }
  private:
    virtual void OnConfigure(const eudaq::Configuration & config) {
      // This gets called whenever the DAQ is configured
      // You can get values from the configuration file with config.Get(name, default);
      unsigned someparam = config.Get("Parameter", 0);
      std::cout << "Configuring: " << config.Name() << std::endl
        << "Some Parameter = " << someparam << std::endl;
      SetStatus(eudaq::Status::LVL_OK, "Configured (" + config.Name() + ")");
    }
    virtual void OnStartRun(unsigned param) {
      // This gets called whenever a new run is started
      // It receives the new run number as a parameter
      std::cout << "Start Run: " << param << std::endl;
      SetStatus(eudaq::Status::LVL_OK, "Running");
    }
    virtual void OnStopRun() {
      // This gets called whenever a run is stopped
      std::cout << "Stopping Run" << std::endl;
      SetStatus(eudaq::Status::LVL_OK, "Stopped");
    }
    virtual void OnTerminate() {
      // This gets called when we are asked by Run Control to terminate
      done = true; // So that the Run Loop terminates
    }
    bool done;
};

int main(int /*argc*/, const char ** argv) {
  eudaq::OptionParser op("EUDAQ RunListener", "1.0", "Listens to commands from Run Control");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol", "tcp://localhost:44000", "address",
      "The address of the RunControl application");
  eudaq::Option<std::string> name (op, "n", "name", "Test", "string",
      "The name of this Listener");
  eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level",
      "The minimum level for displaying log messages locally");
  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL(level.Value());
    RunListener listener(name.Value(), rctrl.Value());
    listener.Run();
  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}
