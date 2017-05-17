#include "eudaq/Producer.hh"

#include "MinitluController.hh"
#include "TLUhardware.hh"
#include <iostream>
#include <ostream>
#include <vector>


class MinitluProducer: public eudaq::Producer {
public:
  MinitluProducer(const std::string name, const std::string &runcontrol);
  void DoConfigure() override;
  void DoStartRun() override;
  void DoStopRun() override;
  void DoTerminate() override;
  void DoReset() override;

  void OnStatus() override;

  void MainLoop();

  static const uint32_t m_id_factory = eudaq::cstr2hash("MinitluProducer");
private:

  std::thread m_thd_run;
  bool m_exit_of_run;

  std::unique_ptr<tlu::miniTLUController> m_tlu;
  uint64_t m_lasttime;
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::Producer>::
    Register<MinitluProducer, const std::string&, const std::string&>(MinitluProducer::m_id_factory);
}


MinitluProducer::MinitluProducer(const std::string name, const std::string &runcontrol)
  :eudaq::Producer(name, runcontrol){

}

void MinitluProducer::MainLoop(){
  bool isbegin = true;

  m_tlu->ResetCounters();
  m_tlu->ResetEventsBuffer();
  m_tlu->ResetFIFO();
  m_tlu->SetTriggerVeto(0);
  while(true) {
    m_lasttime=m_tlu->GetCurrentTimestamp()*25;
    m_tlu->ReceiveEvents();
    while (!m_tlu->IsBufferEmpty()){
      tlu::minitludata *data = m_tlu->PopFrontEvent();
      uint32_t trigger_n = data->eventnumber;
      uint64_t ts_raw = data->timestamp;
      // uint64_t ts_ns = ts_raw; //TODO: to ns
      uint64_t ts_ns = ts_raw*25;
      auto ev = eudaq::Event::MakeUnique("TluRawDataEvent");
      ev->SetTimestamp(ts_ns, ts_ns+25, false);//TODO, duration
      ev->SetTriggerN(trigger_n);

      std::stringstream  triggerss;
      triggerss<< data->input3 << data->input2 << data->input1 << data->input0;
      ev->SetTag("trigger", triggerss.str());
      if(m_tlu->IsBufferEmpty()){
	uint32_t sl0,sl1,sl2,sl3, sl4, sl5, pt;
	m_tlu->GetScaler(sl0,sl1,sl2,sl3,sl4,sl5);
	pt=m_tlu->GetPreVetoTriggers();
	ev->SetTag("SCALER0", std::to_string(sl0));
	ev->SetTag("SCALER1", std::to_string(sl1));
	ev->SetTag("SCALER2", std::to_string(sl2));
	ev->SetTag("SCALER3", std::to_string(sl3));
	ev->SetTag("PARTICLES", std::to_string(pt));

	if(m_exit_of_run){
	  ev->SetEORE();
	}
      }

      if(isbegin){
	isbegin = false;
	ev->SetBORE();
	ev->SetTag("FirmwareID", std::to_string(m_tlu->GetFirmwareVersion()));
	ev->SetTag("BoardID", std::to_string(m_tlu->GetBoardID()));
      }
      SendEvent(std::move(ev));
      delete data;
    }
  }
  m_tlu->SetTriggerVeto(1);
}

