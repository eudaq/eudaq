
// *********************************************************
// IRC DLL header file for importing functions
// GC - 29/04/2016
// *********************************************************


// ------------------------------------------
// General constants
// ------------------------------------------

#define GLB_FILE_PATH_SZ 256

// ------------------------------------------
// Errors & messages log lib constants
// ------------------------------------------

#define ERR_LOG_LVL_NONE             0
#define ERR_LOG_LVL_ALL              1
#define ERR_LOG_LVL_WARINGS_ERRORS   2
#define ERR_LOG_LVL_WARNINGS_ERRORS  2
#define ERR_LOG_LVL_ERRORS           3

#include <inttypes.h> 

// ------------------------------------------
// General types definition
// ------------------------------------------


#ifndef UInt8
  typedef unsigned char UInt8;
#endif

#ifndef UByte
  typedef UInt8 UByte;
#endif

#ifndef SInt8
  typedef char SInt8;
#endif

#ifndef SByte
  typedef SInt8 SByte;
#endif

#ifndef UInt16
  typedef unsigned short UInt16;
#endif

#ifndef UWord
  typedef UInt16 UWord;
#endif

#ifndef SInt16
  typedef short SInt16;
#endif

#ifndef SWord
  typedef SInt16 SWord;
#endif

#ifndef UInt32
  typedef unsigned long int UInt32;
#endif

#ifndef ULong
  typedef UInt32 ULong;
#endif

#ifndef SInt32
  typedef long int SInt32;
#endif

#ifndef SLong
  typedef SInt32 SLong;
#endif


#ifndef WIN32

  #ifndef UInt64
    typedef uint64_t UInt64;
  #endif

  #ifndef SInt64
    typedef int64_t SInt64;
  #endif

#else

  #ifndef UInt64
    typedef uint64_t UInt64;
  #endif

  #ifndef SInt64
    typedef int64_t SInt64;
  #endif

#endif


/* Pointeurs */

typedef char* TPChar;

typedef UInt8* TPUInt8;
typedef TPUInt8 TPUByte;

typedef SInt8* TPSInt8;
typedef TPSInt8 TPSByte;

typedef UInt16* TPUInt16;
typedef TPUInt16 TPUWord;

typedef SInt16* TPSInt16;
typedef TPSInt16 TPSWord;

typedef UInt32* TPUInt32;
typedef TPUInt32 TPULong;

typedef SInt32* TPSInt32;
typedef TPSInt32 TPSLong;

/* ROOT !
typedef Real32* TPReal32;
typedef Real64* TPReal64;
typedef Real80* TPReal80;
*/



// ------------------------------------------
// Types definition for IRC
// ------------------------------------------


// 14/04/2015
// Record for cmd RunConf

typedef struct {
  
  // Since 07/04/2016
  // In order to allow for easier interface / LV clusters if one day implemented ...
  // - All numeric fields are S32
  // - String fields at end of record
  // nevertheless, the old method with one fuction per field will be used now.
  
  SInt32 CmdId;
  SInt32 CmdSubId;
  SInt32 ParamS32;
  
  SInt32 MapsName;
  SInt32 MapsNb;
  SInt32 RunNo;
  SInt32 TotEvNb;
  SInt32 EvNbPerFile;
  SInt32 FrameNbPerAcq;
  SInt32 DataTransferMode;
  SInt32 TrigMode;
  SInt32 SaveToDisk;
  SInt32 SendOnEth;
  SInt32 SendOnEthPCent;
  char   DestDir[GLB_FILE_PATH_SZ];
  char   FileNamePrefix[GLB_FILE_PATH_SZ];
  char   JtagFileName[GLB_FILE_PATH_SZ];
  
  
  /* Before 07/04/2016
  
  SInt32 CmdId;
  SInt32 CmdSubId;
  SInt32 ParamS32;
  
  SInt16 MapsName;
  SInt16 MapsNb;
  SInt32 RunNo;
  SInt32 TotEvNb;
  SInt32 EvNbPerFile;
  SInt32 FrameNbPerAcq;
  SInt8  DataTransferMode;
  SInt8  TrigMode;
  char   DestDir[GLB_FILE_PATH_SZ];
  char   FileNamePrefix[GLB_FILE_PATH_SZ];
  SInt8  SaveToDisk;
  SInt8  SendOnEth;
  SInt8  SendOnEthPCent;
  char   JtagFileName[GLB_FILE_PATH_SZ];
  
  */
  
} IRC_RCBT2628__TCmdRunConf;





