#include "eudaq/Producer.hh"


class DummyProducer : public eudaq::Producer {
public:
  DummyProducer(const std::string name, const std::string &runcontrol);
  ~DummyProducer() override;
  void DoInitialise() override;
  void DoConfigure() override;
  void DoStartRun() override;
  void DoStopRun() override;
  void DoReset() override;
  void DoTerminate() override;
  void RunLoop() override;

  static const uint32_t m_id_factory = eudaq::cstr2hash("DummyProducer");
private:
  bool m_running;
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::Producer>::
    Register<DummyProducer, const std::string&, const std::string&>(DummyProducer::m_id_factory);
}

DummyProducer::DummyProducer(const std::string name, const std::string &runcontrol)
  : eudaq::Producer(name, runcontrol), m_running(false){
  m_running = false;
}

DummyProducer::~DummyProducer(){
  m_running = false;
}

void DummyProducer::RunLoop(){
 
}

void DummyProducer::DoInitialise(){
  auto conf = GetInitConfiguration();  
 
}

void DummyProducer::DoConfigure(){
  auto conf = GetConfiguration();
  
}

void DummyProducer::DoStartRun(){
  m_running = true;
}

void DummyProducer::DoStopRun(){
  m_running = false;
}

void DummyProducer::DoReset(){
  m_running = false;
}

void DummyProducer::DoTerminate() {
  m_running = false;
}
