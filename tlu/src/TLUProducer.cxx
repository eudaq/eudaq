#include "eudaq/Producer.hh"
#include "eudaq/TLUEvent.hh" // for the TLU event
#include "eudaq/Utils.hh"
#include "eudaq/Logger.hh"
#include "eudaq/OptionParser.hh"
#include "tlu/TLUController.hh"
#include <iostream>
#include <ostream>
#include <cctype>

typedef eudaq::TLUEvent TLUEvent;
using eudaq::to_string;
using eudaq::to_hex;
using namespace tlu;

class TLUProducer : public eudaq::Producer, public TLUController {
public:
  TLUProducer(const std::string & runcontrol)
    : eudaq::Producer("TLU", runcontrol),
      m_run(0),
      m_ev(0),
      trigger_interval(0),
      dut_mask(0),
      veto_mask(0),
      and_mask(255),
      or_mask(0),
      done(false),
      TLUStarted(false),
      TLUJustStopped(false),
      lasttime(0)
    {}
  void Event(unsigned tlu_evntno, const long long int & tlu_timestamp) {
    SendEvent(TLUEvent(m_run, tlu_evntno, tlu_timestamp)); // not the right event number!!
  }
  void MainLoop() {
    do {
      bool JustStopped = TLUJustStopped;
      if (TLUStarted || JustStopped) {
        Update(); // get new events
        usleep(100000);
        for (size_t i = 0; i < NumEntries(); ++i) {
          m_ev = GetEntry(i).Eventnum();
          unsigned long long t = GetEntry(i).Timestamp();
          long long d = t-lasttime;
          float freq= 1./(d*20./1000000000);
          std::cout << "  " << GetEntry(i)
                    << ", diff=" << d << (d <= 0 ? "  ***" : "")
                    << ", freq=" << freq
                    << std::endl;
          lasttime = t;
          Event(m_ev,t);
        }
      }
      if (JustStopped) {
        TLUJustStopped = false;
        SendEvent(TLUEvent::EORE(m_run, ++m_ev));
      }
      usleep(100);
    } while (!done);
  }
  virtual void OnConfigure(const eudaq::Configuration & param) {
    SetStatus(eudaq::Status::LVL_OK, "Wait");
    try {
      std::cout << "Configuring." << std::endl;
      std::string filename = param["BitFile"];
      trigger_interval = param.Get("TriggerInterval", 0);
      dut_mask = param.Get("DutMask", 2);
      and_mask = param.Get("AndMask", 0xff);
      or_mask = param.Get("OrMask", 0);
      veto_mask = param.Get("VetoMask", 0);
      // ***
      SetTriggerInterval(trigger_interval);
      SetDUTMask(dut_mask);
      SetVetoMask(veto_mask);
      SetAndMask(and_mask);
      SetOrMask(or_mask);
      SetStatus(eudaq::Status::LVL_OK, "Configured");
    } catch (const std::exception & e) {
      printf("Caught exception: %s\n", e.what());
      SetStatus(eudaq::Status::LVL_ERROR, "Configuration Error");
    } catch (...) {
      printf("Unknown exception\n");
      SetStatus(eudaq::Status::LVL_ERROR, "Configuration Error");
    }
  }
  virtual void OnStartRun(unsigned param) {
    SetStatus(eudaq::Status::LVL_OK, "Wait");
    try {
      m_run = param;
      m_ev = 0;
      std::cout << "Start Run: " << param << std::endl;
      TLUEvent ev(TLUEvent::BORE(m_run));
      ev.SetTag("FirmwareID",  to_string(GetFirmwareID()));
      ev.SetTag("TriggerInterval",to_string(trigger_interval));
      ev.SetTag("DutMask",  "0x"+to_hex(dut_mask));
      ev.SetTag("AndMask",  "0x"+to_hex(and_mask));
      ev.SetTag("OrMask",   "0x"+to_hex(or_mask));
      ev.SetTag("VetoMask", "0x"+to_hex(veto_mask));
      sleep(2); // temporarily, to fix startup with EUDRB
      //      SendEvent(TLUEvent::BORE(m_run).SetTag("Interval",trigger_interval).SetTag("DUT",dut_mask));
      SendEvent(ev);
      Start();
      TLUStarted=true;
      SetStatus(eudaq::Status::LVL_OK, "Started");
    } catch (const std::exception & e) {
      printf("Caught exception: %s\n", e.what());
      SetStatus(eudaq::Status::LVL_ERROR, "Start Error");
    } catch (...) {
      printf("Unknown exception\n");
      SetStatus(eudaq::Status::LVL_ERROR, "Start Error");
    }
  }
  virtual void OnStopRun() {
    try {
      std::cout << "Stop Run" << std::endl;
      Stop();
      TLUStarted=false;
      TLUJustStopped=true;
      SetStatus(eudaq::Status::LVL_OK, "Stopped");
    } catch (const std::exception & e) {
      printf("Caught exception: %s\n", e.what());
      SetStatus(eudaq::Status::LVL_ERROR, "Stop Error");
    } catch (...) {
      printf("Unknown exception\n");
      SetStatus(eudaq::Status::LVL_ERROR, "Stop Error");
    }
  }
  virtual void OnTerminate() {
    std::cout << "Terminate (press enter)" << std::endl;
    done = true;
    sleep(2);
  }
  virtual void OnReset() {
    try {
      std::cout << "Reset" << std::endl;
      SetStatus(eudaq::Status::LVL_OK);
      Stop();   // stop
      Update(); // empty events
      SetStatus(eudaq::Status::LVL_OK, "Reset");
    } catch (const std::exception & e) {
      printf("Caught exception: %s\n", e.what());
      SetStatus(eudaq::Status::LVL_ERROR, "Reset Error");
    } catch (...) {
      printf("Unknown exception\n");
      SetStatus(eudaq::Status::LVL_ERROR, "Reset Error");
    }
  }
  virtual void OnStatus() {
    m_status.SetTag("TRIG", eudaq::to_string(m_ev));
    m_status.SetTag("TIMESTAMP", eudaq::to_string(Timestamp2Seconds(GetTimestamp())));
    m_status.SetTag("LASTTIME", eudaq::to_string(Timestamp2Seconds(lasttime)));
    //std::cout << "Status " << m_status << std::endl;
  }
  virtual void OnUnrecognised(const std::string & cmd, const std::string & param) {
    std::cout << "Unrecognised: (" << cmd.length() << ") " << cmd;
    if (param.length() > 0) std::cout << " (" << param << ")";
    std::cout << std::endl;
    SetStatus(eudaq::Status::LVL_WARN, "Unrecognised command");
  }
private:
  unsigned m_run, m_ev;
  unsigned trigger_interval, dut_mask, veto_mask, and_mask, or_mask;
  bool done;
  bool TLUStarted;
  bool TLUJustStopped;
  unsigned long long lasttime;
};

int main(int /*argc*/, const char ** argv) {
  eudaq::OptionParser op("EUDAQ TLU Producer", "1.0", "The Producer task for the Trigger Logic Unit");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol", "tcp://localhost:7000", "address",
                                   "The address of the RunControl application");
  eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level",
                                   "The minimum level for displaying log messages locally");
  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL(level.Value());
    TLUProducer producer(rctrl.Value());
    producer.MainLoop();
    std::cout << "Quitting" << std::endl;
    eudaq::mSleep(300);
  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}
