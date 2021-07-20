#include "eudaq/Producer.hh"

#include "AidaTluController.hh"
#include "AidaTluHardware.hh"
#include "AidaTluPowerModule.hh"

#include <iostream>
#include <ostream>
#include <vector>
#include <chrono>
#include <thread>


class AidaTluProducer: public eudaq::Producer {
public:
  AidaTluProducer(const std::string name, const std::string &runcontrol);
  void DoConfigure() override;
  void DoInitialise() override;
  void DoStartRun() override;
  void DoStopRun() override;
  void DoTerminate() override;
  void DoReset() override;
  void DoStatus() override;
  void RunLoop() override;

  static const uint32_t m_id_factory = eudaq::cstr2hash("AidaTluProducer");
private:
  bool m_exit_of_run;
  std::mutex m_mtx_tlu; //prevent to reset tlu during the RunLoop thread

  std::unique_ptr<tlu::AidaTluController> m_tlu;
  uint64_t m_starttime;
  uint64_t m_lasttime;
  double m_duration;

  uint8_t m_verbose;
  uint32_t m_delayStart;
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::Producer>::
    Register<AidaTluProducer, const std::string&, const std::string&>(AidaTluProducer::m_id_factory);
}


AidaTluProducer::AidaTluProducer(const std::string name, const std::string &runcontrol)
  :eudaq::Producer(name, runcontrol){
  m_duration = 0;
  m_starttime = 0;
  m_lasttime = 0;
}

