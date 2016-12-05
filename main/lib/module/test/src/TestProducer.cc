#include "eudaq/Producer.hh"
#include "eudaq/Logger.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Utils.hh"
#include <iostream>
#include <cctype>
using eudaq::RawDataEvent;

class TestProducer : public eudaq::Producer {
  public:
  TestProducer(const std::string & name, const std::string & runcontrol);
  void Event(unsigned size);
  void OnConfigure(const eudaq::Configuration & param) override final;
  void OnStartRun(unsigned param) override final;
  void OnStopRun() override final;
  void OnTerminate() override final;
  void OnReset() override final;
  void OnStatus() override final;
  void OnUnrecognised(const std::string & cmd, const std::string & param) override final;
  void Exec() override final;
  
  static const uint32_t m_id_factory = eudaq::cstr2hash("TestProducer");
private:
  bool m_done;
  uint32_t m_run;
  uint32_t m_ev;
  uint32_t m_eventsize;
  uint32_t m_id_stream;
};


namespace{
  auto dummy0 = eudaq::Factory<eudaq::Producer>::
    Register<TestProducer, const std::string&, const std::string&>(TestProducer::m_id_factory);
}


TestProducer::TestProducer(const std::string & name, const std::string & runcontrol)
  : eudaq::Producer(name, runcontrol), m_run(0), m_ev(0), m_done(false), m_eventsize(100) {
  m_id_stream = eudaq::cstr2hash(name.c_str());
}

void TestProducer::Event(unsigned size) {
  RawDataEvent ev("Test", m_id_stream, m_run, ++m_ev);
  std::vector<int> tmp((size+3)/4);
  for (size_t i = 0; i < tmp.size(); ++i) {
    tmp[i] = std::rand();
  }
  ev.AddBlock(0, &tmp[0], size);
  SendEvent(ev);
}

void TestProducer::OnConfigure(const eudaq::Configuration & param) {
  std::cout << "Configuring." << std::endl;
  m_eventsize = param.Get("EventSize", 1);
  eudaq::mSleep(2000);
  EUDAQ_INFO("Configured (" + param.Name() + ")");
  SetStatus(eudaq::Status::LVL_OK, "Configured (" + param.Name() + ")");
}

void TestProducer::OnStartRun(unsigned param) {
  m_run = param;
  m_ev = 0;
      
  eudaq::RawDataEvent ev("Test", m_id_stream, m_run, 0);
  ev.SetFlagBit(eudaq::Event::FLAG_BORE);
  SendEvent(ev);

  std::cout << "Start Run: " << param << std::endl;
  SetStatus(eudaq::Status::LVL_OK, "");
}

void TestProducer::OnStopRun() {
  eudaq::RawDataEvent ev("Test", m_id_stream, m_run, ++m_ev);
  ev.SetFlagBit(eudaq::Event::FLAG_EORE);
  SendEvent(ev);
  std::cout << "Stop Run" << std::endl;
}

void TestProducer::OnTerminate() {
  std::cout << "Terminate (press enter)" << std::endl;
  m_done = true;
}

void TestProducer::OnReset() {
  std::cout << "Reset" << std::endl;
  SetStatus(eudaq::Status::LVL_OK);
}

void TestProducer::OnStatus() {

}

void TestProducer::OnUnrecognised(const std::string & cmd, const std::string & param) {
  std::cout << "Unrecognised: (" << cmd.length() << ") " << cmd;
  if (param.length() > 0) std::cout << " (" << param << ")";
  std::cout << std::endl;
  SetStatus(eudaq::Status::LVL_WARN, "Just testing");
}

void TestProducer::Exec(){
  bool help = true;
  do {
    if (help) {
      help = false;
      std::cout << "--- Commands ---\n"
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
      cmd = tolower(line[0]);
      line = eudaq::trim(std::string(line, 1));
    } else {
      line = "";
    }
    switch (cmd) {
    case '\0': // ignore
      break;
    case 'r':
      Event(eudaq::from_string(line, 1024));
      break;
    case 'l':
      EUDAQ_USER(line);
      break;
    case 'o':
      SetStatus(eudaq::Status::LVL_OK, line);
      break;
    case 'w':
      SetStatus(eudaq::Status::LVL_WARN, line);
      break;
    case 'e':
      SetStatus(eudaq::Status::LVL_ERROR, line);
      break;
    case 'q':
      m_done = true;
      break;
    case '?':
      help = true;
      break;
    default:
      std::cout << "Unrecognised command, type ? for help" << std::endl;
    }
  } while (!m_done);
}
