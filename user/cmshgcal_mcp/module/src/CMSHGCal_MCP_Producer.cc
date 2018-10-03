#include "eudaq/Configuration.hh"
#include "eudaq/Producer.hh"
#include "eudaq/Logger.hh"
#include "eudaq/RawEvent.hh"
#include <iostream>
#include <ostream>
#include <cstdlib>
#include <string>
#include <vector>

#include "CAEN_v17XX.h"

#include <chrono>

enum RUNMODE {
  MCP_DEBUG = 0,
  MCP_RUN
};

static const std::string EVENT_TYPE = "CMSHGCal_MCP_RawEvent";

class CMSHGCal_MCP_Producer : public eudaq::Producer {
public:
  CMSHGCal_MCP_Producer(const std::string & name, const std::string & runcontrol);
  void DoInitialise() override;
  void DoConfigure() override;
  void DoStartRun() override;
  void DoStopRun() override;
  void DoTerminate() override;
  void DoReset() override;
  void RunLoop() override;

  static const uint32_t m_id_factory = eudaq::cstr2hash("CMSHGCal_MCP_Producer");
private:
  //parameters for readout control
  unsigned m_run, m_ev;
  bool stopping, done;
  bool initialized, connection_initialized;

  //Digitizer communication
  CAEN_V1742* CAENV1742_instance;
  std::vector<WORD> dataStream;

  std::chrono::steady_clock::time_point startTime;
  uint64_t timeSinceStart;

  //set at configuration
  int m_readoutSleep;   //sleep in the readout loop in microseconds

  //set on initialisation
  RUNMODE _mode;

};


namespace {
auto dummy0 = eudaq::Factory<eudaq::Producer>::
              Register<CMSHGCal_MCP_Producer, const std::string&, const std::string&>(CMSHGCal_MCP_Producer::m_id_factory);
}


CMSHGCal_MCP_Producer::CMSHGCal_MCP_Producer(const std::string & name, const std::string & runcontrol)
  : eudaq::Producer(name, runcontrol), m_run(0), m_ev(0), stopping(false), done(false) {
  initialized = false;
  _mode = MCP_DEBUG;
  m_readoutSleep = 1000;  //1 ms for sleepin in readout cycle as default
  CAENV1742_instance = NULL;
}


void CMSHGCal_MCP_Producer::DoInitialise() {
  auto ini = GetInitConfiguration();

  if (CAENV1742_instance != NULL) return;
  std::cout << "Initialisation of the MCP Producer..." << std::endl;

    std::cout << "Reading: " << ini->Name() << std::endl;

    int mode = ini->Get("AcquisitionMode", 0);
    switch ( mode ) {
    case 0 :
      _mode = MCP_DEBUG;
      break;
    case 1:
    default :
      _mode = MCP_RUN;
      break;
    }

    std::cout << "Initialisation of the MCP Producer..." << std::endl;
    CAENV1742_instance = new CAEN_V1742(0);
    CAENV1742_instance->Init();

    SetStatus(eudaq::Status::STATE_UNCONF, "Initialised (" + ini->Name() + ")");
  

}


