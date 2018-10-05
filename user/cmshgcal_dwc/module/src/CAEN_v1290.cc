#include "CAEN_v1290.h"


//#define CAENV1290_DEBUG

bool CAEN_V1290::Init() {
  int status = 0;
  std::cout << "[CAEN_V1290]::[INFO]::++++++ CAEN V1290 INIT ++++++" << std::endl;

  if (handle_ < 0) {
    std::cout << "Handle is negative --> Init has failed." << std::endl;
    return false;
  }

  char* software_version = new char[100];
  CAENVME_SWRelease(software_version);
  std::cout << "CAENVME library version: " << software_version << std::endl;

  return true;
}

int CAEN_V1290::SetupModule() {
  int status = 0;
  std::cout << "[CAEN_V1290]::[INFO]::++++++ CAEN V1290 MODULE SETUP ++++++" << std::endl;

  if (!IsConfigured()) {
    std::cout << "Device configuration has not been defined --> Module setup has failed." << std::endl;
    return ERR_CONF_NOT_FOUND;
  }
  //Read Version to check connection
  WORD data = 0;

  eudaq::mSleep(1);
  status |= CAENVME_ReadCycle(handle_, configuration_.baseAddress + CAEN_V1290_FW_VER_REG, &data, CAEN_V1290_ADDRESSMODE, cvD16);
  if (status) {
    std::cout << "[CAEN_V1290]::[ERROR]::Cannot open V1290 board @0x" << std::hex << configuration_.baseAddress << std::dec << " " << status << std::endl;
    return ERR_OPEN;
  }

  unsigned short FWVersion[2];
  for (int i = 0; i < 2; ++i) {
    FWVersion[i] = (data >> (i * 4)) & 0xF;
  }
  std::cout << "[CAEN_V1290]::[INFO]::Open V1290 board @0x" << std::hex << configuration_.baseAddress << std::dec << " FW Version Rev." << FWVersion[1] << "." << FWVersion[0] << std::endl;

  eudaq::mSleep(1);
  data = 0;
  std::cout << "[CAEN_V1290]::[INFO]::Software reset ... " << std::endl;
  status |= CAENVME_WriteCycle(handle_, configuration_.baseAddress + CAEN_V1290_SW_RESET_REG, &data, CAEN_V1290_ADDRESSMODE, cvD16);

  std::cout << "Status: " << status << std::endl;
  if (status) {
    std::cout << "[CAEN_V1290]::[ERROR]::Cannot reset V1290 board " << status << std::endl;
    return ERR_RESET;
  }


  eudaq::mSleep(1);

  //Now the real configuration
  if (configuration_.model == CAEN_V1290_N) {
    channels_ = 16;
  } else if (configuration_.model == CAEN_V1290_A) {
    channels_ = 32;
  } else {

  }

  //configurations using the con register
  data = 0;
  status |= CAENVME_WriteCycle(handle_, configuration_.baseAddress + CAEN_V1290_CON_REG, &data, CAEN_V1290_ADDRESSMODE, cvD16);
  if (configuration_.emptyEventEnable) {
    std::cout << "[CAEN_V1290]::[INFO]::Enable empty events" << std::endl;
    data |= CAEN_V1290_EMPTYEVEN_BITMASK; //enable emptyEvent
  }
  if (configuration_.recordTriggerTimeStamp) {
    std::cout << "[CAEN_V1290]::[INFO]::Enable empty events" << std::endl;
    data |= CAEN_V1290_TIMESTAMP_BITMASK; //enable writing of trigger time stamps
  }
  eudaq::mSleep(1);
  status |= CAENVME_WriteCycle(handle_, configuration_.baseAddress + CAEN_V1290_CON_REG, &data, CAEN_V1290_ADDRESSMODE, cvD16);


  //OpWriteTDC writes to the microcontroller
  eudaq::mSleep(1);
  // I step: set TRIGGER Matching mode
  if (configuration_.triggerMatchMode) {
    std::cout << "[CAEN_V1290]::[INFO]::Enabled Trigger Match Mode" << std::endl;
    status |= OpWriteTDC(CAEN_V1290_TRMATCH_OPCODE);
  }

  eudaq::mSleep(1);
  // I step: set Edge detection
  std::cout << "[CAEN_V1290]::[INFO]::EdgeDetection " << configuration_.edgeDetectionMode << std::endl;
  status |= OpWriteTDC(CAEN_V1290_EDGEDET_OPCODE);
  status |= OpWriteTDC(configuration_.edgeDetectionMode);


  eudaq::mSleep(1);
  // I step: set Time Resolution
  std::cout << "[CAEN_V1290]::[INFO]::TimeResolution " << configuration_.timeResolution << std::endl;
  status |= OpWriteTDC(CAEN_V1290_TIMERESO_OPCODE);
  status |= OpWriteTDC(configuration_.timeResolution);



  eudaq::mSleep(1);
  // II step: set TRIGGER Window Width to value n
  std::cout << "[CAEN_V1290]::[INFO]::Set Trigger window width " << configuration_.windowWidth << std::endl;
  status |= OpWriteTDC(CAEN_V1290_WINWIDT_OPCODE);
  status |= OpWriteTDC(configuration_.windowWidth);

  eudaq::mSleep(1);
  // III step: set TRIGGER Window Offset to value -n
  std::cout << "[CAEN_V1290]::[INFO]::TimeWindowWidth " << configuration_.windowWidth << " TimeWindowOffset " << configuration_.windowOffset << std::endl;
  status |= OpWriteTDC(CAEN_V1290_WINOFFS_OPCODE);
  status |= OpWriteTDC(configuration_.windowOffset);


  //disable all channels
  eudaq::mSleep(1);
  std::cout << "[CAEN_V1290]::[INFO]::Disabling all channel " << std::endl;
  status |= OpWriteTDC(CAEN_V1290_DISALLCHAN_OPCODE);

  // enable channels
  eudaq::mSleep(1);
  for (unsigned int i = 0; i < channels_; ++i) {
    if (! (configuration_.enabledChannels & ( 1 << i )) ) continue;
    std::cout << "[CAEN_V1290]::[INFO]::Enabling channel " << i << std::endl;
    status |= OpWriteTDC(CAEN_V1290_ENCHAN_OPCODE + i);
    eudaq::mSleep(1);
  }

  //number of hits unlimited for July 2017 data taking (14.07.2017)
  eudaq::mSleep(1);
  std::cout << "[CAEN_V1290]::[INFO]::Setting max hits per trigger " << configuration_.maxHitsPerEvent << std::endl;
  status |= OpWriteTDC(CAEN_V1290_MAXHITS_OPCODE);
  status |= OpWriteTDC(configuration_.maxHitsPerEvent);


  eudaq::mSleep(1);
  //Enable trigger time subtraction
  if (configuration_.triggerTimeSubtraction) {
    std::cout << "[CAEN_V1290]::[INFO]::Enabling trigger time subtraction " << std::endl;
    status |= OpWriteTDC(CAEN_V1290_ENTRSUB_OPCODE);
  } else {
    std::cout << "[CAEN_V1290]::[INFO]::Disabling trigger time subtraction " << std::endl;
    status |= OpWriteTDC(CAEN_V1290_DISTRSUB_OPCODE);
  }

  if (status) {
    std::cout << "[CAEN_V1290]::[ERROR]::Initialisation error" << status << std::endl;
    return ERR_CONFIG;
  }


  eudaq::mSleep(1);
  std::cout << "[CAEN_V1290]::[INFO]::++++++ CAEN V1290 CONFIGURED ++++++" << std::endl;
  CheckStatusAfterRead();


  return 0;
}


