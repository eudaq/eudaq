//implementation adapted from: https://github.com/cmsromadaq/H4DAQ/blob/master/interface/CAEN_V1742.hpp
//access: 01 August 2018

#ifndef CAEN_V1742_H
#define CAEN_V1742_H

#include <string>
#include <vector>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>    //necessary for memcopy
#include <cmath>
#include <cstdlib>
#include <sstream>
#include <bitset>

#include "eudaq/Logger.hh"
#include "eudaq/Utils.hh"

#include "CAENVMElib.h"
#include "CAENVMEtypes.h"
#include "CAENVMEoslib.h"
#include "CAENComm.h"
#include <CAENDigitizer.h>

typedef uint32_t WORD;

//PG settings for the board


#define CAEN_V1742_MAXCH  64          /* max. number of channels */
#define CAEN_V1742_MAXSET 8           /* max. number of independent settings */
#define CAEN_V1742_MAXGW  1000        /* max. number of generic write commads */

#define CAEN_V1742_VME_INTERRUPT_LEVEL      1
#define CAEN_V1742_VME_INTERRUPT_STATUS_ID  0xAAAA

#define CAEN_V1742_INTERRUPT_MODE           CAEN_DGTZ_IRQ_MODE_ROAK
#define CAEN_V1742_INTERRUPT_TIMEOUT        200  // ms

#define CAEN_V17XX_READOUTSTATUS_REGISTER   0xEF04

class CAEN_V1742 {

public:

  typedef enum  {
    ERR_NONE = 0,
    ERR_CONF_NOT_FOUND,
    ERR_CONF_INVALID,
    ERR_DGZ_OPEN,
    ERR_BOARD_INFO_READ,
    ERR_INVALID_BOARD_TYPE,
    ERR_DGZ_PROGRAM,
    ERR_MALLOC,
    ERR_RESTART,
    ERR_INTERRUPT,
    ERR_READOUT,
    ERR_READOUT_TIMEOUT,
    ERR_EVENT_BUILD,
    ERR_CLEARBUFFER,
    ERR_UNHANDLED_BOARD,
    ERR_MISMATCH_EVENTS,
    ERR_FREE_BUFFER,
    ERR_STATUS_READOUT,
    ERR_DUMMY_LAST,
  } ERROR_CODES ;


  typedef struct CAEN_V1742_Config_t {

    unsigned int BaseAddress ;
    CAEN_DGTZ_ConnectionType LinkType ;
    int LinkNum ;
    int ConetNode ;

    int Nch ;
    int Nbit ;
    float Ts ;

    int RecordLength ;
    CAEN_DGTZ_DRS4Frequency_t DRS4Frequency;

    unsigned int PostTrigger ;


    int NumEvents ;
    int InterruptNumEvents ;
    int TestPattern ;
    int DesMode ;
    int TriggerEdge ;
    int FPIOtype ;

    CAEN_DGTZ_TriggerMode_t ExtTriggerMode ;

    int ChannelEnableMask ;
    int GroupEnableMask ;

    CAEN_DGTZ_TriggerMode_t ChannelTriggerMode[CAEN_V1742_MAXSET] ;

    uint32_t DCoffset[CAEN_V1742_MAXSET] ;
    int32_t  DCoffsetGrpCh[CAEN_V1742_MAXSET][CAEN_V1742_MAXCH] ;
    uint32_t Threshold[CAEN_V1742_MAXSET] ;
    uint8_t GroupTrgEnableMask[CAEN_V1742_MAXSET] ;

    uint32_t FTDCoffset[CAEN_V1742_MAXSET] ;
    uint32_t FTThreshold[CAEN_V1742_MAXSET] ;

    CAEN_DGTZ_TriggerMode_t FastTriggerMode ;
    uint32_t   FastTriggerEnabled ;

    int GWn ;
    uint32_t GWaddr[CAEN_V1742_MAXGW] ;
    uint32_t GWdata[CAEN_V1742_MAXGW] ;

    int useCorrections ;

  } CAEN_V1742_Config_t ;

  CAEN_V1742 (int ID): digitizerHandle_ (-1) {
    buffer_ = NULL; eventPtr_ = NULL, eventV1742_ = NULL; eventV1720_ = NULL; type_ = "CAEN_V1742" ; id_ = ID; _isConfigured = false; _isInitialized = false;
  };

  inline unsigned int GetId() {return id_;};
  inline void SetId(unsigned int id) {id_ = id;};
  // --- GetType
  inline std::string GetType() const { return type_;}
  // --- Configurable

  virtual int Init () ;
  virtual int SetupModule ();
  virtual int StartAcquisition();
  virtual int StopAcquisition();
  virtual int Clear () ;
  virtual int BufferClear () ;
  virtual int DataReady() ;
  virtual int ClearBusy () ;
  // --- Actually the size in bit of int is 16/32 on 32 bit and 64 on 64bit machines
  virtual int Read (std::vector<WORD> &v) ;
  virtual int SetHandle (int digitizerHandle) { digitizerHandle_ = digitizerHandle ; return 0 ;} ;

  int Config(CAEN_V1742_Config_t &_config);
  inline CAEN_V1742_Config_t* GetConfiguration () { return &digitizerConfiguration_ ; } ;
  int Print () {Print (0) ; } ;
  int Print (int full = 0) ;
  int setDefaults () ;

  int generateFakeData (int, std::vector<WORD>&);

private:

  int getMoreBoardInfo () ;
  int programDigitizer () ;
  int writeEventToOutputBuffer (std::vector<WORD>& CAEN_V1742_rawDataBinaryBuffer, CAEN_DGTZ_EventInfo_t* eventInfo, CAEN_DGTZ_X742_EVENT_t* event) ;
  int writeEventToOutputBuffer (std::vector<WORD>& CAEN_V1742_rawDataBinaryBuffer, CAEN_DGTZ_EventInfo_t* eventInfo, CAEN_DGTZ_UINT16_EVENT_t* event) ;
  /* sets the default values for the cfg parameters */


private:
  int              digitizerHandle_ ;
  CAEN_V1742_Config_t   digitizerConfiguration_ ;
  CAEN_DGTZ_BoardInfo_t boardInfo_ ;

  //Memory buffers for DR
  char *buffer_;
  char *eventPtr_;
  CAEN_DGTZ_X742_EVENT_t * eventV1742_;
  CAEN_DGTZ_UINT16_EVENT_t * eventV1720_; //extra event type for first attempts in the lab with the V1720, it does not have groups, 16 August 2018, T.Q.

  std::ostringstream s;
  CAEN_DGTZ_ErrorCode ret;
  ERROR_CODES ErrCode;

protected:
  unsigned int id_;
  std::string type_;
  bool _isConfigured;
  int _isInitialized;

};



#endif

