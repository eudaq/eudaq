#include "eudaq/Configuration.hh"
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

class TestProducer : public eudaq::Producer {
  public:
    TestProducer(const std::string & name, const std::string & runcontrol)
      : eudaq::Producer(name, runcontrol), m_run(0), m_ev(0), done(false), eventsize(100) {}
    void Event(const std::string & str) {
      StringEvent ev(m_run, ++m_ev, str);
      //ev.SetTag("Debug", "foo");
      SendEvent(ev);
    }
    void Event(unsigned size) {
      RawDataEvent ev("Test", m_run, ++m_ev);
      std::vector<int> tmp((size+3)/4);
      for (size_t i = 0; i < tmp.size(); ++i) {
        tmp[i] = std::rand();
      }
      ev.AddBlock(0, &tmp[0], size);
      //ev.SetTag("Debug", "foo");
      SendEvent(ev);
    }
    virtual void OnConfigure(const eudaq::Configuration & param) {
      std::cout << "Configuring." << std::endl;
      eventsize = param.Get("EventSize", 1);
      eudaq::mSleep(2000);
      EUDAQ_INFO("Configured (" + param.Name() + ")");
      SetConnectionState( eudaq::ConnectionState::STATE_CONF, "Configured (" + param.Name() + ")");
    }
    virtual void OnStartRun(unsigned param) {
      m_run = param;
      m_ev = 0;
      SendEvent(RawDataEvent::BORE("Test", m_run));
      std::cout << "Start Run: " << param << std::endl;
      SetConnectionState(eudaq::ConnectionState::STATE_RUNNING);
    }
    virtual void OnStopRun() {
      SendEvent(RawDataEvent::EORE("Test", m_run, ++m_ev));
      std::cout << "Stop Run" << std::endl;
      if(m_connectionstate.GetState() != eudaq::ConnectionState::STATE_ERROR)
        SetConnectionState(eudaq::ConnectionState::STATE_CONF);
    }
    virtual void OnTerminate() {
      std::cout << "Terminate (press enter)" << std::endl;
      done = true;
    }
    virtual void OnReset() {
      std::cout << "Reset" << std::endl;
      //SetConnectionState(eudaq::ConnectionState::STATE_UNCONF);
    }
    virtual void OnConnectionState() {
      //std::cout << "ConnectionState - " << m_connectionstate << std::endl;
      //SetConnectionState(eudaq::ConnectionState::WARNING, "Only joking");
    }
    virtual void OnUnrecognised(const std::string & cmd, const std::string & param) {
      std::cout << "Unrecognised: (" << cmd.length() << ") " << cmd;
      if (param.length() > 0) std::cout << " (" << param << ")";
      std::cout << std::endl;
      //SetConnectionState(eudaq::ConnectionState::LVL_WARN, "Just testing");
    }
    unsigned m_run, m_ev;
    bool done;
    unsigned eventsize;
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
    TestProducer producer(name.Value(), rctrl.Value());
    bool help = true;
    do {
      if (help) {
        help = false;
        std::cout << "--- Commands ---\n"
          << "s data Send StringEvent with 'data' as payload\n"
          << "r size Send RawDataEvent with size bytes of random data (default=1k)\n"
          << "l msg  Send log message\n"
          << "o msg  Set status=OK\n"
          << "w msg  Set status=WARN\n"
          << "e msg  Set status=ERROR\n"
          << "q      Quit\n"
          << "?      \n"
          << "----------------" << std::endl;
      }
      std::string line;
      std::getline(std::cin, line);
      //std::cout << "Line=\'" << line << "\'" << std::endl;
      char cmd = '\0';
      if (line.length() > 0) {
        cmd = std::tolower(line[0]);
        line = eudaq::trim(std::string(line, 1));
      } else {
        line = "";
      }
      switch (cmd) {
        case '\0': // ignore
          break;
        case 's':
          producer.Event(line);
          break;
        case 'r':
          producer.Event(eudaq::from_string(line, 1024));
          break;
        case 'l':
          EUDAQ_USER(line);
          break;
        case 'o':
          EUDAQ_LOG(OK, line);
          break;
        case 'w':
          EUDAQ_LOG(WARN, line);
          break;
        case 'e':
          EUDAQ_LOG(ERROR, line);
          break;
        case 'q':
          producer.done = true;
          break;
        case '?':
          help = true;
          break;
        default:
          std::cout << "Unrecognised command, type ? for help" << std::endl;
      }
    } while (!producer.done);
    std::cout << "Quitting" << std::endl;
  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}
