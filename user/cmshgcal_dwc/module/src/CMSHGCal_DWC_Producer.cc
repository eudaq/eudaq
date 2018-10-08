#include "eudaq/Configuration.hh"
#include "eudaq/Producer.hh"
#include "eudaq/Logger.hh"
#include "eudaq/RawEvent.hh"
#include <iostream>
#include <ostream>
#include <cstdlib>
#include <string>
#include <vector>

#include "CAEN_v1290.h"

#include <chrono>

enum RUNMODE {
  TDC_DEBUG = 0,
  TDC_RUN
};

static const std::string EVENT_TYPE = "CMSHGCal_DWC_RawEvent";

class CMSHGCal_DWC_Producer : public eudaq::Producer {
public:
  CMSHGCal_DWC_Producer(const std::string & name, const std::string & runcontrol);
  void DoInitialise() override;
  void DoConfigure() override;
  void DoStartRun() override;
  void DoStopRun() override;
  void DoTerminate() override;
  void DoReset() override;
  void RunLoop() override;

  static const uint32_t m_id_factory = eudaq::cstr2hash("CMSHGCal_DWC_Producer");
private:
  //parameters for readout control
  unsigned m_run, m_ev;
  bool stopping, done;
  bool initialized, connection_initialized;

  //TDC communication
  std::vector<CAEN_V1290*> tdcs;
  int *VX2718handle;
  bool* tdcDataReady;
  bool performReadout;
  int readoutError, tdc_error;
  std::vector<WORD> dataStream;

  std::chrono::steady_clock::time_point startTime;
  uint64_t timeSinceStart;

  //set at configuration
  int m_readoutSleep;   //sleep in the readout loop in microseconds

  //set on initialisation
  RUNMODE _mode;
  int NumberOfTDCs;

};


namespace {
auto dummy0 = eudaq::Factory<eudaq::Producer>::
              Register<CMSHGCal_DWC_Producer, const std::string&, const std::string&>(CMSHGCal_DWC_Producer::m_id_factory);
}


CMSHGCal_DWC_Producer::CMSHGCal_DWC_Producer(const std::string & name, const std::string & runcontrol)
  : eudaq::Producer(name, runcontrol), m_run(0), m_ev(0), stopping(false), done(false) {
  initialized = false; connection_initialized = false;
  _mode = TDC_DEBUG;
  NumberOfTDCs = -1;
  m_readoutSleep = 1000;  //1 ms for sleepin in readout cycle as default
}


void CMSHGCal_DWC_Producer::DoInitialise() {
  if (connection_initialized) return;
  auto ini = GetInitConfiguration();
  std::cout << "Initialisation of the CMS HGCal DWC Producer..." << std::endl;
 
    //run the debug mode or run with real TDCs?
    int mode = ini->Get("AcquisitionMode", 0);
    switch ( mode ) {
    case 0 :
      _mode = TDC_DEBUG;
      break;
    case 1:
    default :
      _mode = TDC_RUN;
      break;
    }
    std::cout << "Mode at initialisation: " << _mode << std::endl;

    if (_mode == TDC_RUN) {
      //necessary: setup the communication board (VX2718)
      //corresponding values for the init function are taken from September 2016 configuration
      //https://github.com/cmsromadaq/H4DAQ/blob/master/data/H2_2016_08_HGC/config_pcminn03_RC.xml#L26
      VX2718handle = new int;
      int status = CAENVME_Init(static_cast<CVBoardTypes>(1), 0, 0, VX2718handle);
      if (status) {
        std::cout << "[CAEN_VX2718]::[ERROR]::Cannot open VX2718 board...Is it connected and powered?" << std::endl;
        SetStatus(eudaq::Status::STATE_ERROR, "Initialisation Error");
  initialized = false;
  return;
      }
  }

    //Define the number of TDCs
    if (NumberOfTDCs == -1) { //do not allow for dynamic changing of TDCs because the number of DQM plots depend on it and are determined at first runtime.
      for (size_t i = 0; i < tdcs.size(); i++) delete tdcs[i];
      tdcs.clear();
      NumberOfTDCs = ini->Get("NumberOfTDCs", 1);
      tdcDataReady = new bool[NumberOfTDCs];
      for (int id = 1; id <= NumberOfTDCs; id++) {
        tdcs.push_back(new CAEN_V1290(id));
        tdcDataReady[id - 1] = false;
      }
    } else {
      std::cout << "Number of TDCs(=" << NumberOfTDCs << ") has not been changed. Restart the producer to change the number of TDCs." << std::endl;
    }

    if (_mode == TDC_RUN) {
        bool tdcs_initialized = true;
        for (int i = 0; i < NumberOfTDCs; i++) {
          tdcs[i]->SetHandle(*VX2718handle);
          tdcs_initialized = tdcs[i]->Init() && tdcs_initialized;
        }
        initialized = tdcs_initialized;
    } else initialized = true;

    connection_initialized = true;
    if (initialized) SetStatus(eudaq::Status::STATE_UNCONF, "Initialised (" + ini->Name() + ")");
}


