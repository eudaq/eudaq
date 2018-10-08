#ifndef CAEN_V1290_H
#define CAEN_V1290_H

#include <string>
#include <vector>
#include <iostream>
#include <stdlib.h>

#include "eudaq/Utils.hh"

#include "CAENVMElib.h"
#include "CAENVMEtypes.h"
#include "CAENVMEoslib.h"
#include "CAENComm.h"
#include <CAENDigitizer.h>

typedef uint32_t WORD;


#define CAEN_V1290_CHANNELS         32

#define CAEN_V1290_ADDRESSMODE cvA32_U_DATA

#define CAEN_V1290_OUTPUT_BUFFER           0x0000
#define CAEN_V1290_SW_EVRESET_REG          0x1018
#define CAEN_V1290_SW_CLEAR_REG            0x1016
#define CAEN_V1290_SW_RESET_REG            0x1014
#define CAEN_V1290_MICROHANDREG            0x1030
#define CAEN_V1290_MICROREG                0x102E
#define CAEN_V1290_EVT_FIFO                0x1038
#define CAEN_V1290_EVT_FIFO_STATUS         0x103E
#define CAEN_V1290_FW_VER_REG              0x1026
#define CAEN_V1290_STATUS_REG              0x1002
#define CAEN_V1290_CON_REG                 0x1000

#define CAEN_V1290_TRMATCH_OPCODE          0x0000
#define CAEN_V1290_WINWIDT_OPCODE          0x1000
#define CAEN_V1290_WINOFFS_OPCODE          0x1100
#define CAEN_V1290_ENTRSUB_OPCODE          0x1400
#define CAEN_V1290_DISTRSUB_OPCODE         0x1500
#define CAEN_V1290_READTRG_OPCODE          0x1600
#define CAEN_V1290_EDGEDET_OPCODE          0x2200
#define CAEN_V1290_READ_EDGEDET_OPCODE     0x2300
#define CAEN_V1290_TIMERESO_OPCODE         0x2400
#define CAEN_V1290_READTIMERESO_OPCODE     0x2600
#define CAEN_V1290_MAXHITS_OPCODE          0x3300

#define CAEN_V1290_ENCHAN_OPCODE           0x4000
#define CAEN_V1290_DISCHAN_OPCODE          0x4100
#define CAEN_V1290_ENALLCHAN_OPCODE        0x4200
#define CAEN_V1290_DISALLCHAN_OPCODE       0x4300

#define CAEN_V1290_EMPTYEVEN_BITMASK       0x0008
#define CAEN_V1290_TIMESTAMP_BITMASK       0x0200
#define CAEN_V1290_RDY_BITMASK             0x0001
#define CAEN_V1290_FULL_BITMASK            0x0004
#define CAEN_V1290_TRGMATCH_BITMASK        0x0008
#define CAEN_V1290_TERMON_BITMASK          0x0020
#define CAEN_V1290_ERROR0_BITMASK          0x0040
#define CAEN_V1290_ERROR1_BITMASK          0x0080
#define CAEN_V1290_ERROR2_BITMASK          0x0100
#define CAEN_V1290_ERROR3_BITMASK          0x0200
#define CAEN_V1290_TRGLOST_BITMASK         0x8000
#define CAEN_V1290_WORDTYE_BITMASK         0x001F

//for testing purposes and software trigger
#define CAEN_V1290_ENABLE_TEST_MODE_OPCODE         0xC500
#define CAEN_V1290_DISABLE_TEST_MODE_OPCODE        0xC600
#define CAEN_V1290_SOFTWARETRIGGERREG      0x101A
#define CAEN_V1290_SETTESTOUTPUT           0xC700

class CAEN_V1290 {
public:
  typedef enum  {
    ERR_NONE = 0,
    ERR_CONF_NOT_FOUND,
    ERR_OPEN,
    ERR_CONFIG,
    ERR_RESET,
    ERR_READ,
    ERR_WRITE_OP,
    ERR_READ_OP,
    ERR_DUMMY_LAST,
  } ERROR_CODES;