int CAEN_V1290::Clear() {
  //Send a software reset. Module has to be re-initialized after this
  int status = 0;
  if (handle_ < 0)
    return ERR_CONF_NOT_FOUND;

  WORD data = 0xFF;
  status |= CAENVME_WriteCycle(handle_, configuration_.baseAddress + CAEN_V1290_SW_RESET_REG, &data, CAEN_V1290_ADDRESSMODE, cvD16);

  if (status) {
    std::cout << "[CAEN_V1290]::[ERROR]::Cannot reset V1290 board " << status << std::endl;
    return ERR_RESET;
  }

  /*
  eudaq::mSleep(1);

  status |= CAENVME_SystemReset(handle_);
  if (status) {
    std::cout << "[CAEN_VX2718]::[ERROR]::Cannot reset VX2718 board " << status << std::endl;
    return ERR_RESET;
  }

  status=Init();
  */
  return status;
}


int CAEN_V1290::BufferClear() {
//Send a software reset. Module has to be re-initialized after this
  int status = 0;
  if (handle_ < 0)
    return ERR_CONF_NOT_FOUND;

  WORD data = 0xFF;
  status |= CAENVME_WriteCycle(handle_, configuration_.baseAddress + CAEN_V1290_SW_CLEAR_REG, &data, CAEN_V1290_ADDRESSMODE, cvD16);

  if (status) {
    std::cout << "[CAEN_V1290]::[ERROR]::Cannot clear V1290 buffers " << status << std::endl;
    return ERR_RESET;
  }

  return 0;
}


