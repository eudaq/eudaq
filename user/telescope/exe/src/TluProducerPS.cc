#include "eudaq/RawDataEvent.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Processor.hh"
#include <string>

#include "TLUController.hh"


using namespace tlu;
using namespace eudaq;

class TluProducerPS: public Processor{
public:
  TluProducerPS();
  ~TluProducerPS() override;
  void ProcessEvent(eudaq::EventSPC ev) final override{};
  void ProduceEvent() final override;
  void ProcessCommand(const std::string& cmd, const std::string& arg) final override;

  static constexpr const char* m_description = "TluProducerPS";
  static constexpr const uint32_t m_id_ps = cstr2hash(m_description);
private:
  std::shared_ptr<TLUController> m_tlu;
  uint32_t m_run;
  uint32_t m_ev;
  bool m_running;
  char* m_xxx;


  uint32_t trigger_interval;
  uint32_t dut_mask;
  uint32_t and_mask;
  uint32_t handshake_mode;
};

namespace{
  auto dummy0 = Factory<Processor>::
    Register<TluProducerPS>(cstr2hash(TluProducerPS::m_description));
}


TluProducerPS::TluProducerPS(): Processor(TluProducerPS::m_description){  
  m_tlu = std::make_shared<TLUController>(2);
  m_tlu->Stop();        // stop
  m_tlu->Update(false); // empty events

  trigger_interval = 1;
  dut_mask = 31;
  and_mask = 15;
  handshake_mode = 31;

}


TluProducerPS::~TluProducerPS(){
  if(m_tlu){
    m_tlu->Stop();
    m_tlu->Update(false); // empty events
  }
}

void TluProducerPS::ProduceEvent(){
  while(1){
    m_tlu->Update(true);
    if(m_tlu->NumEntries()==0)
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    for (size_t i = 0; i < m_tlu->NumEntries(); ++i){
      m_ev = m_tlu->GetEntry(i).Eventnum();
      uint64_t t = m_tlu->GetEntry(i).Timestamp();
      auto rawev = eudaq::RawDataEvent::MakeShared("TluRawDataEvent", GetInstanceN(),
						   m_run, m_ev++);
      rawev->SetTimestamp(t, t+1);
      if(m_running)
	RegisterEvent(rawev);
    }
    if(GetProducerStopFlag())
      break;
  }
}

void TluProducerPS::ProcessCommand(const std::string& key, const std::string& val){
  switch(str2hash(key)){ 
  case cstr2hash("StartRun"):{
    m_ev = 0;
    if(val.empty())
      m_run++;
    else
      m_run = std::stoul(val);
    auto rawev = eudaq::RawDataEvent::MakeShared("TluRawDataEvent", GetInstanceN(),
						 m_run, m_ev++);
    rawev->SetBORE();
    rawev->SetTag("TimestampZero", m_tlu->TimestampZero().Formatted());
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    RegisterEvent(rawev);
    m_tlu->ResetTimestamp();
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    m_tlu->ResetTriggerCounter();
    m_tlu->ResetTimestamp();
    m_tlu->ResetScalers();
    m_tlu->Update(true);
    m_running = true;
    m_tlu->Start();
    break;
  }
  case cstr2hash("StopRun"):{
    m_tlu->Stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    auto rawev = eudaq::RawDataEvent::MakeShared("TluRawDataEvent", GetInstanceN(),
						 m_run, m_ev++);
    rawev->SetEORE();
    RegisterEvent(rawev);
    break;
  }

  case cstr2hash("SendConf"):{
    m_tlu->SetDebugLevel(10);
    m_tlu->SetFirmware("tlu_toplevel_13_Nov_a.bit");
    m_tlu->SetVersion(0);
    m_tlu->Configure();

    m_tlu->SetTriggerInterval(trigger_interval);
    m_tlu->SetDUTMask(dut_mask);
    m_tlu->SetAndMask(and_mask);
    m_tlu->SetHandShakeMode(handshake_mode);
    m_tlu->SetTriggerInformation(USE_TRIGGER_INPUT_INFORMATION);
    m_tlu->ResetTimestamp();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    m_tlu->Update(true);
    break;
  }
  default:
    std::cout<<"unkonw user cmd"<<std::endl;
  }

    
}