void CMSHGCal_DWC_Producer::DoConfigure() {
  if (!initialized) SetStatus(eudaq::Status::STATE_ERROR, "Configuration Error"); 
  auto conf = GetConfiguration();
  SetStatus(eudaq::Status::STATE_UNCONF, "Configuring (" + conf->Name() + ")");
  std::cout << "Configuring: " << conf->Name() << std::endl;
  conf->Print(std::cout);


  m_readoutSleep = conf->Get("readoutSleep", 1000);

  for (int i = 0; i < NumberOfTDCs; i++) {
    if (_mode == TDC_RUN) {
      if (initialized) {
        CAEN_V1290::CAEN_V1290_Config_t _config;
        _config.baseAddress = conf->Get(("baseAddress_" + std::to_string(i + 1)).c_str(), 0x00AA0000);
        _config.model = static_cast<CAEN_V1290::CAEN_V1290_Model_t>(conf->Get(("model_" + std::to_string(i + 1)).c_str(), 1));
        _config.triggerTimeSubtraction = static_cast<bool>(conf->Get(("triggerTimeSubtraction_" + std::to_string(i + 1)).c_str(), 1));
        _config.triggerMatchMode = static_cast<bool>(conf->Get(("triggerMatchMode_" + std::to_string(i + 1)).c_str(), 1));
        _config.emptyEventEnable = static_cast<bool>(conf->Get(("emptyEventEnable_" + std::to_string(i + 1)).c_str(), 1));
        _config.recordTriggerTimeStamp = static_cast<bool>(conf->Get(("recordTriggerTimeStamp_" + std::to_string(i + 1)).c_str(), 1));
        _config.edgeDetectionMode = static_cast<CAEN_V1290::CAEN_V1290_EdgeDetection_t>(conf->Get(("edgeDetectionMode_" + std::to_string(i + 1)).c_str(), 3));
        _config.timeResolution = static_cast<CAEN_V1290::CAEN_V1290_TimeResolution_t>(conf->Get(("timeResolution_" + std::to_string(i + 1)).c_str(), 3));
        _config.maxHitsPerEvent = static_cast<CAEN_V1290::CAEN_V1290_MaxHits_t>(conf->Get(("maxHitsPerEvent_" + std::to_string(i + 1)).c_str(), 8));
        _config.enabledChannels = conf->Get(("enabledChannels_" + std::to_string(i + 1)).c_str(), 0x00FF);
        _config.windowWidth = conf->Get(("windowWidth_" + std::to_string(i + 1)).c_str(), 0x40);
        _config.windowOffset = conf->Get(("windowOffset_" + std::to_string(i + 1)).c_str(), -1);
        tdcs[i]->Config(_config);
        initialized = (tdcs[i]->SetupModule()==0) && initialized;
      }
    }
  }
  if (initialized) SetStatus(eudaq::Status::STATE_CONF, "Configured (" + conf->Name() + ")");
}

