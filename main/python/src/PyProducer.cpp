#include "eudaq/Producer.hh"
#include "eudaq/Logger.hh"
#include "eudaq/StringEvent.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Utils.hh"
#include "eudaq/OptionParser.hh"
#include <iostream>

using eudaq::StringEvent;
using eudaq::RawDataEvent;

class PyProducer : public eudaq::Producer {
  public:
    PyProducer(const std::string & name, const std::string & runcontrol)
      : eudaq::Producer(name, runcontrol), m_run(0), m_ev(0), done(false), eventsize(100) {}
    void Event(const std::string & str) {
      StringEvent ev(m_run, ++m_ev, str);
      //ev.SetTag("Debug", "foo");
      SendEvent(ev);
    }
    void RandomEvent(unsigned size) {
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
      SetStatus(eudaq::Status::LVL_OK, "Configured (" + param.Name() + ")");
    }
    virtual void OnStartRun(unsigned param) {
      m_run = param;
      m_ev = 0;
      SendEvent(RawDataEvent::BORE("Test", m_run));
      std::cout << "Start Run: " << param << std::endl;
      SetStatus(eudaq::Status::LVL_OK, "");
    }
    virtual void OnStopRun() {
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
};

// ctypes can only talk to C functions -- need to provide them through 'extern "C"'
extern "C" {
  PyProducer* PyProducer_new(char *name, char *rcaddress){return new PyProducer(std::string(name),std::string(rcaddress));}
  void PyProducer_Event(PyProducer *pp, char *data){pp->Event(std::string(data));}
  void PyProducer_RandomEvent(PyProducer *pp, unsigned size){pp->RandomEvent(size);}

}