void MinitluProducer::DoConfigure() {
  auto conf = GetConfiguration();

  //std::string uhal_conn = "file:///dummy_connections.xml";
  std::string uhal_conn = "file://./dummy_TLU_connections.xml";
  std::string uhal_node = "dummy.udp";
  uhal_conn = conf->Get("ConnectionFile", uhal_conn);
  uhal_node = conf->Get("DeviceName",uhal_node);
  m_tlu = std::unique_ptr<tlu::miniTLUController>(new tlu::miniTLUController(uhal_conn, uhal_node));

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

  //Populate address list for I2C elements
  m_tlu->SetI2C_core_addr(conf->Get("I2C_COREEXP_Addr", 0x21));
  m_tlu->SetI2C_clockChip_addr(conf->Get("I2C_CLK_Addr", 0x68));
  m_tlu->SetI2C_DAC1_addr(conf->Get("I2C_DAC1_Addr",0x13) );
  m_tlu->SetI2C_DAC2_addr(conf->Get("I2C_DAC2_Addr",0x1f) );
  m_tlu->SetI2C_EEPROM_addr(conf->Get("I2C_ID_Addr", 0x50) );
  m_tlu->SetI2C_expander1_addr( conf->Get("I2C_EXP1_Addr",0x74));
  m_tlu->SetI2C_expander2_addr(conf->Get("I2C_EXP2_Addr",0x75) );

  // Initialize TLU hardware and registers
  m_tlu->InitializeI2C();
  m_tlu->InitializeDAC();
  m_tlu->InitializeIOexp();
  m_tlu->InitializeClkChip(conf->Get("CLOCK_CFG_FILE","./confClk.txt")  );

  // Enable HDMI connectors
  m_tlu->enableHDMI(1, conf->Get("HDMI1_on", true), false);
  m_tlu->enableHDMI(2, conf->Get("HDMI2_on", true), false);
  m_tlu->enableHDMI(3, conf->Get("HDMI3_on", true), false);
  m_tlu->enableHDMI(4, conf->Get("HDMI4_on", true), false);

  // Select clock to HDMI
  m_tlu->SetDutClkSrc(1, conf->Get("HDMI1_clk", 1), false);
  m_tlu->SetDutClkSrc(2, conf->Get("HDMI2_clk", 1), false);
  m_tlu->SetDutClkSrc(3, conf->Get("HDMI3_clk", 1), false);
  m_tlu->SetDutClkSrc(4, conf->Get("HDMI4_clk", 1), false);

  //Set lemo clock
  m_tlu->enableClkLEMO(conf->Get("LEMOclk", true), false);

  // Set thresholds
  m_tlu->SetThresholdValue(0, conf->Get("DACThreshold0", 1.2));
  m_tlu->SetThresholdValue(1, conf->Get("DACThreshold1", 1.2));
  m_tlu->SetThresholdValue(2, conf->Get("DACThreshold2", 1.2));
  m_tlu->SetThresholdValue(3, conf->Get("DACThreshold3", 1.2));
  m_tlu->SetThresholdValue(4, conf->Get("DACThreshold0", 1.2));
  m_tlu->SetThresholdValue(5, conf->Get("DACThreshold1", 1.2));

  // Set trigger stretch and delay
  std::vector<unsigned int> stretcVec = {(unsigned int)conf->Get("in0_STR",0),
                                        (unsigned int)conf->Get("in1_STR",0),
                                        (unsigned int)conf->Get("in2_STR",0),
                                        (unsigned int)conf->Get("in3_STR",0),
                                        (unsigned int)conf->Get("in4_STR",0),
                                        (unsigned int)conf->Get("in5_STR",0)};

  std::vector<unsigned int> delayVec = {(unsigned int)conf->Get("in0_DEL",0),
                                        (unsigned int)conf->Get("in1_DEL",0),
                                        (unsigned int)conf->Get("in2_DEL",0),
                                        (unsigned int)conf->Get("in3_DEL",0),
                                        (unsigned int)conf->Get("in4_DEL",0),
                                        (unsigned int)conf->Get("in5_DEL",0)};
  m_tlu->SetPulseStretchPack(stretcVec);
  m_tlu->SetPulseDelayPack(delayVec);
  //std::cout <<  "Stretch " << (int)m_tlu->GetPulseStretch() << " delay " << (int)m_tlu->GetPulseDelay()  << std::endl;

  // Set triggerMask
  // The conf function does not seem happy with a 32-bit default. Need to check.
  m_tlu->SetTriggerMask( (uint32_t)(conf->Get("trigMaskHi", 0xFFFF)),  (uint32_t)(conf->Get("trigMaskLo", 0xFFFE)) );

  m_tlu->SetDUTMask(conf->Get("DUTMask",1)); // Which DUTs are on
  m_tlu->SetDUTMaskMode(conf->Get("DUTMaskMode",0xff)); // AIDA (x1) or EUDET (x0)
  m_tlu->SetDUTMaskModeModifier(conf->Get("DUTMaskModeModifier",0xff)); // Only for EUDET
  m_tlu->SetDUTIgnoreBusy(conf->Get("DUTIgnoreBusy",0xF)); // Ignore busy in AIDA mode
  m_tlu->SetDUTIgnoreShutterVeto(conf->Get("DUTIgnoreShutterVeto",1));//ILC stuff related
  m_tlu->SetEnableRecordData(conf->Get("EnableRecordData", 1));
  m_tlu->SetInternalTriggerInterval(conf->Get("InternalTriggerInterval",0)); // 160M/interval
  m_tlu->GetEventFifoCSR();
  m_tlu->GetEventFifoFillLevel();
}

void MinitluProducer::DoStartRun(){
  m_exit_of_run = false;
  m_thd_run = std::thread(&MinitluProducer::MainLoop, this);
}

void MinitluProducer::DoStopRun(){
  m_exit_of_run = true;
  if(m_thd_run.joinable())
    m_thd_run.join();
}

void MinitluProducer::DoTerminate(){
  m_exit_of_run = true;
  if(m_thd_run.joinable())
    m_thd_run.join();
}

void MinitluProducer::DoReset(){
  m_exit_of_run = true;
  if(m_thd_run.joinable())
    m_thd_run.join();

  m_tlu.reset();
}

void MinitluProducer::OnStatus() {
  if (m_tlu) {
    uint64_t time = m_tlu->GetCurrentTimestamp();
    time = time/40000000; // in second

    SetStatusTag("TIMESTAMP", std::to_string(time));
    SetStatusTag("LASTTIME", std::to_string(m_lasttime));

    uint32_t sl0,sl1,sl2,sl3,sl4,sl5, pret, post;
    pret=m_tlu->GetPreVetoTriggers();
    post=m_tlu->GetPostVetoTriggers();
    m_tlu->GetScaler(sl0,sl1,sl2,sl3,sl4,sl5);
    SetStatusTag("SCALER0", std::to_string(sl0));
    SetStatusTag("SCALER1", std::to_string(sl1));
    SetStatusTag("SCALER2", std::to_string(sl2));
    SetStatusTag("SCALER3", std::to_string(sl3));
    SetStatusTag("SCALER4", std::to_string(sl4));
    SetStatusTag("SCALER5", std::to_string(sl5));
    SetStatusTag("PARTICLES", std::to_string(pret));
    SetStatusTag("TRIG", std::to_string(post));
  }
}
