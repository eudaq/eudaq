#include "eudaq/Configuration.hh"
#include "eudaq/Producer.hh"
#include "eudaq/Logger.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Timer.hh"
#include "eudaq/Utils.hh"

#include "MinitluController.hh"
#include <iostream>
#include <ostream>
#include <vector>

using eudaq::to_string;
using eudaq::to_hex;
using namespace tlu;

class MinitluProducer: public eudaq::Producer {
public:
  MinitluProducer(const std::string name, const std::string &runcontrol);
  void MainLoop();
  void DoConfigure(const eudaq::Configuration & param) override;
  void DoStartRun(unsigned param) override;
  void DoStopRun() override;
  void DoTerminate() override;
  void DoReset() override;
  void OnStatus() override;
  void Exec() override;
  
  static const uint32_t m_id_factory = eudaq::cstr2hash("MinitluProducer");
private:
  unsigned m_run, m_ev;
  std::unique_ptr<miniTLUController> m_tlu;
  uint64_t m_lasttime;
  
  enum FSMState {
    STATE_ERROR,
    STATE_UNCONF,
    STATE_GOTOCONF,
    STATE_CONFED,
    STATE_GOTORUN,
    STATE_RUNNING,
    STATE_GOTOSTOP,
    STATE_GOTOTERM
  } m_fsmstate;
  
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::Producer>::
    Register<MinitluProducer, const std::string&, const std::string&>(MinitluProducer::m_id_factory);
}



MinitluProducer::MinitluProducer(const std::string name, const std::string &runcontrol)
  :eudaq::Producer(name, runcontrol), m_tlu(nullptr){
  m_fsmstate = STATE_UNCONF;
    
}