//19 May 2017: So far only some dummy values
int CAEN_V1290::Config(CAEN_V1290::CAEN_V1290_Config_t &_config) {
  GetConfiguration()->baseAddress = _config.baseAddress;
  GetConfiguration()->model = _config.model;
  GetConfiguration()->triggerTimeSubtraction = _config.triggerTimeSubtraction;
  GetConfiguration()->triggerMatchMode = _config.triggerMatchMode;
  GetConfiguration()->emptyEventEnable = _config.emptyEventEnable;
  GetConfiguration()->edgeDetectionMode = _config.edgeDetectionMode;
  GetConfiguration()->timeResolution = _config.timeResolution;
  GetConfiguration()->maxHitsPerEvent = _config.maxHitsPerEvent;
  GetConfiguration()->enabledChannels = _config.enabledChannels;
  GetConfiguration()->windowWidth = _config.windowWidth;
  GetConfiguration()->windowOffset = _config.windowOffset;

  _isConfigured = true;

  return 0;
}


bool CAEN_V1290::DataReady() {
  int status = 0;

  if (handle_ < 0) {
    std::cout << "[CAEN_V1290]::[ERROR]::V1290 board handle not found" << status << std::endl;
    return ERR_CONF_NOT_FOUND;
  }

  WORD data;
  int v1290_rdy = 0;

  int ntry = 10, nt = 0;
  while ( (v1290_rdy != 1 ) && nt < ntry ) {
    status |= CAENVME_ReadCycle(handle_, configuration_.baseAddress + CAEN_V1290_STATUS_REG, &data, CAEN_V1290_ADDRESSMODE, cvD16);
    v1290_rdy = data & CAEN_V1290_RDY_BITMASK;
    ++nt;
  }

  return (v1290_rdy == 1);

}