// ---------------------------------
// Mi26/28 run control RC side
// ---------------------------------


//extern "C" __declspec(dllimport) SInt32 IRC__FBegin ( SInt8 ErrorLogLvl, char* ErrorLogFile, SInt8 MsgLogLvl, char* MsgLogFile ); // declspec doesnt  seenm to needed
extern "C" SInt32 IRC__FBegin ( SInt8 ErrorLogLvl, char* ErrorLogFile, SInt8 MsgLogLvl, char* MsgLogFile );
extern "C" SInt32 IRC_RCBT2628__FRcEnd ();

extern "C" SInt32 IRC_RCBT2628__FRcBegin ();
extern "C" SInt32 IRC_RCBT2628__FRcEnd   ();


extern "C" SInt32 IRC_RCBT2628__FRcSendCmdGetLastCmdError  ( SInt32 ParamS32, SInt32* PtAns0S32, SInt32* PtAns1S32, SInt32* PtAnsLastCmdError, SInt32 TimeOutMs );

extern "C" SInt32 IRC_RCBT2628__FRcSendCmdInit             ( SInt32 ParamS32, SInt32* PtAns0S32, SInt32* PtAns1S32, SInt32 TimeOutMs );

extern "C" SInt32 IRC_RCBT2628__FRcSendCmdEnd              ( SInt32 ParamS32, SInt32* PtAns0S32, SInt32* PtAns1S32, SInt32 TimeOutMs );

extern "C" SInt32 IRC_RCBT2628__FRcSendCmdAppQuit          ( SInt32 ParamS32, SInt32* PtAns0S32, SInt32* PtAns1S32, SInt32 TimeOutMs );

extern "C" SInt32 IRC_RCBT2628__FRcSendCmdFwLoad           ( SInt32 ParamS32, SInt32* PtAns0S32, SInt32* PtAns1S32, SInt32 TimeOutMs );

extern "C" SInt32 IRC_RCBT2628__FRcSendCmdFwUnload         ( SInt32 ParamS32, SInt32* PtAns0S32, SInt32* PtAns1S32, SInt32 TimeOutMs );

extern "C" SInt32 IRC_RCBT2628__FRcSendCmdRunStartStop     ( SInt32 ParS32StopStart /* 0 = Stop, 1 = Start */, SInt32* PtAns0S32, SInt32* PtAns1S32, SInt32 TimeOutMs );

extern "C" SInt32 IRC_RCBT2628__FRcSendCmdStatusAcqNb      ( SInt32 ParamS32, SInt32* PtAns0S32, SInt32* PtAns1S32, SInt32 TimeOutMs );

extern "C" SInt32 IRC_RCBT2628__FRcSendCmdJtagLoad         ( SInt32 ParamS32, char* ParamJtagFile, SInt32* PtAns0S32, SInt32* PtAns1S32, SInt32 TimeOutMs );

extern "C" SInt32 IRC_RCBT2628__FRcSendCmdJtagReset        ( SInt32 ParamS32, SInt32* PtAns0S32, SInt32* PtAns1S32, SInt32 TimeOutMs );

extern "C" SInt32 IRC_RCBT2628__FRcSendCmdJtagStart        ( SInt32 ParamS32, SInt32* PtAns0S32, SInt32* PtAns1S32, SInt32 TimeOutMs );

extern "C" SInt32 IRC_RCBT2628__FRcSendCmdRunConf          ( SInt32 ParamS32, IRC_RCBT2628__TCmdRunConf* PtParamRunConf, SInt32* PtAns0S32, SInt32* PtAns1S32, SInt32 TimeOutMs );

extern "C" SInt32 IRC_RCBT2628__FRcGetLastCmdError         ( SInt32* PtLastCmdError, SInt32 TimeOutMs );