  typedef enum  {
    CAEN_V1290_A = 0, //32ch version (IDC inputs)
    CAEN_V1290_N = 1 //16ch version (LEMO inputs)
  } CAEN_V1290_Model_t;

  typedef enum  {
    CAEN_V1290_PAIRMODE = 0,
    CAEN_V1290_ONLY_TRAILING = 1,
    CAEN_V1290_ONLY_LEADING = 2,
    CAEN_V1290_TRAILING_AND_LEADING = 3,
  } CAEN_V1290_EdgeDetection_t;

  typedef enum  {
    CAEN_V1290_800PS_RESO = 0,
    CAEN_V1290_200PS_RESO = 1,
    CAEN_V1290_100PS_RESO = 2,
    CAEN_V1290_25PS_RESO = 3,
  } CAEN_V1290_TimeResolution_t;

  typedef enum  {
    CAEN_V1290_ZERO_HITS = 0,
    CAEN_V1290_ONE_HITS = 1,
    CAEN_V1290_TWO_HITS = 2,
    CAEN_V1290_FOUR_HITS = 3,
    CAEN_V1290_EIGHT_HITS = 4,
    CAEN_V1290_SIXTEEN_HITS = 5,
    CAEN_V1290_THIRTYTWO_HITS = 6,
    CAEN_V1290_SIXTYFOUR_HITS = 7,
    CAEN_V1290_ONEHUNDREDTWENTYEIGHT_HITS = 8,
    CAEN_V1290_NOLIMIT_HITS = 9,
  } CAEN_V1290_MaxHits_t;

  typedef enum  {
    CAEN_V1290_GLBHEADER = 0x8,
    CAEN_V1290_GLBTRAILER = 0x10,
    CAEN_V1290_GLBTRTIMETAG = 0x11,
    CAEN_V1290_TDCHEADER = 0x1,
    CAEN_V1290_TDCTRAILER = 0x3,
    CAEN_V1290_TDCERROR = 0x4,
    CAEN_V1290_TDCMEASURE = 0x0,
  } CAEN_V1290_WORDTYPE_t;

  typedef struct CAEN_V1290_Config_t {
    unsigned int baseAddress;

    CAEN_V1290_Model_t model;

    bool triggerTimeSubtraction;
    bool triggerMatchMode;
    bool emptyEventEnable;

    unsigned int edgeDetectionMode;
    unsigned int enabledChannels;
    unsigned int recordTriggerTimeStamp;

    unsigned int maxHitsPerEvent;

    unsigned int timeResolution;

    unsigned int windowWidth;
    int windowOffset;

  } CAEN_V1290_Config_t;

  CAEN_V1290(int ID): handle_(-1) {
    type_ = "CAEN_V1290"; id_ = ID; _isConfigured = false; srand(0);
  };

  inline unsigned int GetId() {return id_;};
  inline void SetId(unsigned int id) {id_ = id;};
  // --- GetType
  inline std::string GetType() const { return type_;}
  // --- Configurable


  virtual bool Init();
  virtual int SetupModule();
  virtual int Clear();
  virtual int ClearBusy() {return 0;};
  virtual int BufferClear(); //reset the buffers
  virtual int Print() { return 0; }
  //virtual int Config(BoardConfig *bC);
  virtual int Config(CAEN_V1290::CAEN_V1290_Config_t &);
  virtual bool IsConfigured() const {return _isConfigured;};
  virtual bool DataReady();
  virtual int Read(std::vector<WORD> &v);
  virtual int SetHandle(int handle) { handle_ = handle; return 0;};
  virtual int GetHandle() { return handle_;};

  inline CAEN_V1290_Config_t* GetConfiguration() { return &configuration_; };

  virtual void generatePseudoData(unsigned int eventNr, std::vector<WORD> &data);

  int EnableTDCTestMode(WORD testData);
  int DisableTDCTestMode();
  int SoftwareTrigger();

  int CheckStatusAfterRead();
private:
  int OpWriteTDC(WORD data);
  int OpReadTDC(WORD* data);

  bool _isConfigured;

  uint32_t handle_;
  CAEN_V1290_Config_t configuration_;
  uint32_t channels_;


protected:
  unsigned int id_;
  std::string type_;
};



#endif