void CMSHGCal_MCP_Producer::DoConfigure() {
  auto conf = GetConfiguration();
  SetStatus(eudaq::Status::STATE_UNCONF, "Configuring (" + conf->Name() + ")");
  CAENV1742_instance->setDefaults();
  std::cout << "Configuring: " << conf->Name() << std::endl;
  conf->Print(std::cout);


  m_readoutSleep = conf->Get("readoutSleep", 1000);

  if (_mode == MCP_RUN) {
    //read configuration specific to the board
    CAEN_V1742::CAEN_V1742_Config_t _config;

    // addressing configurations
    _config.BaseAddress = conf->Get("BASEADDRESS", 0x33330000);
    std::string link_type_str = conf->Get("LINK_TYPE", "OpticalLink");
    if (link_type_str == "OpticalLink") _config.LinkType = CAEN_DGTZ_PCI_OpticalLink;
    else if (link_type_str == "USB") _config.LinkType = CAEN_DGTZ_USB;
    else std::cout << "Unknown link type: " << link_type_str << std::endl;
    _config.LinkNum = conf->Get("LINK_NUM", 0);
    _config.ConetNode = conf->Get("CONET_NODE", 0);

    std::string useCorrections_string = conf->Get("CORRECTION_LEVEL", "AUTO");
    if (useCorrections_string == "AUTO") _config.useCorrections = -1 ;
    else _config.useCorrections = atoi (useCorrections_string.c_str ()) ;

    // RECORD_LENGTH = number of samples in the acquisition window
    // For the models 742 the options available are only 1024, 520, 256 and 136
    _config.RecordLength = conf->Get("RECORD_LENGTH", 1024);

    // TEST_PATTERN: if enabled, data from ADC are replaced by test pattern (triangular wave)
    // options: YES, NO
    if (conf->Get("TEST_PATTERN", "NO") == "YES") _config.TestPattern = 1 ;
    else _config.TestPattern = 0 ;

    // DRS4_Frequency.  Values: 0 = 5 GHz, 1 = 2.5 GHz,  2 = 1 GHz
    _config.DRS4Frequency = (CAEN_DGTZ_DRS4Frequency_t)conf->Get("DRS4_FREQUENCY", 0);

    // EXTERNAL_TRIGGER: external trigger input settings. When enabled, the ext. trg. can be either
    // propagated (ACQUISITION_AND_TRGOUT) or not (ACQUISITION_ONLY) through the TRGOUT
    // options: DISABLED, ACQUISITION_ONLY, ACQUISITION_AND_TRGOUT
    std::string externalTrigger_string = conf->Get("EXTERNAL_TRIGGER", "DISABLED");
    if (externalTrigger_string == "ACQUISITION_ONLY")
      _config.ExtTriggerMode = CAEN_DGTZ_TRGMODE_ACQ_ONLY ;
    else if (externalTrigger_string == "ACQUISITION_AND_TRGOUT")
      _config.ExtTriggerMode = CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT ;
    else if (externalTrigger_string == "DISABLED")
      _config.ExtTriggerMode = CAEN_DGTZ_TRGMODE_DISABLED ;
    else {
      std::cout << "[CAEN_V1742]::[WARNING]:: EXTERNAL_TRIGGER " << externalTrigger_string << " is an invalid option" << std::endl;
      _config.ExtTriggerMode = CAEN_DGTZ_TRGMODE_DISABLED ;
    }

    //FAST_TRIGGER: fast trigger input settings. ONLY FOR 742 MODELS. When enabled, the fast trigger is used for the data acquisition
    //options: DISABLED, ACQUISITION_ONLY
    std::string fastTrigger_string = conf->Get("FAST_TRIGGER", "ACQUISITION_ONLY");
    if (fastTrigger_string == "ACQUISITION_ONLY")
      _config.FastTriggerMode = CAEN_DGTZ_TRGMODE_ACQ_ONLY ;
    else if (fastTrigger_string == "DISABLED")
      _config.FastTriggerMode = CAEN_DGTZ_TRGMODE_DISABLED ;
    else {
      std::cout << "[CAEN_V1742]::[WARNING]:: FAST_TRIGGER " << fastTrigger_string << " is an invalid option" << std::endl;
      _config.FastTriggerMode = CAEN_DGTZ_TRGMODE_DISABLED ;
    }

    // ENABLED_FAST_TRIGGER_DIGITIZING: ONLY FOR 742 MODELS. If enabled the fast trigger signal is digitized and it is present in data readout as channel n.8 for each group.
    // options: YES, NO
    if (conf->Get("ENABLED_FAST_TRIGGER_DIGITIZING", "NO") == "YES") _config.FastTriggerEnabled = 1 ;
    else _config.FastTriggerEnabled = 0 ;

    // MAX_NUM_EVENTS_BLT: maximum number of events to read out in one Block Transfer.
    // options: 1 to 1023
    _config.NumEvents = conf->Get("MAX_NUM_EVENTS_BLT", 1);

    // POST_TRIGGER: post trigger size in percent of the whole acquisition window
    // options: 0 to 100
    // On models 742 there is a delay of about 35nsec on signal Fast Trigger TR; the post trigger is added to this delay
    _config.PostTrigger = conf->Get("POST_TRIGGER", 15);

    // TRIGGER_EDGE: decides whether the trigger occurs on the rising or falling edge of the signal
    // options: RISING, FALLING
    std::string triggerEdge_string = conf->Get("TRIGGER_EDGE", "FALLING");
    if (triggerEdge_string == "FALLING")
      _config.TriggerEdge = 1 ;
    else if (triggerEdge_string == "RISING")
      _config.TriggerEdge = 0 ;
    else {
      std::cout << "[CAEN_V1742]::[WARNING]:: TRIGGER_EDGE " << triggerEdge_string << " is an invalid option" << std::endl;
      _config.TriggerEdge = 1 ;
    }

    // USE_INTERRUPT: number of events that must be ready for the readout when the IRQ is asserted.
    // Zero means that the interrupts are not used (readout runs continuously)
    _config.InterruptNumEvents = conf->Get("USE_INTERRUPT", 0);

    // FPIO_LEVEL: type of the front panel I/O LEMO connectors
    // options: NIM, TTL
    std::string fpiolevel_string = conf->Get("FPIO_LEVEL", "NIM");
    if (fpiolevel_string == "NIM")
      _config.FPIOtype = 0 ;
    else if (fpiolevel_string == "TTL")
      _config.FPIOtype = 1 ;
    else {
      std::cout << "[CAEN_V1742]::[WARNING]:: FPIO_LEVEL " << fpiolevel_string << " is an invalid option" << std::endl;
      _config.FPIOtype = 0 ;
    }

    // Enable Mask: enable/disable one channel (or one group in the case of the Mod 740 and Mod 742)
    _config.ChannelEnableMask = conf->Get("CHANNEL_ENABLE_MASK", 0x3);
    _config.GroupEnableMask = conf->Get("GROUP_ENABLE_MASK", 0x1);

    // CHANNEL_TRIGGER: channel auto trigger settings. When enabled, the ch. auto trg. can be either
    // propagated (ACQUISITION_AND_TRGOUT) or not (ACQUISITION_ONLY) through the TRGOUT
    // options: DISABLED, ACQUISITION_ONLY, ACQUISITION_AND_TRGOUT
    std::string channelTrigger_string = conf->Get("CHANNELS_TRIGGER", "DISABLED");
    CAEN_DGTZ_TriggerMode_t tm ;
    if (channelTrigger_string == "ACQUISITION_ONLY")
      tm = CAEN_DGTZ_TRGMODE_ACQ_ONLY ;
    else if (channelTrigger_string == "ACQUISITION_AND_TRGOUT")
      tm = CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT ;
    else if (channelTrigger_string == "DISABLED")
      tm = CAEN_DGTZ_TRGMODE_DISABLED ;
    else {
      std::cout << "[CAEN_V1742]::[WARNING]:: CHANNELS_TRIGGER " << channelTrigger_string << " is an invalid option" << std::endl;
      tm = CAEN_DGTZ_TRGMODE_DISABLED ;
    }
    for (int i = 0 ; i < CAEN_V1742_MAXSET ; ++i) _config.ChannelTriggerMode[i] = tm;

    // DC_OFFSET: DC offset adjust (DAC channel setting) in percent of the Full Scale.
    // For model 740 and 742* the DC offset adjust is the same for all channel in the group
    // -50: analog input dynamic range = -Vpp to 0 (negative signals)*
    // +50: analog input dynamic range = 0 to +Vpp (positive signals)*
    // 0:   analog input dynamic range = -Vpp/2 to +Vpp/2 (bipolar signals)*
    // options: -50.0 to 50.0  (floating point)
    // *NOTE: Ranges are different for 742 Model.....see GRP_CH_DC_OFFSET description
    uint32_t val = (uint32_t) ( (conf->Get("DC_OFFSET_CHANNELS", 0) + 50) * 65535 / 100) ;
    for (int i = 0 ; i < CAEN_V1742_MAXSET ; ++i) _config.DCoffset[i] = val ;

    //same as DC_OFFSET_CHANNELS but for the trigger, scale is absolute and not relative
    val = conf->Get("DC_OFFSET_TRIGGERS", 0);
    for (int i = 0 ; i < CAEN_V1742_MAXSET ; ++i) _config.FTDCoffset[i] = val ;

    // GROUP_TRG_ENABLE_MASK: this option is used only for the Models x740. These models have the
    // channels grouped 8 by 8; one group of 8 channels has a common trigger that is generated as
    // the OR of the self trigger of the channels in the group that are enabled by this mask.
    // options: 0 to FF
    val = conf->Get("GROUP_TRG_ENABLE_MASK", 0xFF);
    for (int i = 0 ; i < CAEN_V1742_MAXSET ; ++i) _config.GroupTrgEnableMask[i] = val  & 0xFF;

    // TRIGGER_THRESHOLD: threshold for the channel auto trigger (ADC counts)
    // options 0 to 2^N-1 (N=Number of bit of the ADC)
    val = conf->Get("TRIGGER_THRESHOLD_CHANNELS", 100);
    for (int i = 0 ; i < CAEN_V1742_MAXSET ; ++i) _config.Threshold[i] = val;

    val = conf->Get("TRIGGER_THRESHOLD_TRIGGERS", 32768);
    for (int i = 0 ; i < CAEN_V1742_MAXSET ; ++i) _config.FTThreshold[i] = val;

    // GRP_CH_DC_OFFSET dc_0, dc_1, dc_2, dc_3, dc_4, dc_5, dc_6, dc_7
    // Available only for model 742, allows to set different DC offset adjust for each channel (DAC channel setting) in percent of the Full Scale.
    // -50: analog input dynamic range = -3Vpp/2 to -Vpp/2 (max negative dynamic)
    // +50: analog input dynamic range = +Vpp/2 to +3Vpp/2 (max positive dynamic)
    // 0: analog input dynamic range = -Vpp/2 to +Vpp/2 (bipolar signals)
    // options: -50.0 to 50.0  (floating point)
    std::stringstream ss("GRP_CH_DC_OFFSET");
    for (int i = 0 ; i < CAEN_V1742_MAXSET ; ++i)  {
      for (int k = 0; k < CAEN_V1742_MAXCH; ++k) {
        ss.str(""); ss << "GRP_CH_DC_OFFSET_" << i << "_" << k;
        int dc_val = (int) ( (conf->Get(ss.str().c_str(), 0) + 50) * 65535 / 100) ;
        _config.DCoffsetGrpCh[i][k] = dc_val ;
      }
    }
    CAENV1742_instance->Config(_config);
    //actual application of the configuration of the physical digitizer only in the MCP_RUN mode
    if (!initialized) initialized = (CAENV1742_instance->Init()==1); //will be executed only once after the intitial configuration
    if (!initialized) {
  std::cout << "Digitizer could not be initialised..."<<std::endl;
  return;}
    CAENV1742_instance->SetupModule();
    CAENV1742_instance->Print(0);
  } else {
    CAENV1742_instance->setDefaults();
  initialized = true;
  } 

  
  


  SetStatus(eudaq::Status::STATE_CONF, "Configured (" + conf->Name() + ")");
}

