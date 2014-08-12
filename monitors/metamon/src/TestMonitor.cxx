#include "eudaq/Monitor.hh"
#include "eudaq/Utils.hh"
#include "eudaq/DetectorEvent.hh"
#include "eudaq/EUDRBEvent.hh"
#include "eudaq/Logger.hh"
#include "eudaq/OptionParser.hh"

class TestMonitor : public eudaq::Monitor {
  public:
    TestMonitor(const std::string & runcontrol, const std::string & datafile)
      : eudaq::Monitor("Test", runcontrol, 0, 0, 0, datafile), done(false)
    {
    }
    virtual void OnEvent(std::shared_ptr<eudaq::DetectorEvent> ev) {
      std::cout << *ev << std::endl;
      for (size_t i = 0; i < ev->NumEvents(); ++i) {
        if (eudaq::EUDRBEvent * rev = dynamic_cast<eudaq::EUDRBEvent *>(ev->GetEvent(i))) {
          rev->Debug();
        }
      }
    }
    virtual void OnBadEvent(std::shared_ptr<eudaq::Event> ev) {
      EUDAQ_ERROR("Bad event type found in data file");
      std::cout << "Bad Event: " << *ev << std::endl;
    }
    virtual void OnConfigure(const std::string & param) {
      std::cout << "Configure: " << param << std::endl;
      SetStatus(eudaq::Status::LVL_OK);
    }
    virtual void OnTerminate() {
      std::cout << "Terminating" << std::endl;
      done = true;
    }
    virtual void OnReset() {
      std::cout << "Reset" << std::endl;
      SetStatus(eudaq::Status::LVL_OK);
    }
    virtual void OnUnrecognised(const std::string & cmd, const std::string & param) {
      std::cout << "Unrecognised: (" << cmd.length() << ") " << cmd;
      if (param.length() > 0) std::cout << " (" << param << ")";
      std::cout << std::endl;
      SetStatus(eudaq::Status::LVL_ERROR, "Just testing");
    }
    bool done;
};

int main(int /*argc*/, const char ** argv) {
  eudaq::OptionParser op("EUDAQ Monitor", "1.0", "A comand-line version of the Monitor");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol", "tcp://localhost:44000", "address",
      "The address of the RunControl application");
  eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level",
      "The minimum level for displaying log messages locally");
  eudaq::Option<std::string> file (op, "f", "data-file", "", "filename",
      "A data file to load - setting this changes the default"
      " run control address to 'null://'");
  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL(level.Value());
    if (file.IsSet() && !rctrl.IsSet()) rctrl.SetValue("null://");
    TestMonitor monitor(rctrl.Value(), file.Value());
    do {
      eudaq::mSleep(10);
    } while (!monitor.done);
    std::cout << "Quitting" << std::endl;
  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}