void MinitluProducer::Exec(){
  StartCommandReceiver();
  while(IsActiveCommandReceiver()){
    MainLoop();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
}


void MinitluProducer::MainLoop() {
  while (m_fsmstate != STATE_GOTOTERM){
    if (m_fsmstate == STATE_UNCONF) {
      eudaq::mSleep(100);
      continue;
    }
    if (m_fsmstate == STATE_RUNNING) {
      m_tlu->ReceiveEvents();
      while (!m_tlu->IsBufferEmpty()){
	minitludata * data = m_tlu->PopFrontEvent();	  
	m_ev = data->eventnumber;
	uint64_t t = data->timestamp;
	// TLUEvent ev(m_run, m_ev, t);
	auto ev = eudaq::RawDataEvent::MakeUnique("TluRawDataEvent");
	ev->SetTimestamp(t, t+1);//TODO, duration

	std::stringstream  triggerss;
	triggerss<< data->input3 << data->input2 << data->input1 << data->input0;
	ev->SetTag("trigger", triggerss.str());	  
	if(m_tlu->IsBufferEmpty()){
	  uint32_t sl0,sl1,sl2,sl3, pt;
	  m_tlu->GetScaler(sl0,sl1,sl2,sl3);
	  pt=m_tlu->GetPreVetoTriggers();  
	  ev->SetTag("SCALER0", to_string(sl0));
	  ev->SetTag("SCALER1", to_string(sl1));
	  ev->SetTag("SCALER2", to_string(sl2));
	  ev->SetTag("SCALER3", to_string(sl3));
	  ev->SetTag("PARTICLES", to_string(pt));
	}
	SendEvent(std::move(ev));
	m_ev++;
	delete data;
      }
      continue;
    }
      
    if (m_fsmstate == STATE_GOTOSTOP) {
      // SendEvent(TLUEvent::EORE(m_run, ++m_ev));
      m_ev++;
      auto ev = eudaq::RawDataEvent::MakeUnique("TluRawDataEvent");
      ev->SetEORE();
      SendEvent(std::move(ev));
      m_fsmstate = STATE_CONFED;
      continue;
    }
    eudaq::mSleep(100);
      
  };
}


void MinitluProducer::DoConfigure(const eudaq::Configuration & param) {
  if (m_tlu)
    m_tlu = nullptr;
     
  m_tlu = std::unique_ptr<miniTLUController>(new miniTLUController(param.Get("ConnectionFile","file:///dummy_connections.xml"),
								   param.Get("DeviceName","dummy.udp")));
  // m_tlu->SetWRegister("logic_clocks.LogicRst", 1);
  // eudaq::mSleep(100);
  // m_tlu->SetWRegister("logic_clocks.LogicRst", 0);
  // eudaq::mSleep(100);
  // m_tlu->SetLogicClocksCSR(0);
  // eudaq::mSleep(100);
	    
  m_tlu->ResetSerdes();
  m_tlu->ResetCounters();
  m_tlu->SetTriggerVeto(1);
  m_tlu->ResetFIFO();
  m_tlu->ResetEventsBuffer();      
      
  m_tlu->InitializeI2C(param.Get("I2C_DAC_Addr",0x1f), param.Get("I2C_ID_Addr",0x50));
  m_tlu->SetThresholdValue(0, param.Get("DACThreshold0",1.3));
  m_tlu->SetThresholdValue(1, param.Get("DACThreshold1",1.3));
  m_tlu->SetThresholdValue(2, param.Get("DACThreshold2",1.3));
  m_tlu->SetThresholdValue(3, param.Get("DACThreshold3",1.3));
      

  m_tlu->SetDUTMask(param.Get("DUTMask",1));
  m_tlu->SetDUTMaskMode(param.Get("DUTMaskMode",0xff));
  m_tlu->SetDUTMaskModeModifier(param.Get("DUTMaskModeModifier",0xff));
  m_tlu->SetDUTIgnoreBusy(param.Get("DUTIgnoreBusy",7));
  m_tlu->SetDUTIgnoreShutterVeto(param.Get("DUTIgnoreShutterVeto",1));//ILC stuff related
  m_tlu->SetEnableRecordData(param.Get("EnableRecordData", 1));
  m_tlu->SetInternalTriggerInterval(param.Get("InternalTriggerInterval",0)); // 160M/interval
  m_tlu->SetTriggerMask(param.Get("TriggerMask",0));
  m_tlu->SetPulseStretch(param.Get("PluseStretch ",0));
  m_tlu->SetPulseDelay(param.Get("PluseDelay",0));
      
  m_fsmstate=STATE_CONFED;
}

void MinitluProducer::DoStartRun(unsigned param) {    
  m_tlu->ResetCounters();
  m_tlu->ResetEventsBuffer();
  m_tlu->ResetFIFO();
  m_tlu->SetTriggerVeto(0);;

  m_run = param;
  m_ev = 0;
  std::cout << "Start Run: " << param << std::endl;
  // TLUEvent ev(TLUEvent::BORE(m_run));
  auto ev = eudaq::RawDataEvent::MakeUnique("TluRawDataEvent");
  ev->SetBORE();
  ev->SetTag("FirmwareID", to_string(m_tlu->GetFirmwareVersion()));
  ev->SetTag("BoardID", to_string(m_tlu->GetBoardID()));
  SendEvent(std::move(ev));

  m_lasttime=m_tlu->GetCurrentTimestamp()/40000000;
  m_fsmstate = STATE_RUNNING;
      
}

void MinitluProducer::DoStopRun() {
  m_tlu->SetTriggerVeto(1);
  m_fsmstate = STATE_GOTOSTOP;
  while (m_fsmstate == STATE_GOTOSTOP) {
    eudaq::mSleep(100); //waiting for EORE being send
  }
}

void MinitluProducer::DoTerminate() {
  m_fsmstate = STATE_GOTOTERM;
  eudaq::mSleep(1000);
}

void MinitluProducer::DoReset() {
  m_tlu->SetTriggerVeto(1);
  //TODO:: stop_tlu
}

void MinitluProducer::OnStatus() {
  if (m_tlu) {
    uint64_t time = m_tlu->GetCurrentTimestamp();
    time = time/40000000; // in second
    SetStatusTag("TIMESTAMP", to_string(time));
    SetStatusTag("LASTTIME", to_string(m_lasttime));
    m_lasttime = time;
      
    uint32_t sl0,sl1,sl2,sl3, pret, post;
    pret=m_tlu->GetPreVetoTriggers();
    post=m_tlu->GetPostVetoTriggers();  
    m_tlu->GetScaler(sl0,sl1,sl2,sl3);
    SetStatusTag("SCALER0", to_string(sl0));
    SetStatusTag("SCALER1", to_string(sl1));
    SetStatusTag("SCALER2", to_string(sl2));
    SetStatusTag("SCALER3", to_string(sl3));
    SetStatusTag("PARTICLES", to_string(pret));
    SetStatusTag("TRIG", to_string(post));
      
  }
}

  