void AidaTluProducer::RunLoop(){
  std::unique_lock<std::mutex> lk(m_mtx_tlu);
  bool isbegin = true;
  m_tlu->ResetCounters();
  m_tlu->ResetEventsBuffer();
  m_tlu->ResetFIFO();

  // Pause the TLU to allow slow devices to get ready after the euRunControl has
  // issued the DoStart() command
  std::this_thread::sleep_for( std::chrono::milliseconds( m_delayStart ) );

  // Send reset pulse to all DUTs and reset internal counters
  m_tlu->SetRunActive(1, 1);

  // Enable triggers
  m_tlu->SetTriggerVeto(0, m_verbose);

  while(!m_exit_of_run) {
    m_lasttime=m_tlu->GetCurrentTimestamp()*25;
    if(isbegin) m_starttime = m_lasttime;
    m_tlu->ReceiveEvents(m_verbose);
    while (!m_tlu->IsBufferEmpty()){
      tlu::fmctludata *data = m_tlu->PopFrontEvent();
      uint32_t trigger_n = data->eventnumber;
      uint64_t ts_raw = data->timestamp;
      uint64_t ts_ns = ts_raw*25;
      auto ev = eudaq::Event::MakeUnique("TluRawDataEvent");
      ev->SetTimestamp(ts_ns, ts_ns+25, false);
      ev->SetTriggerN(trigger_n);

      std::stringstream triggerss;
      //triggerss<< data->input5 << data->input4 << data->input3 << data->input2 << data->input1 << data->input0;
      triggerss<< std::to_string(data->input5) << std::to_string(data->input4) << std::to_string(data->input3) << std::to_string(data->input2) << std::to_string(data->input1) << std::to_string(data->input0);
      ev->SetTag("TRIGGER", triggerss.str());
      ev->SetTag("FINE_TS0", std::to_string(data->sc0));
      ev->SetTag("FINE_TS1", std::to_string(data->sc1));
      ev->SetTag("FINE_TS2", std::to_string(data->sc2));
      ev->SetTag("FINE_TS3", std::to_string(data->sc3));
      ev->SetTag("FINE_TS4", std::to_string(data->sc4));
      ev->SetTag("FINE_TS5", std::to_string(data->sc5));
      ev->SetTag("TYPE", std::to_string(data->eventtype));

      if(m_tlu->IsBufferEmpty()){
      	uint32_t sl0,sl1,sl2,sl3, sl4, sl5, pt;
      	m_tlu->GetScaler(sl0,sl1,sl2,sl3,sl4,sl5);
      	pt=m_tlu->GetPreVetoTriggers();
        ev->SetTag("PARTICLES", std::to_string(pt));
      	ev->SetTag("SCALER0", std::to_string(sl0));
      	ev->SetTag("SCALER1", std::to_string(sl1));
      	ev->SetTag("SCALER2", std::to_string(sl2));
      	ev->SetTag("SCALER3", std::to_string(sl3));
        ev->SetTag("SCALER4", std::to_string(sl4));
        ev->SetTag("SCALER5", std::to_string(sl5));
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
  m_tlu->SetTriggerVeto(1, m_verbose);
  // Set TLU internal logic to stop.
  m_tlu->SetRunActive(0, 1);
}

void AidaTluProducer::DoInitialise(){
  /* Establish a connection with the TLU using IPBus.
     Define the main hardware parameters.
  */
  auto ini = GetInitConfiguration();
  //std::cout << "  INITIALIZE ID: " << ini->Get("initid", 0) << std::endl;
  EUDAQ_INFO("TLU INITIALIZE ID: " + std::to_string(ini->Get("initid", 0)));
  //std::string uhal_conn = "file://./../user/eudet/misc/hw_conf/aida_tlu/fmctlu_connection.xml";
  //std::string uhal_node = "fmctlu.udp";
  std::string uhal_conn;
  std::string uhal_node;
  uhal_conn = ini->Get("ConnectionFile", uhal_conn);
  uhal_node = ini->Get("DeviceName",uhal_node);
  m_tlu = std::unique_ptr<tlu::AidaTluController>(new tlu::AidaTluController(uhal_conn, uhal_node));

  if( ini->Get("skipini", false) ){
    EUDAQ_INFO("TLU SKIPPING INITIALIZATION (skipini = 1)");
  }
  else{

    m_verbose = abs(ini->Get("verbose", 0));
    EUDAQ_INFO("TLU VERBOSITY SET TO: " + std::to_string(m_verbose));

    // Define constants
    m_tlu->DefineConst(ini->Get("nDUTs", 4), ini->Get("nTrgIn", 6));

    // Import I2C addresses for hardware
    // Populate address list for I2C elements
    m_tlu->SetI2C_core_addr(ini->Get("I2C_COREEXP_Addr", 0x21));
    m_tlu->SetI2C_clockChip_addr(ini->Get("I2C_CLK_Addr", 0x68));
    m_tlu->SetI2C_DAC1_addr(ini->Get("I2C_DAC1_Addr",0x13) );
    m_tlu->SetI2C_DAC2_addr(ini->Get("I2C_DAC2_Addr",0x1f) );
    m_tlu->SetI2C_EEPROM_addr(ini->Get("I2C_ID_Addr", 0x50) );
    m_tlu->SetI2C_expander1_addr(ini->Get("I2C_EXP1_Addr",0x74));
    m_tlu->SetI2C_expander2_addr(ini->Get("I2C_EXP2_Addr",0x75) );
    m_tlu->SetI2C_pwrmdl_addr(ini->Get("I2C_DACModule_Addr",  0x1C), ini->Get("I2C_EXP1Module_Addr",  0x76), ini->Get("I2C_EXP2Module_Addr",  0x77), ini->Get("I2C_pwrId_Addr",  0x51));
    m_tlu->SetI2C_disp_addr(ini->Get("I2C_disp_Addr",0x3A));

    // Initialize TLU hardware
    m_tlu->InitializeI2C(m_verbose);
    m_tlu->InitializeIOexp(m_verbose);
    if (ini->Get("intRefOn", false)){
      m_tlu->InitializeDAC(ini->Get("intRefOn", false), ini->Get("VRefInt", 2.5), m_verbose);
    }
    else{
      m_tlu->InitializeDAC(ini->Get("intRefOn", false), ini->Get("VRefExt", 1.3), m_verbose);
    }

    // Initialize the Si5345 clock chip using pre-generated file
    if (ini->Get("CONFCLOCK", true)){
      std::string  clkConfFile;
      std::string defaultCfgFile= "./../user/eudet/misc/hw_conf/aida_tlu/fmctlu_clock_config.txt";
      clkConfFile= ini->Get("CLOCK_CFG_FILE", defaultCfgFile);
      if (clkConfFile== defaultCfgFile){
        EUDAQ_WARN("TLU: Could not find the parameter for clock configuration in the INI file. Using the default.");
      }
      int clkres;
      clkres= m_tlu->InitializeClkChip( clkConfFile, m_verbose  );
      if (clkres == -1){
        EUDAQ_ERROR("TLU: clock configuration failed.");
      }
    }

    // Reset IPBus registers
    m_tlu->ResetSerdes();
    m_tlu->ResetCounters();
    m_tlu->SetTriggerVeto(1, m_verbose);
    m_tlu->ResetFIFO();
    m_tlu->ResetEventsBuffer();

    m_tlu->ResetTimestamp();

  }
}

void AidaTluProducer::DoConfigure() {

  auto conf = GetConfiguration();

  EUDAQ_INFO("CONFIG ID: " + std::to_string(conf->Get("confid", 0)));
  m_verbose = abs(conf->Get("verbose", 0));
  EUDAQ_INFO("TLU VERBOSITY SET TO: " + std::to_string(m_verbose));
  m_delayStart = conf->Get("delayStart", 0);
  EUDAQ_INFO("TLU DELAY START SET TO: " + std::to_string(m_delayStart) + " ms");

  m_tlu->SetTriggerVeto(1, m_verbose);
  if( conf->Get("skipconf", false) ){
    EUDAQ_INFO("TLU SKIPPING CONFIGURATION (skipconf = 1)");
  }
  else{
    // Enable HDMI connectors
    if(m_verbose > 0) EUDAQ_INFO(" -DUT CONFIGURATION");
    m_tlu->configureHDMI(1, conf->Get("HDMI1_set", 0b0001), m_verbose);
    m_tlu->configureHDMI(2, conf->Get("HDMI2_set", 0b0001), m_verbose);
    m_tlu->configureHDMI(3, conf->Get("HDMI3_set", 0b0001), m_verbose);
    m_tlu->configureHDMI(4, conf->Get("HDMI4_set", 0b0001), m_verbose);

    // Select clock to HDMI
    m_tlu->SetDutClkSrc(1, conf->Get("HDMI1_clk", 1), m_verbose);
    m_tlu->SetDutClkSrc(2, conf->Get("HDMI2_clk", 1), m_verbose);
    m_tlu->SetDutClkSrc(3, conf->Get("HDMI3_clk", 1), m_verbose);
    m_tlu->SetDutClkSrc(4, conf->Get("HDMI4_clk", 1), m_verbose);

    //Set lemo clock
    if(m_verbose > 0) EUDAQ_INFO(" -CLOCK OUTPUT CONFIGURATION");
    m_tlu->enableClkLEMO(conf->Get("LEMOclk", true), m_verbose);

    // Set thresholds
    if(m_verbose > 0) EUDAQ_INFO(" -DISCRIMINATOR THRESHOLDS CONFIGURATION");
    m_tlu->SetThresholdValue(0, conf->Get("DACThreshold0", 1.2), m_verbose);
    m_tlu->SetThresholdValue(1, conf->Get("DACThreshold1", 1.2), m_verbose);
    m_tlu->SetThresholdValue(2, conf->Get("DACThreshold2", 1.2), m_verbose);
    m_tlu->SetThresholdValue(3, conf->Get("DACThreshold3", 1.2), m_verbose);
    m_tlu->SetThresholdValue(4, conf->Get("DACThreshold4", 1.2), m_verbose);
    m_tlu->SetThresholdValue(5, conf->Get("DACThreshold5", 1.2), m_verbose);

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

    if(m_verbose > 0) EUDAQ_INFO(" -ADJUST STRETCH AND DELAY");
    m_tlu->SetPulseStretchPack(stretcVec, m_verbose);
    m_tlu->SetPulseDelayPack(delayVec, m_verbose);

    // Set triggerMask
    // The conf function does not seem happy with a 32-bit default. Need to check.
    if(m_verbose > 0) EUDAQ_INFO(" -DEFINE TRIGGER MASK");
    m_tlu->SetTriggerMask( (uint32_t)(conf->Get("trigMaskHi", 0xFFFF)),  (uint32_t)(conf->Get("trigMaskLo", 0xFFFE)) );

    // Set triggerPolarity
    if(m_verbose > 0) EUDAQ_INFO(" -DEFINE TRIGGER POLARITY");
    m_tlu->SetTriggerPolarity( (uint64_t)(conf->Get("trigPol", 0x003F)));

    // Set PMT power
    if(m_verbose > 0) EUDAQ_INFO(" -PMT OUTPUT VOLTAGES");
    m_tlu->pwrled_setVoltages(conf->Get("PMT1_V", 0.0), conf->Get("PMT2_V", 0.0), conf->Get("PMT3_V", 0.0), conf->Get("PMT4_V", 0.0), m_verbose);

    if(m_verbose > 0) EUDAQ_INFO(" -DUT OPERATION MODE");
    m_tlu->SetDUTMask( (int32_t)(conf->Get("DUTMask",1)), m_verbose); // Which DUTs are on
    m_tlu->SetDUTMaskMode( (int32_t)(conf->Get("DUTMaskMode",0xff)), m_verbose); // AIDA (x1) or EUDET (x0)
    m_tlu->SetDUTMaskModeModifier( (int32_t)(conf->Get("DUTMaskModeModifier",0xff)), m_verbose); // Only for EUDET
    m_tlu->SetDUTIgnoreBusy( (int32_t)(conf->Get("DUTIgnoreBusy",0xF)), m_verbose); // Ignore busy in AIDA mode
    m_tlu->SetDUTIgnoreShutterVeto( (int32_t)(conf->Get("DUTIgnoreShutterVeto",1)), m_verbose); //

    if(m_verbose > 0) EUDAQ_INFO(" -SHUTTER OPERATION MODE");
    m_tlu->SetShutterParameters( (bool)conf->Get("EnableShutterMode",0),
                                 (int8_t)(conf->Get("ShutterSource",0)),
                                 (int32_t)(conf->Get("ShutterOnTime",0)),
                                 (int32_t)(conf->Get("ShutterOffTime",0)),
                                 (int32_t)(conf->Get("ShutterVetoOffTime",0)),
                                 (int32_t)(conf->Get("InternalShutterInterval",0)),
                                  m_verbose);


    if(m_verbose > 0) EUDAQ_INFO(" -AUTO TRIGGER SETTINGS");
    m_tlu->SetInternalTriggerFrequency( (int32_t)( conf->Get("InternalTriggerFreq", 0)), m_verbose );

    if(m_verbose > 0) EUDAQ_INFO(" -FINALIZING TLU CONFIGURATION");
    m_tlu->SetEnableRecordData( (uint32_t)(conf->Get("EnableRecordData", 1)) );
    m_tlu->GetEventFifoCSR();
    m_tlu->GetEventFifoFillLevel();
  }
}

void AidaTluProducer::DoStartRun(){
  m_exit_of_run = false;
  EUDAQ_INFO("TLU START command received");
}

void AidaTluProducer::DoStopRun(){
  m_exit_of_run = true;
  EUDAQ_INFO("TLU STOP command received");
}

void AidaTluProducer::DoTerminate(){
  m_exit_of_run = true;
  EUDAQ_INFO("TLU TERMINATE command received");
}

void AidaTluProducer::DoReset(){
  m_exit_of_run = true;
  EUDAQ_INFO("TLU RESET command received");
  std::unique_lock<std::mutex> lk(m_mtx_tlu); //waiting for the runloop's return
  m_tlu.reset();
}

void AidaTluProducer::DoStatus() {
  if (m_tlu) {
    m_duration = double(m_lasttime - m_starttime) / 1000000000; // in seconds
    uint32_t sl0, sl1, sl2, sl3, sl4, sl5, pret, post;
    pret = m_tlu->GetPreVetoTriggers();
    post = m_tlu->GetPostVetoTriggers();
    m_tlu->GetScaler(sl0, sl1, sl2, sl3, sl4, sl5);
    //Is tlu controller safe to be accessed by 2 threads (RunLoop and DoStatus) at some time?
    SetStatusTag("IDTrig", std::to_string(post));
    SetStatusTag("Freq. (avg.) [kHz]", std::to_string(post/m_duration/1000));
    SetStatusTag("Run duration [s]", std::to_string(m_duration));
    SetStatusTag("Particles", std::to_string(pret));
    SetStatusTag("Scaler", std::to_string(sl0) + ":" + std::to_string(sl1) + ":" + std::to_string(sl2) + ":" + std::to_string(sl3) + ":" + std::to_string(sl4) + ":" + std::to_string(sl5));
  }
}