//Read the ADC buffer and send it out in WORDS vector
int CAEN_V1290::Read(std::vector<WORD> &v) {
  int status = 0;

  if (handle_ < 0) {
    v.push_back( (0x4 << 28) | (0 & 0x7FFF ));
    std::cout << "[CAEN_V1290]::[ERROR]::V1290 board handle not found" << status << std::endl;
    return ERR_CONF_NOT_FOUND;
  }

  WORD data;

  int v1290_rdy = 0;
  int v1290_error = 1;

  int ntry = 1, nt = 0;
  //Wait for a valid datum in the ADC
  while ( (v1290_rdy != 1 || v1290_error != 0 ) && nt < ntry ) {
    status |= CAENVME_ReadCycle(handle_, configuration_.baseAddress + CAEN_V1290_STATUS_REG, &data, CAEN_V1290_ADDRESSMODE, cvD16);
    v1290_rdy = data & CAEN_V1290_RDY_BITMASK;
    v1290_error = 0;
    v1290_error |= data & CAEN_V1290_ERROR0_BITMASK;
    v1290_error |= data & CAEN_V1290_ERROR1_BITMASK;
    v1290_error |= data & CAEN_V1290_ERROR2_BITMASK;
    v1290_error |= data & CAEN_V1290_ERROR3_BITMASK;
    v1290_error |= data & CAEN_V1290_TRGLOST_BITMASK;
    ++nt;
  }

  if (v1290_rdy == 0) {
#ifdef CAENV1290_DEBUG
    std::cout << "[CAEN_V1290]::[ERROR]::V1290 board not ready" << status << std::endl;
#endif
    //v.push_back( (0x4 << 28) | (1 & 0x7FFF ));
    return ERR_NONE;
  }

  if (status || v1290_error != 0) {
    std::cout << "[CAEN_V1290]::[ERROR]::Cannot get a valid data from V1290 board " << status << std::endl;
    std::cout << "CAEN_V1290_ERROR0_BITMASK: " << (data & CAEN_V1290_ERROR0_BITMASK) << std::endl;
    std::cout << "CAEN_V1290_ERROR1_BITMASK: " << (data & CAEN_V1290_ERROR1_BITMASK) << std::endl;
    std::cout << "CAEN_V1290_ERROR2_BITMASK: " << (data & CAEN_V1290_ERROR2_BITMASK) << std::endl;
    std::cout << "CAEN_V1290_ERROR3_BITMASK: " << (data & CAEN_V1290_ERROR3_BITMASK) << std::endl;
    std::cout << "CAEN_V1290_TRGLOST_BITMASK: " << (data & CAEN_V1290_TRGLOST_BITMASK) << std::endl;

    v.push_back( (0x4 << 28) | (2 & 0x7FFF ));
    return ERR_READ;
  }

#ifdef CAENV1290_DEBUG
  std::cout << "[CAEN_V1290]::[DEBUG]::RDY " << v1290_rdy << " ERROR " << v1290_error << " NTRY " << nt << std::endl;
#endif

  data = 0;

  status |= CAENVME_ReadCycle(handle_, configuration_.baseAddress + CAEN_V1290_OUTPUT_BUFFER, &data, CAEN_V1290_ADDRESSMODE, cvD32);

#ifdef CAENV1290_DEBUG
  std::cout << "[CAEN_V1290]::[DEBUG]::IS EVENT HEADER " << ((data & 0x40000000) >> 30) << std::endl;
#endif


  if ( ! (data & 0x40000000) || status ) {
    v.push_back( (0x4 << 28) | (3 & 0x7FFF ));
    std::cout << "[CAEN_V1290]::[ERROR]::First word not a Global header" << std::endl;
    return ERR_READ;
  }

  int evt_num = data >> 5 & 0x3FFFFF;
#ifdef CAENV1290_DEBUG
  std::cout << "[CAEN_V1290]::[DEBUG]::EVENT NUMBER " << evt_num << std::endl;
#endif
  v.push_back( (0xA << 27) | (evt_num & 0xFFFFFFF ));     //defines the global header in the data stream that is sent to the producer


  //Read until trailer
  int glb_tra = 0;
  while (!glb_tra && !status) {
    data = 0;
    status |= CAENVME_ReadCycle(handle_, configuration_.baseAddress + CAEN_V1290_OUTPUT_BUFFER, &data, CAEN_V1290_ADDRESSMODE, cvD32);
    int wordType = (data >> 27) & CAEN_V1290_WORDTYE_BITMASK;
#ifdef CAENV1290_DEBUG
    std::cout << "[CAEN_V1290]::[DEBUG]::EVENT WORDTYPE " << wordType << std::endl;
#endif

    if (wordType == CAEN_V1290_GLBTRAILER) {
#ifdef CAENV1290_DEBUG
      int TDC_ERROR = (data >> 24) & 0x1;
      int OUTPUT_BUFFER_ERROR = (data >> 25) & 0x1;
      int TRIGGER_LOST = (data >> 26) & 0x1;
      int WORD_COUNT = (data >> 5) & 0x10000;
      std::cout << "[CAEN_V1290]::[DEBUG]::WE FOUND THE EVENT TRAILER" << std::endl;
      std::cout << "TDC_ERROR: " << TDC_ERROR << std::endl;
      std::cout << "OUTPUT_BUFFER_ERROR: " << OUTPUT_BUFFER_ERROR << std::endl;
      std::cout << "TRIGGER_LOST: " << TRIGGER_LOST << std::endl;
      std::cout << "WORD_COUNT: " << WORD_COUNT << std::endl;
#endif
      glb_tra = 1;
      v.push_back(data);
    } else if (wordType == CAEN_V1290_TDCHEADER ) {
#ifdef CAENV1290_DEBUG
      int TDCnr = (data >> 24) & 0x3;
      int eventID = (data >> 12) & 0xFFF;
      int bunchID = (data) & 0xFFF;
      std::cout << "[CAEN_V1290]::[DEBUG]::WE FOUND THE TDC HEADER of nr. " << TDCnr << " with eventID " << eventID << " and bunchID " << bunchID << std::endl;
#endif
      v.push_back(data);
    } else if (wordType == CAEN_V1290_TDCTRAILER ) {
#ifdef CAENV1290_DEBUG
      int TDCnr = (data >> 24) & 0x3;
      int eventID = (data >> 12) & 0xFFF;
      int wordCount = (data) & 0xFFF;
      std::cout << "[CAEN_V1290]::[DEBUG]::WE FOUND THE TDC TRAILER of nr. " << TDCnr << " with eventID " << eventID << " and word count " << wordCount << std::endl;
#endif
      v.push_back(data);
    } else if (wordType == CAEN_V1290_TDCERROR ) {
      std::cout << "[CAEN_V1290]::[ERROR]::TDC ERROR!" << std::endl;
      v.push_back(data);
      return ERR_READ;
    } else if (wordType == CAEN_V1290_TDCMEASURE ) {
      v.push_back(data);
#ifdef CAENV1290_DEBUG
      int measurement = data & 0x1fffff;
      int channel = (data >> 21) & 0x1f;
      int trailing = (data >> 26) & 0x1;
      float tdc_time = (float)measurement * 25. / 1000.;
      std::cout << "[CAEN_V1290]::[INFO]::HIT CHANNEL " << channel << " TYPE " << trailing << " TIME " << tdc_time << std::endl;
#endif
    } else if (wordType == CAEN_V1290_GLBTRTIMETAG ) {
      v.push_back(data);
    } else {
      std::cout << "[CAEN_V1290]::[ERROR]::UNKNOWN WORD TYPE!" << std::endl;
      v.clear();
      v.push_back( (0x7 << 27) | (5 & 0x7FFF ));
      return ERR_READ;
    }
  }

  if (status) {
    std::cout << "[CAEN_V1290]::[ERROR]::READ ERROR!" << std::endl;
    v.clear();
    v.push_back( (0x4 << 27) | (6 & 0x7FFF ));
    return ERR_READ;
  }

  if (status) {
    std::cout << "[CAEN_V1290]::[ERROR]::MEB Full or problems reading" << status << std::endl;
    return ERR_READ;
  }
  return 0;
}


