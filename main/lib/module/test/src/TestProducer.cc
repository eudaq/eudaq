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

  void DoConfigure() override final;
  void DoStartRun() override final;
  void DoStopRun() override final;
  void DoTerminate() override final;
  void DoReset() override final;
  
  void OnStatus() override final;
  void OnUnrecognised(const std::string & cmd, const std::string & param) override final;
  void Exec() override final;
  
  static const uint32_t m_id_factory = eudaq::cstr2hash("TestProducer");
private:
  bool m_exit;
};


namespace{
  auto dummy0 = eudaq::Factory<eudaq::Producer>::
    Register<TestProducer, const std::string&, const std::string&>(TestProducer::m_id_factory);
}


TestProducer::TestProducer(const std::string & name, const std::string & runcontrol)
  : eudaq::Producer(name, runcontrol), m_exit(false){
  
}

void TestProducer::Event(unsigned size) {
  eudaq::EventUP ev = eudaq::RawDataEvent::MakeUnique("Test");
  std::vector<int> tmp((size+3)/4);
  for (size_t i = 0; i < tmp.size(); ++i) {
    tmp[i] = std::rand();
  }
  RawDataEvent *evraw = dynamic_cast<RawDataEvent*>(ev.get());
  evraw->AddBlock(0, &tmp[0], size);
  SendEvent(std::move(ev));
}

void TestProducer::DoConfigure() {
  std::cout << "Configuring." << std::endl;
  auto conf = GetConfiguration();
  uint32_t eventsize = conf->Get("EventSize", 1);
  std::cout<< "EventSize = "<< eventsize;
}

void TestProducer::DoStartRun() {
  auto ev = eudaq::RawDataEvent::MakeUnique("Test");
  ev->SetBORE();
  SendEvent(std::move(ev));
  std::cout << "TestProducer: Start Run: " << GetRunNumber() << std::endl;
}

void TestProducer::DoStopRun() {
  eudaq::EventUP ev = eudaq::RawDataEvent::MakeUnique("Test");
  ev->SetEORE();
  SendEvent(std::move(ev));
  std::cout << "TestProducer: Stop Run" << std::endl;
}

void TestProducer::DoTerminate() {
  std::cout << "TestProducer: Terminate" << std::endl;
  m_exit = true;
}

void TestProducer::DoReset() {
  std::cout << "TestProducer: Reset" << std::endl;
}


void TestProducer::OnStatus() {
  
}

void TestProducer::OnUnrecognised(const std::string & cmd, const std::string & param) {
  std::cout << "TestProducer: Unrecognised cmd (" << cmd.length() << ") " << cmd;
  if (param.length() > 0) std::cout << " (" << param << ")";
  std::cout << std::endl;
  SetStatus(eudaq::Status::LVL_WARN, "Just testing");
}

void TestProducer::Exec(){
  StartCommandReceiver();
  bool help = true;
  while(IsActiveCommandReceiver()){
    if(help) {
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
      // exit();
      break;
    case '?':
      help = true;
      break;
    default:
      std::cout << "Unrecognised command, type ? for help" << std::endl;
    }
  }
  std::cout << "TestProducer: Terminated" << std::endl;
}
