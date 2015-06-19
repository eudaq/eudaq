#include "eudaq/Producer.hh"
#include "eudaq/Logger.hh"
#include "eudaq/StringEvent.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Utils.hh"
#include "eudaq/OptionParser.hh"
#include <iostream>
#include <ostream>
#include <cctype>
#include <cstdlib>

// class TestEvent : public eudaq::Event {
// public:
//   TestEvent(const std::string & text) : m_text(text) {}
// private:
//   virtual std::string Serialize() const { return m_text; }
//   std::string m_text;
// };

using eudaq::StringEvent;
using eudaq::RawDataEvent;

class LoadProducer : public eudaq::Producer {
  public:
    LoadProducer(const std::string & name, const std::string & runcontrol)
      : eudaq::Producer(name, runcontrol), m_run(0), m_ev(0), done(false), eventsize(100), running(false) {}
    void Event(const std::string & str) {
      StringEvent ev(m_run, ++m_ev, str);
      //ev.SetTag("Debug", "foo");
      SendEvent(ev);
    }
    void Event(unsigned size) {
      RawDataEvent ev("Test", m_run, ++m_ev);
      for (int j=0; j<10; j++) {
	std::vector<int> tmp((size+3)/4);
	for (size_t i = 0; i < tmp.size(); ++i) {
	  tmp[i] = std::rand();
	}
	ev.AddBlock(j, &tmp[0], size);
      }
      //ev.SetTag("Debug", "foo");
      SendEvent(ev);
    }
    virtual void OnConfigure(const eudaq::Configuration & param) {
      std::cout << "Configuring." << std::endl;
      eventsize = param.Get("EventSize", 1);
      eudaq::mSleep(2000);
      EUDAQ_INFO("Configured (" + param.Name() + ")");
      SetStatus(eudaq::Status::LVL_OK, "Configured (" + param.Name() + ")");
    }
    virtual void OnStartRun(unsigned param) {
      m_run = param;
      m_ev = 0;
      SendEvent(RawDataEvent::BORE("Test", m_run));
      std::cout << "Start Run: " << param << std::endl;
      SetStatus(eudaq::Status::LVL_OK, "");
      running = true;
    }
    virtual void OnStopRun() {
      running = false;
      SendEvent(RawDataEvent::EORE("Test", m_run, ++m_ev));
      std::cout << "Stop Run" << std::endl;
    }
    virtual void OnTerminate() {
      std::cout << "Terminate (press enter)" << std::endl;
      done = true;
    }
    virtual void OnReset() {
      std::cout << "Reset" << std::endl;
      SetStatus(eudaq::Status::LVL_OK);
    }
    virtual void OnStatus() {
      //std::cout << "Status - " << m_status << std::endl;
      //SetStatus(eudaq::Status::WARNING, "Only joking");
    }
    virtual void OnUnrecognised(const std::string & cmd, const std::string & param) {
      std::cout << "Unrecognised: (" << cmd.length() << ") " << cmd;
      if (param.length() > 0) std::cout << " (" << param << ")";
      std::cout << std::endl;
      SetStatus(eudaq::Status::LVL_WARN, "Just testing");
    }
    unsigned m_run, m_ev;
    bool done;
    unsigned eventsize;
    bool running;
};

int main(int /*argc*/, const char ** argv) {
  eudaq::OptionParser op("EUDAQ Producer", "1.0", "A comand-line version of a dummy Producer");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol", "tcp://localhost:44000", "address",
      "The address of the RunControl application");
  eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level",
      "The minimum level for displaying log messages locally");
  eudaq::Option<std::string> name (op, "n", "name", "Test", "string",
      "The name of this Producer");
  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL(level.Value());
    LoadProducer producer(name.Value(), rctrl.Value());

    int count = 0;
    
    do {
      
      if (!producer.running) {
	eudaq::mSleep(20); 
	continue;
      }
      
      if (count++ % 1000 == 0)
	std::cout << "Sending event " << count << std::endl;
      
      producer.Event(80);

//       eudaq::mSleep(10); 
      
    } while (!producer.done);
    std::cout << "Quitting" << std::endl;
  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}