void CMSHGCal_DWC_Producer::DoStartRun() {
  m_run = GetRunNumber();
  m_ev = 0;

  EUDAQ_INFO("Start Run: " + m_run);

  if (_mode == TDC_RUN) {
    if (initialized)
      for (size_t i = 0; i < tdcs.size(); i++) tdcs[i]->BufferClear();
    else {
      EUDAQ_INFO("ATTENTION !!! Communication to the TDC has not been established");
      SetStatus(eudaq::Status::STATE_ERROR, "Communication to the TDC has not been established");
    }
  }

  SetStatus(eudaq::Status::STATE_RUNNING, "Running");
  startTime = std::chrono::steady_clock::now();
  stopping = false;
}

void CMSHGCal_DWC_Producer::DoStopRun() {
  SetStatus(eudaq::Status::STATE_CONF, "Stopping");
  EUDAQ_INFO("Stopping Run: " + m_run);
  std::cout << "[RUN " << m_run << "] Number of events read: " << m_ev << std::endl;
  stopping = true;
}

void CMSHGCal_DWC_Producer::DoReset() {
  if (_mode == TDC_RUN) {
    if (initialized)
      for (size_t i = 0; i < tdcs.size(); i++) tdcs[i]->Clear();
    else {
      EUDAQ_INFO("ATTENTION !!! Communication to the TDC has not been established");
      SetStatus(eudaq::Status::STATE_ERROR, "Communication to the TDC has not been established");
    }
  }
}

void CMSHGCal_DWC_Producer::DoTerminate() {
  EUDAQ_INFO("Terminating...");
  done = true;
  eudaq::mSleep(200);
  for (size_t i = 0; i < tdcs.size(); i++) delete tdcs[i];
}

void CMSHGCal_DWC_Producer::RunLoop() {
  if (!initialized) return;
  while (!done) {
    usleep(m_readoutSleep);

    if (_mode == TDC_RUN) {
      performReadout = true;
      for (int i = 0; i < tdcs.size(); i++) {
        if(!tdcDataReady[i]) {usleep(50); tdcDataReady[i] = tdcs[i]->DataReady();}
        performReadout = performReadout && tdcDataReady[i];        
        //if (tdcDataReady[i] == true) {std::cout<<"TDC "<<i<<" ready for readout..."<<std::endl; continue;}
      }
      if (!performReadout) {
        if (stopping) {stopping = false; SetStatus(eudaq::Status::STATE_CONF, "Stopped"); return;}
        else continue;
      }
    } else {
      if (stopping) {stopping = false; SetStatus(eudaq::Status::STATE_CONF, "Stopped"); return;}
    }

    
    //When this point is reached, there is an event to read from the TDCs...
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
    if (m_ev == 1) ev->SetBORE();


    readoutError = CAEN_V1290::ERR_NONE;
    for (int i = 0; i < tdcs.size(); i++) {
      dataStream.clear();
      if (_mode == TDC_RUN && initialized) {
        tdc_error = tdcs[i]->Read(dataStream);
        readoutError = tdc_error;
      } else if (_mode == TDC_DEBUG) {
        tdcs[i]->generatePseudoData(m_ev, dataStream);
        readoutError = CAEN_V1290::ERR_NONE;
      }
      //std::cout << "Block size for TDC " << i << ": " << dataStream.size() << std::endl;
      ev->AddBlock(i, dataStream);
    }

    //Adding the event to the EUDAQ format
    if (readoutError==CAEN_V1290::ERR_NONE) SendEvent(std::move(ev)); //only send event when there was no readout error
    for (int i = 0; i < tdcs.size(); i++) tdcDataReady[i] = false;
    

    if (readoutError == CAEN_V1290::ERR_READ) {
      for (int i = 0; i < tdcs.size(); i++) {
        std::cout << "[EVENT " << m_ev << "] Checking the status of TDC " << i << " ..." << std::endl;
        tdcs[i]->CheckStatusAfterRead();
      }
    }

    if (_mode == TDC_DEBUG) eudaq::mSleep(100);
  }
}

