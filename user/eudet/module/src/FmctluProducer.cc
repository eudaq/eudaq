#include "eudaq/Producer.hh"

#include "FmctluController.hh"
#include "FmctluHardware.hh"
#include <iostream>
#include <ostream>
#include <vector>


class FmctluProducer: public eudaq::Producer {
public:
  FmctluProducer(const std::string name, const std::string &runcontrol);
  void DoConfigure() override;
  void DoInitialise() override;
  void DoStartRun() override;
  void DoStopRun() override;
  void DoTerminate() override;
  void DoReset() override;

  void OnStatus() override;

  void MainLoop();

  static const uint32_t m_id_factory = eudaq::cstr2hash("FmctluProducer");
private:

  std::thread m_thd_run;
  bool m_exit_of_run;

  std::unique_ptr<tlu::FmctluController> m_tlu;
  uint64_t m_lasttime;
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::Producer>::
    Register<FmctluProducer, const std::string&, const std::string&>(FmctluProducer::m_id_factory);
}


FmctluProducer::FmctluProducer(const std::string name, const std::string &runcontrol)
  :eudaq::Producer(name, runcontrol){

}

void FmctluProducer::MainLoop(){
  bool isbegin = true;

  m_tlu->ResetCounters();
  m_tlu->ResetEventsBuffer();
  m_tlu->ResetFIFO();
  m_tlu->SetTriggerVeto(0);
  //while(true) {
  while(!m_exit_of_run) {
    m_lasttime=m_tlu->GetCurrentTimestamp()*25;
    m_tlu->ReceiveEvents();
    while (!m_tlu->IsBufferEmpty()){
      tlu::fmctludata *data = m_tlu->PopFrontEvent();
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
        ev->SetTag("SCALER4", std::to_string(sl4));
        ev->SetTag("SCALER5", std::to_string(sl5));
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

void FmctluProducer::DoInitialise(){
  /* Establish a connection with the TLU using IPBus.
     Define the main hardware parameters.
  */
  auto ini = GetInitConfiguration();
  std::cout << "INITIALIZE ID: " << ini->Get("initid", 0) << std::endl;
  std::string uhal_conn = "file://./FMCTLU_connections.xml";
  std::string uhal_node = "fmctlu.udp";
  uhal_conn = ini->Get("ConnectionFile", uhal_conn);
  uhal_node = ini->Get("DeviceName",uhal_node);
  m_tlu = std::unique_ptr<tlu::FmctluController>(new tlu::FmctluController(uhal_conn, uhal_node));

  // Define constants
  m_tlu->DefineConst(ini->Get("nDUTs", 4), ini->Get("nTrgIn", 6));

  // Reset IPBus registers
  m_tlu->ResetSerdes();
  m_tlu->ResetCounters();
  m_tlu->SetTriggerVeto(1);
  m_tlu->ResetFIFO();
  m_tlu->ResetEventsBuffer();

  //Import I2C addresses for hardware
  //Populate address list for I2C elements
  m_tlu->SetI2C_core_addr(ini->Get("I2C_COREEXP_Addr", 0x21));
  m_tlu->SetI2C_clockChip_addr(ini->Get("I2C_CLK_Addr", 0x68));
  m_tlu->SetI2C_DAC1_addr(ini->Get("I2C_DAC1_Addr",0x13) );
  m_tlu->SetI2C_DAC2_addr(ini->Get("I2C_DAC2_Addr",0x1f) );
  m_tlu->SetI2C_EEPROM_addr(ini->Get("I2C_ID_Addr", 0x50) );
  m_tlu->SetI2C_expander1_addr(ini->Get("I2C_EXP1_Addr",0x74));
  m_tlu->SetI2C_expander2_addr(ini->Get("I2C_EXP2_Addr",0x75) );

  // Initialize TLU hardware
  m_tlu->InitializeI2C();
  m_tlu->InitializeIOexp();
  if (ini->Get("intRefOn", false)){
    m_tlu->InitializeDAC(ini->Get("intRefOn", false), ini->Get("VRefInt", 2.5));
  }
  else{
    m_tlu->InitializeDAC(ini->Get("intRefOn", false), ini->Get("VRefExt", 1.3));
  }

  m_tlu->ResetTimestamp();

}

void FmctluProducer::DoConfigure() {
  auto conf = GetConfiguration();
  std::cout << "CONFIG ID: " << conf->Get("confid", 0) << std::endl;

  m_tlu->SetTriggerVeto(1);

  if (conf->Get("CONFCLOCK", true)){
    m_tlu->InitializeClkChip(conf->Get("CLOCK_CFG_FILE","./../user/eudet/misc/fmctlu_clock_config.txt")  );
  }

  // Enable HDMI connectors
  m_tlu->configureHDMI(1, conf->Get("HDMI1_set", 0b0001), false);
  m_tlu->configureHDMI(2, conf->Get("HDMI2_set", 0b0001), false);
  m_tlu->configureHDMI(3, conf->Get("HDMI3_set", 0b0001), false);
  m_tlu->configureHDMI(4, conf->Get("HDMI4_set", 0b0001), false);

  // Select clock to HDMI
  m_tlu->SetDutClkSrc(1, conf->Get("HDMI1_clk", 1), false);
  m_tlu->SetDutClkSrc(2, conf->Get("HDMI2_clk", 1), false);
  m_tlu->SetDutClkSrc(3, conf->Get("HDMI3_clk", 1), false);
  m_tlu->SetDutClkSrc(4, conf->Get("HDMI4_clk", 1), false);

  //Set lemo clock
  m_tlu->enableClkLEMO(conf->Get("LEMOclk", true), true);

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

void FmctluProducer::DoStartRun(){
  m_exit_of_run = false;
  std::cout << "TLU START command received" << std::endl;
  m_thd_run = std::thread(&FmctluProducer::MainLoop, this);
}

void FmctluProducer::DoStopRun(){
  m_exit_of_run = true;
  std::cout << "TLU STOP command received" << std::endl;
  if(m_thd_run.joinable())
    m_thd_run.join();
}

void FmctluProducer::DoTerminate(){
  m_exit_of_run = true;
  std::cout << "TLU TERMINATE command received" << std::endl;
  if(m_thd_run.joinable())
    m_thd_run.join();
}

void FmctluProducer::DoReset(){
  m_exit_of_run = true;
  std::cout << "TLU RESET command received" << std::endl;
  if(m_thd_run.joinable())
    m_thd_run.join();

  m_tlu.reset();
}

void FmctluProducer::OnStatus() {
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