int CAEN_V1290::CheckStatusAfterRead() {
  if (handle_ < 0) {
    return ERR_CONF_NOT_FOUND;
  }

  int status = 0;
  WORD data;
  status |= CAENVME_ReadCycle(handle_, configuration_.baseAddress + CAEN_V1290_STATUS_REG, &data, CAEN_V1290_ADDRESSMODE, cvD16);
  if (status) {
    std::cout << "[CAEN_V1290]::[ERROR]::Cannot get status after read from V1290 board " << status << std::endl;
  }

  int v1290_EMPTYEVEN       = data & CAEN_V1290_EMPTYEVEN_BITMASK;
  int v1290_RDY             = data & CAEN_V1290_RDY_BITMASK;
  int v1290_FULL            = data & CAEN_V1290_FULL_BITMASK;
  int v1290_TRGMATCH        = data & CAEN_V1290_TRGMATCH_BITMASK;
  int v1290_TERMON          = data & CAEN_V1290_TERMON_BITMASK;
  int v1290_ERROR0          = data & CAEN_V1290_ERROR0_BITMASK;
  int v1290_ERROR1          = data & CAEN_V1290_ERROR1_BITMASK;
  int v1290_ERROR2          = data & CAEN_V1290_ERROR2_BITMASK;
  int v1290_ERROR3          = data & CAEN_V1290_ERROR3_BITMASK;
  int v1290_TRGLOST         = data & CAEN_V1290_TRGLOST_BITMASK;
  int v1290_WORDTYE         = data & CAEN_V1290_WORDTYE_BITMASK;



  int v1290_error = 0;
  v1290_error |= data & CAEN_V1290_ERROR0_BITMASK;
  v1290_error |= data & CAEN_V1290_ERROR1_BITMASK;
  v1290_error |= data & CAEN_V1290_ERROR2_BITMASK;
  v1290_error |= data & CAEN_V1290_ERROR3_BITMASK;
  v1290_error |= data & CAEN_V1290_TRGLOST_BITMASK;

  if ( v1290_FULL || v1290_error ) {
    std::cout << "[CAEN_V1290]::[INFO]::Trying to restore V1290 board. Reset" << status << std::endl;
    std::cout << "[CAEN_V1290]::[INFO]::Status: " << std::endl;
    std::cout << "v1290_EMPTYEVEN: " << v1290_EMPTYEVEN << std::endl;
    std::cout << "v1290_RDY: " << v1290_RDY << std::endl;
    std::cout << "v1290_FULL: " << v1290_FULL << std::endl;
    std::cout << "v1290_TRGMATCH: " << v1290_TRGMATCH << std::endl;
    std::cout << "v1290_TERMON: " << v1290_TERMON << std::endl;
    std::cout << "v1290_ERROR0: " << v1290_ERROR0 << std::endl;
    std::cout << "v1290_ERROR1: " << v1290_ERROR1 << std::endl;
    std::cout << "v1290_ERROR2: " << v1290_ERROR2 << std::endl;
    std::cout << "v1290_ERROR3: " << v1290_ERROR3 << std::endl;
    std::cout << "v1290_TRGLOST: " << v1290_TRGLOST << std::endl;
    std::cout << "v1290_WORDTYPE: " << v1290_WORDTYE << std::endl;
    std::cout << std::endl;


    status = BufferClear();
    //full reset of TDC if the clearing of buffers did not help, added on 09 August 2018 by T.Q.
    if (status) {
      status = Clear();
      SetupModule();
    }
  }
  if (status) {
    std::cout << "[CAEN_V1290]::[ERROR]::Cannot restore healthy state in V1290 board " << status << std::endl;
    return ERR_READ;
  } else {
    std::cout << "[CAEN_V1290]::[STATUS]::V1290 board is in healthy state " << status << std::endl;
    return ERR_NONE;
  }

  return 0;
}