void CMSHGCal_MCP_Producer::DoStartRun() {
  m_run = GetRunNumber();
  m_ev = 0;

  EUDAQ_INFO("Start Run: " + m_run);

  if (_mode == MCP_RUN) {
    if (initialized)
      CAENV1742_instance->StartAcquisition();
    else {
      EUDAQ_INFO("ATTENTION !!! Communication to the digitizer has not been established");
      SetStatus(eudaq::Status::STATE_ERROR, "Communication to the TDC has not been established");
    }
  }

  SetStatus(eudaq::Status::STATE_RUNNING, "Running");
  startTime = std::chrono::steady_clock::now();
  stopping = false;
}

void CMSHGCal_MCP_Producer::DoStopRun() {
  SetStatus(eudaq::Status::STATE_CONF, "Stopping");
  EUDAQ_INFO("Stopping Run: " + m_run);
  if (_mode == MCP_RUN) {
    if (initialized)
      CAENV1742_instance->StopAcquisition();
    else {
      EUDAQ_INFO("ATTENTION !!! Communication to the digitizer has not been established");
      SetStatus(eudaq::Status::STATE_ERROR, "Communication to the digitizer has not been established");
    }
  }
  std::cout << "[RUN " << m_run << "] Number of events read: " << m_ev << std::endl;
  stopping = true;
}

