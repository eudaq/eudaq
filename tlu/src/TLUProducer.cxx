#include "eudaq/Producer.hh"
#include "eudaq/TLUEvent.hh" // for the TLU event
#include "eudaq/Utils.hh"
#include "eudaq/Logger.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/counted_ptr.hh"
#include "tlu/TLUController.hh"
#include "tlu/USBTracer.hh"
#include <iostream>
#include <ostream>
#include <cctype>

typedef eudaq::TLUEvent TLUEvent;
using eudaq::to_string;
using eudaq::to_hex;
using namespace tlu;

class TLUProducer : public eudaq::Producer {
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
      trig_rollover(0),
      done(false),
      TLUStarted(false),
      TLUJustStopped(false),
      lasttime(0),
      m_tlu(0)
    {}
  void Event(unsigned tlu_evntno, const long long int & tlu_timestamp) {
    SendEvent(TLUEvent(m_run, tlu_evntno, tlu_timestamp)); // not the right event number!!
  }
  void MainLoop() {
    do {
      usleep(100);
      if (!m_tlu) continue;
      bool JustStopped = TLUJustStopped;
      if (JustStopped) {
        m_tlu->Stop();
        eudaq::mSleep(100);
      }
      if (TLUStarted || JustStopped) {
        eudaq::mSleep(100);
        m_tlu->Update(timestamps); // get new events
        if (trig_rollover > 0 && m_tlu->GetTriggerNum() > trig_rollover) {
          bool inhibit = m_tlu->InhibitTriggers();
          m_tlu->ResetTriggerCounter();
          m_tlu->InhibitTriggers(inhibit);
        }
        //std::cout << "--------" << std::endl;
        for (size_t i = 0; i < m_tlu->NumEntries(); ++i) {
          m_ev = m_tlu->GetEntry(i).Eventnum();
          unsigned long long t = m_tlu->GetEntry(i).Timestamp();
          long long d = t-lasttime;
          float freq= 1./(d*20./1000000000);
          std::cout << "  " << m_tlu->GetEntry(i)
                    << ", diff=" << d << (d <= 0 ? "  ***" : "")
                    << ", freq=" << freq
                    << std::endl;
          lasttime = t;
          Event(m_ev,t);
        }
        if (m_tlu->NumEntries()) {
          std::cout << "========" << std::endl;
        } else {
          std::cout << "." << std::flush;
        }
      }
      if (JustStopped) {
        SendEvent(TLUEvent::EORE(m_run, ++m_ev));
        TLUJustStopped = false;
      }
    } while (!done);
  }
  virtual void OnConfigure(const eudaq::Configuration & param) {
    SetStatus(eudaq::Status::LVL_OK, "Wait");
    try {
      std::cout << "Configuring." << std::endl;
      if (m_tlu) m_tlu = 0;
      int errorhandler = param.Get("ErrorHandler", 2);
      m_tlu = counted_ptr<TLUController>(new TLUController(errorhandler));

      trigger_interval = param.Get("TriggerInterval", 0);
      dut_mask = param.Get("DutMask", 2);
      and_mask = param.Get("AndMask", 0xff);
      or_mask = param.Get("OrMask", 0);
      veto_mask = param.Get("VetoMask", 0);
      trig_rollover = param.Get("TrigRollover", 0);
      timestamps = param.Get("Timestamps", 1);
      // ***
      m_tlu->SetFirmware(param.Get("BitFile", ""));
      m_tlu->SetVersion(param.Get("Version", 0));
      m_tlu->Configure();
      m_tlu->SetTriggerInterval(trigger_interval);
      m_tlu->SetDUTMask(dut_mask);
      m_tlu->SetVetoMask(veto_mask);
      m_tlu->SetAndMask(and_mask);
      m_tlu->SetOrMask(or_mask);

      // by dhaas
      sleep(2);

      std::cout << "...Configured (" << param.Name() << ")" << std::endl;
      EUDAQ_INFO("Configured (" + param.Name() + ")");
      SetStatus(eudaq::Status::LVL_OK, "Configured (" + param.Name() + ")");
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
      m_tlu->ResetTriggerCounter();
      std::cout << "Start Run: " << param << std::endl;
      TLUEvent ev(TLUEvent::BORE(m_run));
      ev.SetTag("FirmwareID",  to_string(m_tlu->GetFirmwareID()));
      ev.SetTag("TriggerInterval",to_string(trigger_interval));
      ev.SetTag("DutMask",  "0x"+to_hex(dut_mask));
      ev.SetTag("AndMask",  "0x"+to_hex(and_mask));
      ev.SetTag("OrMask",   "0x"+to_hex(or_mask));
      ev.SetTag("VetoMask", "0x"+to_hex(veto_mask));
      sleep(5); // temporarily, to fix startup with EUDRB
      //      SendEvent(TLUEvent::BORE(m_run).SetTag("Interval",trigger_interval).SetTag("DUT",dut_mask));
      SendEvent(ev);
      sleep(5);
      m_tlu->Start();
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
      TLUStarted=false;
      TLUJustStopped=true;
      while (TLUJustStopped) {
        eudaq::mSleep(100);
      }
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
    sleep(1);
  }
  virtual void OnReset() {
    try {
      std::cout << "Reset" << std::endl;
      SetStatus(eudaq::Status::LVL_OK);
      m_tlu->Stop();   // stop
      m_tlu->Update(false); // empty events
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
    m_status.SetTag("TIMESTAMP", eudaq::to_string(Timestamp2Seconds(m_tlu ? m_tlu->GetTimestamp() : 0)));
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
  unsigned trig_rollover;
  bool timestamps, done;
  bool TLUStarted;
  bool TLUJustStopped;
  unsigned long long lasttime;
  counted_ptr<TLUController> m_tlu;
};

int main(int /*argc*/, const char ** argv) {
  eudaq::OptionParser op("EUDAQ TLU Producer", "1.0", "The Producer task for the Trigger Logic Unit");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol", "tcp://localhost:44000", "address",
                                   "The address of the RunControl application");
  eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level",
                                   "The minimum level for displaying log messages locally");
  eudaq::Option<std::string> op_trace(op, "t", "tracefile", "", "filename", "Log file for tracing USB access");
  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL(level.Value());
    if (op_trace.Value() != "") {
      setusbtracefile(op_trace.Value());
    }
    TLUProducer producer(rctrl.Value());
    producer.MainLoop();
    std::cout << "Quitting" << std::endl;
    eudaq::mSleep(300);
  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}