int CAEN_V1290::OpWriteTDC(WORD data) {
  int status;

  const int TIMEOUT = 100000;

  int time = 0;
  /* Check the Write OK bit */
  WORD rdata = 0;

  do {
    status = CAENVME_ReadCycle(handle_, configuration_.baseAddress + CAEN_V1290_MICROHANDREG , &rdata, CAEN_V1290_ADDRESSMODE, cvD16);
    time++;
    eudaq::mSleep(10);
  } while (!(rdata & 0x1) && (time < TIMEOUT) );

#ifdef CAENV1290_DEBUG
  std::cout << "[CAEN_V1290]::[DEBUG]::Number of requests to wait for the handshake: " << time << " return data: " << rdata << std::endl;
#endif


  if ( time == TIMEOUT ) {
    std::cout << "[CAEN_V1290]::[ERROR]::Cannot handshake micro op writing " << status << std::endl;
    return ERR_WRITE_OP;
  }

  status = 0;

  status |= CAENVME_WriteCycle(handle_, configuration_.baseAddress + CAEN_V1290_MICROREG, &data, CAEN_V1290_ADDRESSMODE, cvD16);
  if (status) {
    std::cout << "[CAEN_V1290]::[ERROR]::Cannot write micro op " << status << std::endl;
    return ERR_WRITE_OP;
  }

  return 0;
}

/*----------------------------------------------------------------------*/