void CMSHGCal_MCP_Producer::DoReset() {
  if (_mode == MCP_RUN) {
    if (initialized)
      CAENV1742_instance->Clear();
    else {
      EUDAQ_INFO("ATTENTION !!! Communication to the digitizer has not been established");
      SetStatus(eudaq::Status::STATE_ERROR, "Communication to the digitizer has not been established");
    }
  }
}

void CMSHGCal_MCP_Producer::DoTerminate() {
  EUDAQ_INFO("Terminating...");
  done = true;
  eudaq::mSleep(200);
}

void CMSHGCal_MCP_Producer::RunLoop() {
  if (!initialized) return;
  while (!done) {
    usleep(m_readoutSleep);

    if (_mode == MCP_RUN) {
      if (CAENV1742_instance->DataReady() != 1000) {
        if (stopping) return;
        else continue;   //successful readout <--> error code is zero = NONE
      }
      if (CAENV1742_instance->Read(dataStream) != 0) {
        if (stopping) return;
        else continue;   //successful readout <--> error code is zero = NONE
      }
    } else {
      if (stopping) {stopping = false; SetStatus(eudaq::Status::STATE_CONF, "Stopped"); return;}
      CAENV1742_instance->generateFakeData(m_ev + 1, dataStream);
    }

    m_ev++;
    //get the timestamp since start:
    timeSinceStart = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - startTime).count();
    if (!(m_ev % 1000)) std::cout <<  "[EVENT " << m_ev << "]  " << timeSinceStart / 1000. << " ms" << std::endl;

    //making an EUDAQ event
    eudaq::EventUP ev = eudaq::Event::MakeUnique(EVENT_TYPE);
    ev->SetRunN(m_run);
    ev->SetEventN(m_ev);
    ev->SetTriggerN(m_ev); 
    ev->SetTimestamp(timeSinceStart, timeSinceStart);
    if (m_ev==1) ev->SetBORE();    

    ev->AddBlock(0, dataStream);
    //Adding the event to the EUDAQ format
    SendEvent(std::move(ev));
    dataStream.clear();

    if (_mode == MCP_DEBUG) eudaq::mSleep(100);

  }
}