int CAEN_V1290::OpReadTDC(WORD* data) {
  int status;
  const int TIMEOUT = 100000;

  int time = 0;
  /* Check the Write OK bit */
  WORD rdata = 0;
  do {
    status = CAENVME_ReadCycle(handle_, configuration_.baseAddress +  CAEN_V1290_MICROHANDREG , &rdata, CAEN_V1290_ADDRESSMODE, cvD16);
    time++;
  } while (!(rdata & 0x2) && (time < TIMEOUT) );

#ifdef CAENV1290_DEBUG
  std::cout << "[CAEN_V1290]::[DEBUG]::Number of requests to wait for the handshake: " << time << " return data: " << rdata << std::endl;
#endif

  if ( time == TIMEOUT ) {
    std::cout << "[CAEN_V1290]::[ERROR]::Cannot handshake micro op reading " << status << std::endl;
    return ERR_WRITE_OP;
  }

  status = 0;
  status |= CAENVME_ReadCycle(handle_, configuration_.baseAddress + CAEN_V1290_MICROREG, data, CAEN_V1290_ADDRESSMODE, cvD16);
  if (status) {
    std::cout << "[CAEN_V1290]::[ERROR]::Cannot read micro op " << status << std::endl;
    return ERR_WRITE_OP;
  }

  return 0;
}



//Testing functions:
int CAEN_V1290::EnableTDCTestMode(WORD testData) {
  int status = 0;

  // I step: Enable the test mode
  status |= OpWriteTDC(CAEN_V1290_ENABLE_TEST_MODE_OPCODE);
  eudaq::mSleep(10);
  // II step: Write the test word
  status |= OpWriteTDC(testData);
  eudaq::mSleep(10);

  // III step: Write a t=0x0044 into the test-word high
  status |= OpWriteTDC(0x0044);
  eudaq::mSleep(10);

  // IV step: set the test output
  for (unsigned int channel = 0; channel <= 15; channel++) {
    status |= OpWriteTDC(CAEN_V1290_SETTESTOUTPUT);
    eudaq::mSleep(20);
    status |= OpWriteTDC(channel * channel);
    eudaq::mSleep(20);
  }

  return status;
}

int CAEN_V1290::DisableTDCTestMode() {
  int status = 0;

  status |= OpWriteTDC(CAEN_V1290_DISABLE_TEST_MODE_OPCODE);
  return status;
}

int CAEN_V1290::SoftwareTrigger() {
  int status = 0;
  WORD dummy_data = 0x0000;
  status |= CAENVME_WriteCycle(handle_, configuration_.baseAddress + CAEN_V1290_SOFTWARETRIGGERREG, &dummy_data, CAEN_V1290_ADDRESSMODE, cvD16);
  return status;
}



void CAEN_V1290::generatePseudoData(unsigned int eventNr, std::vector<WORD> &data) {

  WORD bitStream = 0;

  //first the BOE
  bitStream = bitStream | 10 << 27;
  data.push_back(bitStream);

  //extended trigger time stamp
  bitStream = 0;
  bitStream = bitStream | 0x11 << 27;
  bitStream = bitStream | eventNr * 40;
  data.push_back(bitStream);


  unsigned int beamX = (rand() % 10000) / 100 - 50;
  unsigned int beamY = (rand() % 10000) / 100 - 50;
  unsigned int _default = 20000;

  unsigned int readouts[16] = {_default + rand() % 1000, _default - beamX * 200, _default + rand() % 1000, _default - beamY * 200, _default + rand() % 1000, _default - beamX * 200, _default + rand() % 1000, _default - beamY * 200, _default + rand() % 1000, _default - beamX * 200, _default + rand() % 1000, _default - beamY * 200, _default + rand() % 1000, _default - beamX * 200, _default + rand() % 1000, _default - beamY * 200};


  //generate the channel information
  for (unsigned int channel = 0; channel < 16; channel++) {
    if (rand() % 100 < 10) continue; //ten percent change of no hit detection

    unsigned int readout = readouts[channel];

    int N_hits = rand() % 18; //maximum 10 hits in one readout
    for (int N = 0; N < N_hits; N++) {
      bitStream = 0;
      bitStream = bitStream | 0 << 27;
      bitStream = bitStream | channel << 21;
      bitStream = bitStream | (readout + N);
      data.push_back(bitStream);
    }
  }

  //last the BOE
  bitStream = 0;
  bitStream = bitStream | 0x10 << 27;
  data.push_back(bitStream);
}
