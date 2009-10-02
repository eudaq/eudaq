
/*******************************************************************************
File      : x:\lib\com\maps\mi26\mi26.typ
Goal      : Types definition of Mi26 library.
          : It provides Mi26 types definition and data handling functions.
Prj date  : 24/11/2008
File date : 24/11/2008
Doc date  : //200
Author    : Gilles CLAUS
E-mail    : gilles.claus@ires.in2p3.fr
----------------------------------------------------------------------------------
License   : You are free to use this source files for your own development as long
          : as it stays in a public research context. You are not allowed to use it
          : for commercial purpose. You must put this header with laboratory and
          : authors names in all development based on this library.
----------------------------------------------------------------------------------
Labo      : IPHC */
/*******************************************************************************/


#ifndef MI26_TYP
#define MI26_TYP

#ifdef ROOT_ROOT
  typedef UInt32 MI26__TColor;
  #define  clWhite 0
  #define  clBlack 0
  #define  clBlue  0
  #define  clRed 0  
#else  
  typedef TColor MI26__TColor;
#endif





/* ============================= */
/*  Lib context                  */
/*  Contain all global variables */
/* ----------------------------- */
/* Date : 24/11/2008             */
/* ============================= */

typedef struct {

  SInt8 FileErrLogLvl;
  char  FileErrFile[GLB_FILE_PATH_SZ];

} MI26__TContext;

/* ============================================================================ */
/*  Discri register, 2 views                                                    */
/*  - Array of W32 words                                                        */
/*  - Array of bits                                                             */
/* ---------------------------------------------------------------------------- */
/* WARNING : It is not a bit mapping but conversion must be done by a function  */
/* ---------------------------------------------------------------------------- */
/* Date : 24/11/2008                                                            */
/* ============================================================================ */

typedef struct {

  UInt32 ADataW32[MI26__REG_DISCRI_W32_SZ];
  SInt8  ADataBit[MI26__REG_DISCRI_BIT_SZ];

} MI26__TRegDiscri;

/* ============================================================================ */
/*  Discri register as array of W32                                             */
/* ---------------------------------------------------------------------------- */
/* Date : 25/11/2008                                                            */
/* ============================================================================ */

typedef struct {

  UInt32 AW32[MI26__REG_DISCRI_W32_SZ];

} MI26__TRegDiscriW32;


/* ============================================================================ */
/*  Discri register as array of bit                                             */
/* ---------------------------------------------------------------------------- */
/* Date : 25/11/2008                                                            */
/* ============================================================================ */

typedef struct {

  SInt8  ABit[MI26__REG_DISCRI_BIT_SZ];

} MI26__TRegDiscriBit;


/* ============================================================================ */
/*  Discri matrix dual view : W32 or Bit                                        */
/* ---------------------------------------------------------------------------- */
/* Date : 24/11/2008                                                            */
/* ============================================================================ */

typedef struct {

  MI26__TRegDiscri ALine[MI26__MAT_DISCRI_LINES_NB];

} MI26__TMatDiscri;

/* ============================================================================ */
/*  Discri matrix view as W32 array                                             */
/* ---------------------------------------------------------------------------- */
/* Date : 25/11/2008                                                            */
/* ============================================================================ */

typedef struct {

  UInt32 AALineW32[MI26__MAT_DISCRI_LINES_NB][MI26__REG_DISCRI_W32_SZ];

} MI26__TMatDiscriW32;

/* ============================================================================ */
/*  Discri matrix view as bit array                                             */
/* ---------------------------------------------------------------------------- */
/* Date : 25/11/2008                                                            */
/* ============================================================================ */

typedef struct {

  SInt8 AALineCol[MI26__MAT_DISCRI_LINES_NB][MI26__REG_DISCRI_BIT_SZ];

} MI26__TMatDiscriBit;


typedef struct {

  float AALineCol[MI26__MAT_DISCRI_LINES_NB][MI26__REG_DISCRI_BIT_SZ];

} MI26__TMatDiscriBitf;


/* ============================================================================ */
/*  Discri matrix view as bit array - BUT scale 1/4                             */
/* ---------------------------------------------------------------------------- */
/* Date : 23/07/2009                                                            */
/* ============================================================================ */

typedef struct {
  
  SInt8 AALineCol[MI26__MAT_DISCRI_LINES_NB / 2][MI26__REG_DISCRI_BIT_SZ / 2];
  
} MI26__TMatDiscriBitHalfScale;



/* ============================================================================ */
/*  After Zero Sup register, 2 views                                            */
/*  - Array of W32 words                                                        */
/*  - Array of bits                                                             */
/* ---------------------------------------------------------------------------- */
/* WARNING : It is not a bit mapping but conversion must be done by a function  */
/* ---------------------------------------------------------------------------- */
/* Date : 24/11/2008                                                            */
/* ============================================================================ */

typedef struct {

  UInt32 ADataW32[MI26__REG_AFTER_ZS_W32_SZ];
  SInt8  ADataBit[MI26__REG_AFTER_ZS_BIT_SZ];

} MI26__TRegAfterZs;

/* ============================================================================ */
/*  After Zero Sup register as array of W32                                     */
/* ---------------------------------------------------------------------------- */
/* Date : 25/11/2008                                                            */
/* ============================================================================ */

typedef struct {

  UInt32 AW32[MI26__REG_AFTER_ZS_W32_SZ];

} MI26__TRegAfterZsW32;

/* ============================================================================ */
/*  After Zero Sup register as array of bit                                     */
/* ---------------------------------------------------------------------------- */
/* Date : 25/11/2008                                                            */
/* ============================================================================ */

typedef struct {

  SInt8  ABit[MI26__REG_AFTER_ZS_BIT_SZ];

} MI26__TRegAfterZsBit;


/* ============================================================================ */
/*  After Mux register, 2 views                                                 */
/*  - Array of W32 words                                                        */
/*  - Array of bits                                                             */
/* ---------------------------------------------------------------------------- */
/* WARNING : It is not a bit mapping but conversion must be done by a function  */
/* ---------------------------------------------------------------------------- */
/* Date : 24/11/2008                                                            */
/* ============================================================================ */

typedef struct {

  UInt32 ADataW32[MI26__REG_AFTER_MUX_W32_SZ];
  SInt8  ADataBit[MI26__REG_AFTER_MUX_BIT_SZ];

} MI26__TRegAfterMux;

/* ============================================================================ */
/*  After Mux register as array of W32                                          */
/* ---------------------------------------------------------------------------- */
/* Date : 25/11/2008                                                            */
/* ============================================================================ */

typedef struct {

  UInt32 AW32[MI26__REG_AFTER_MUX_W32_SZ];

} MI26__TRegAfterMuxW32;


/* ============================================================================ */
/*  After Mux register as array of bit                                          */
/* ---------------------------------------------------------------------------- */
/* Date : 25/11/2008                                                            */
/* ============================================================================ */

typedef struct {

  SInt8  ABit[MI26__REG_AFTER_MUX_BIT_SZ];

} MI26__TRegAfterMuxBit;



/* ======================================================= */
/* Frame provided by Mi26 DAQ, it's independent of output  */
/* mode BUT data are not organized as in MI26__TZsFFrame   */
/* The format is :                                         */
/* - Header                                                */
/* - Frames counter                                        */
/* - Data length = W16 number of useful data ( excluding   */
/*   trailer and bits at zero at end of frame )            */
/* - Array of W16 data                                     */
/* ------------------------------------------------------- */
/* This is a FIXED size record which contains the maximum  */
/* possible W16 defined by MI26__ZS_FFRAME_RAW_MAX_W16     */
/* ------------------------------------------------------- */
/* Date : 08/12/2008                                       */
/* ======================================================= */


typedef struct {

  ASIC__TFrameStatus SStatus;
  
  UInt32 Header;
  UInt32 FrameCnt;
  UInt32 DataLength;
  
  UInt32 Trailer;
  UInt32 Zero;
  UInt32 Zero2;
  
  
  UInt16 ADataW16[MI26__ZS_FFRAME_RAW_MAX_W16]; // MUST BE AT END OF RECORD !
  
//  SInt32 a;
//  char   Text[255];
  
} __TZsFFrameRaw;





typedef struct {

  ASIC__TFrameStatus SStatus;

  UInt32 Header;
  UInt32 FrameCnt;
  UInt32 DataLength;

  UInt32 Trailer;
  UInt32 Zero;
  UInt32 Zero2;


  UInt16 ADataW16[MI26__ZS_FFRAME_RAW_MAX_W16]; // MUST BE AT END OF RECORD !

} MI26__TZsFFrameRaw; // F in FFrameRaw means Fixed size frame

 

/* =================================================== */
/*  Field States/Line of Zero Sup frame, 2 views       */
/*  - W16 word                                         */
/*  - Fields                                           */
/* --------------------------------------------------- */
/* It's bit mapping => No conversion function required */
/* --------------------------------------------------- */
/* Date : 24/11/2008                                   */
/* =================================================== */

typedef union {

  UInt16 W16;

  struct {

    UInt16 StateNb  :  4;
    UInt16 LineAddr : 11;
    UInt16 Ovf      :  1;

  } F;

} MI26__TStatesLine;

/* =================================================== */
/*  Field State of Zero Sup frame, 2 views             */
/*  - W16 word                                         */
/*  - Fields                                           */
/* --------------------------------------------------- */
/* It's bit mapping => No conversion function required */
/* --------------------------------------------------- */
/* Date : 24/11/2008                                   */
/* =================================================== */

typedef union {

  UInt16 W16;

  struct {

    UInt16 HitNb   :  2;
    UInt16 ColAddr : 11;
    UInt16 NotUsed :  3;

  } F;

} MI26__TState;


/* ======================================================= */
/* One list of states associated to one line               */
/* - States/Lines information                              */
/* - States list                                           */
/* ------------------------------------------------------- */
/* This is a FIXED size record which contains all states   */
/* of one line, max MI26__ZS_FFRAME_MAX_STATES_NB_PER_LINE */
/* ------------------------------------------------------- */
/* Date : 24/11/2008                                       */
/* ======================================================= */

typedef struct {

  MI26__TStatesLine StatesLine;
  MI26__TState      AStates[MI26__ZS_FFRAME_MAX_STATES_NB_PER_STATES_REC];

} MI26__TZsFStatesRec; // F in FStatesRec means Fixed size record


/* ======================================================= */
/* Frame provided by Mi26, this is the final result after  */
/* data processing depending of output mode selected       */
/* - Header                                                */
/* - Frames counter                                        */
/* - Data length = W16 number of useful data ( excluding   */
/*   trailer and bits at zero at enbd of frame )           */
/* ------------------------------------------------------- */
/* This is a FIXED size record which contains all states   */
/* of one line, max MI26__ZS_FFRAME_MAX_STATES_NB_PER_LINE */
/* ------------------------------------------------------- */
/* Date : 24/11/2008                                       */
/* ======================================================= */

typedef struct {

  UInt32 Header;
  UInt32 FrameCnt;
  UInt32 DataLength;
  SInt16 TrigSignalLine;
  SInt8  TrigSignalClk;
  SInt16 TrigLine;
  
  UInt32 StatesRecNb; // It's NOT a MI26 frame field, it's calculated by sw
                      // It's the number of valid record in AStatesRec

  MI26__TZsFStatesRec AStatesRec[MI26__ZS_FFRAME_MAX_STATES_REC];

  UInt32 Trailer;
  UInt32 Zero;
  UInt32 Zero2;

} MI26__TZsFFrame; // F in FFrame means Fixed size frame




#ifndef APP__RMV_MI26__TCDiscriFile
#ifndef APP__RMV_CLASSES

// 31/01/2009

class MI26__TCDiscriFile : public FIL__TCBinFile {


  private :

  protected :


  public :

    MI26__TCDiscriFile ( char* ErrLogFile, SInt8 EnableErrLog, SInt8 ErrLogLvl );
    ~MI26__TCDiscriFile ();

    SInt32                 PubFConf  ( char* DataFile, SInt8 WriteRead, SInt8 FlushAfterWrite, SInt8 MeasTime );

    SInt32                 PubFSetFileName  ( char* DataFile );
    SInt32                 PubFSetFlushMode ( SInt8 FlushAfterWrite );

    SInt32                 PubFGetFileSz  ();
    SInt32                 PubFGetEvNb  ();

    SInt32                 PubFCreate ();
    SInt32                 PubFOpen ();

    SInt32                 PubFWriteEv      ( MI26__TMatDiscriW32* PtSrcMat  );
    SInt32                 PubFReadNextEv   ( MI26__TMatDiscriW32* PtDestMat, SInt32 MaxDestSz );
    MI26__TMatDiscriW32*   PubFReadNextEv   ();
    SInt32                 PubFGotoEv       ( SInt32 EvNo );
    SInt32                 PubFReadEv       ( SInt32 EvNo, MI26__TMatDiscriW32* PtDestMat, SInt32 MaxDestSz );
    MI26__TMatDiscriW32*   PubFReadEv       ( SInt32 EvNo );

    SInt32                 PubFFlush ();
    SInt32                 PubFClose ();

};

#endif
#endif

/* ==================================================== */
/* Discriminators test information file record          */
/*                                                      */
/* - Test no                                            */
/* - Chip no / comment                                  */
/* - Number of steps                                    */
/* - Events nb / step                                   */
/* - Run no associated to each step                     */
/* - Bias used for each step                            */
/* - User defined parameters for each step              */
/*   The can be used to convert udac / mv etc ...       */
/*                                                      */
/* ---------------------------------------------------- */
/* The file discri_test_NNNN.cnf contains the structure */
/* MI26__TDisTestCnfFile                                */
/* ---------------------------------------------------- */
/* Date : 10/02/2009                                    */
/* ==================================================== */

// A copy of bias used for the current run
// 10/02/2009

typedef struct {

  UInt8 ABias[MI26__REG_BIAS_NB];

} MI26__TBiasCnf;


// All information about current discri measurement step
// 10/02/2009

typedef struct {

  SInt32           RunNo;
  MI26__TBiasCnf   Bias;
  float            APar[MI26__DIS_MEAS_STEP_MAX_PAR_NB];

} MI26__TDisMeasStep;


// Format of file discri_test_NNNN.cnf
// 10/02/2009

/*typedef struct {

  SInt32 TestNo;                  // No of test
  char   ChipNoCmt[GLB_CMT_SZ];   // Comment about mimosa : name, number ...
  SInt32 StepNb;                  // Number of threshold steps used to perform this test = number of run
  SInt32 EvNbPerStep;             // Number of events per step = nb of events per run

  // Information about each step
  // - No of run
  // - Bias values used for this run
  // - An array of float user parameters, up to MI26__DIS_MEAS_STEP_MAX_PAR_NB
  //   Parameters can be used to store threshold in mv or convertion ratio udac / mv etc ...

} MI26__TDisMeasHeader;

typedef struct {
 MI26__TDisMeasHeader Head;
 MI26__TDisMeasStep AStep[MI26__DIS_TEST_MAX_STEP_NB];
}MI26__TDisTestCnfFile;*/


typedef struct {

  SInt32 TestNo;                  // No of test
  char   ChipNoCmt[GLB_CMT_SZ];   // Comment about mimosa : name, number ...
  SInt32 StepNb;                  // Number of threshold steps used to perform this test = number of run
  SInt32 EvNbPerStep;             // Number of events per step = nb of events per run

  // Information about each step
  // - No of run
  // - Bias values used for this run
  // - An array of float user parameters, up to MI26__DIS_MEAS_STEP_MAX_PAR_NB
  //   Parameters can be used to store threshold in mv or convertion ratio udac / mv etc ...

  MI26__TDisMeasStep AStep[MI26__DIS_TEST_MAX_STEP_NB];


} MI26__TDisTestCnfFile;



/* ==================================================== */
/* Discriminators data processing output file record    */
/* in sub matrices view mode => sub A, B, C, D          */
/*                                                      */
/* - Header                                             */
/* -- No of test                                        */
/* -- Step number = thresholds number                   */
/* -- Path of config file to get more informations      */
/*    Config file is discri_test_NNNN.cnf               */
/*                                                      */
/* - Data                                               */
/*   It's a 3D array which contains hit count in % for  */
/*   each pixel. It's ORGANIZED per submatrix.          */
/*   => Header.SubMatView = 1                           */
/* ---------------------------------------------------- */
/* The file discri_test_NNNN.dat contains the structure */
/* MI26__TDisHitCntAllSubMatFile                        */
/* ---------------------------------------------------- */
/* WARNING : This structure AS IS allows only to access */
/* to file header and FIRST test data.                  */
/* In order to acces to next test data :                */
/* - init a BYTE pointer VPtB to FirstStepData addr     */
/* - increment with size of MI26__TDisHitCntAllSubMatStepData */
/* - init a MI26__TDisHitCntStepData pointer to VPtB    */
/* - and so on for next step ...                        */
/*                                                      */
/* ---------------------------------------------------- */
/* Date : 10/02/2009                                    */
/* ==================================================== */


typedef struct {

  SInt32 TestNo;
  SInt8  SubMatView; // 0 => whole matrix as single data piece
                     // 1 => submatrices A, B, C, D
  SInt32 StepNb;
  char   CnfFileName[GLB_FILE_PATH_SZ];


} MI26__TDisHitCntHeader;


typedef struct {

  float AAASubMat[MI26__SUB_MAT_NB][MI26__MAT_DISCRI_LINES_NB][MI26__REG_DISCRI_BIT_SZ / 4];

} MI26__TDisHitCntAllSubMatStepData;

typedef struct {

  float AAAASubMat [MI26__DIS_TEST_MAX_STEP_NB][MI26__SUB_MAT_NB][MI26__MAT_DISCRI_LINES_NB][MI26__REG_DISCRI_BIT_SZ / 4];

} MI26__TDisHitCntAllSubMatStepDataMG;

typedef struct {

  MI26__TDisHitCntHeader            Header;
  MI26__TDisHitCntAllSubMatStepData FirstStepData;

} MI26__TDisHitCntAllSubMatFile;


/* ==================================================== */
/* Discriminators data processing output file record    */
/* in whole matrix view mode                            */
/*                                                      */
/* - Header                                             */
/* -- No of test                                        */
/* -- Step number = thresholds number                   */
/* -- Path of config file to get more informations      */
/*    Config file is discri_test_NNNN.cnf               */
/*                                                      */
/* - Data                                               */
/*   It's a 2D array which contains hit count in % for  */
/*   each pixel. It's ORGANIZED as ONE matrix.          */
/*   => Header.SubMatView = 0                           */
/* ---------------------------------------------------- */
/* The file discri_test_NNNN.dat contains the structure */
/* MI26__TDisHitCntMatFile                              */
/* ---------------------------------------------------- */
/* WARNING : This structure AS IS allows only to access */
/* to file header and FIRST test data.                  */
/* In order to acces to next test data :                */
/* - init a BYTE pointer VPtB to FirstStepData addr     */
/* - increment with size of MI26__TDisHitCntMatStepData */
/* - init a MI26__TDisHitCntStepData pointer to VPtB    */
/* - and so on for next step ...                        */
/*                                                      */
/* ---------------------------------------------------- */
/* Date : 10/02/2009                                    */
/* ==================================================== */


typedef struct {

  float AAMat[MI26__MAT_DISCRI_LINES_NB][MI26__REG_DISCRI_BIT_SZ];

} MI26__TDisHitCntMatStepData;


typedef struct {

  MI26__TDisHitCntHeader      Header;
  MI26__TDisHitCntMatStepData FirstStepData;

} MI26__TDisHitCntMatFile;



typedef struct {

  SInt8 AALineCol[MI26__MAT_DISCRI_LINES_NB][MI26__REG_DISCRI_BIT_SZ / 4];

} MI26__TQuaterMatDiscriBit;

typedef struct {

  MI26__TColor AALineCol[MI26__MAT_DISCRI_LINES_NB][MI26__REG_DISCRI_BIT_SZ];

} MI26__TMatDiscriColor;


typedef struct {
  
  MI26__TColor AALineCol[MI26__MAT_DISCRI_LINES_NB / 2][MI26__REG_DISCRI_BIT_SZ / 2];
  
} MI26__TMatDiscriColorHalfScale;


typedef struct {

  MI26__TColor AALineCol[MI26__MAT_DISCRI_LINES_NB][MI26__REG_DISCRI_BIT_SZ / 4];

} MI26__TQuaterMatDiscriColor;


// To store "1" count on each discri for N events

typedef struct {

  SInt32  ABit[MI26__REG_DISCRI_BIT_SZ];

} MI26__TRegDiscriCumul;


// To store % of "1" on each discri for N events

typedef struct {

  float  ABit[MI26__REG_DISCRI_BIT_SZ];

} MI26__TRegDiscriPerCent;

// To store "1" count on whole matrix for N events

typedef struct {

  UInt16 AALineCol[MI26__MAT_DISCRI_LINES_NB][MI26__REG_DISCRI_BIT_SZ];

} MI26__TMatDiscriCumul;


typedef struct {
  
  UInt16 AALineCol[MI26__MAT_DISCRI_LINES_NB / 2][MI26__REG_DISCRI_BIT_SZ / 2];
  
} MI26__TMatDiscriCumulHalfScale;


// To store % of "1" on whole matrix for N events

typedef struct {

  float AALineCol[MI26__MAT_DISCRI_LINES_NB][MI26__REG_DISCRI_BIT_SZ];

} MI26__TMatDiscriPerCent;


typedef struct {

  float AALineCol[MI26__MAT_DISCRI_LINES_NB][MI26__REG_DISCRI_BIT_SZ];

} MI26__TQuaterMatDiscriPerCent;



/* ============================================================================ */
/*                                                     */
/*                                                          */
/*                                                              */
/* ---------------------------------------------------------------------------- */
/*   */
/* ---------------------------------------------------------------------------- */
/* Date :                                                             */
/* ============================================================================ */


// DPXI__MI26_NB_MAX_MI26_PER_DAQ

typedef MI26__TZsFFrame    MI26__TAZsFFrame[MI26__NB_MAX_MI26_PER_DAQ];



typedef struct {
  
  // Date & Time
  
  UInt32 StartDate;            //    DD-MM-YY =           D23D16 - D15D08 - D07D00
  UInt32 StartTime;            // HH-MM-SS-CC =  D31D24 - D23D16 - D15D08 - D07D00
  
  // Asic conf
  
  UInt8  AsicName;             // Value of enumerated ASIC__TEAsicName defined in asic.def
  SInt8  AsicNb;               // Number of Asic read by DAQ
  
  // Trigger mode
  
  SInt8  SwTrigEnabled;        // Enable automatic start of acquisition on first trigger of a spill
                               // - 0 => Board starts upon acquisition request command, don't care about trigger
                               // - 1 => After acquisition request command, board wait on first trigger to start
                               // Trigger detection is done by sw, therefore it has a latency of N x 15 ms
                                
  SInt8  HwTrigModeSavedData;  // Trigger mode
                               // - 0 => Run contains all frames : with OR without trigger
                               // - 1 => Run contains ONLY frames with a trigger 
  
  SInt32 HwTrigPar[MI26__TZSRunCnf__HW_TRIG_PAR_NB];
  
  // Run parameters
  
  SInt32 RunNo;
  SInt8  RunSave;
  SInt8  RunSaveMode;
  SInt32 RunEvNb;
  SInt32 RunFileEvNb;
  
} MI26__TZSRunCnf;


typedef struct {
  
  // Date & Time
  
  UInt32 StopDate; //    DD-MM-YY =           D23D16 - D15D08 - D07D00
  UInt32 StopTime; // HH-MM-SS-CC =  D31D24 - D23D16 - D15D08 - D07D00
  
  // Status
  
  SInt32 EvNbTaken;
  SInt32 RejAcqNb;
  SInt32 ARejAcqList[MI26__TZSRunRes__MAX_ACQ_REJ_NB];
  
} MI26__TZSRunRes;


// 09/07/2009

#ifndef APP__RMV_CLASSES

class MI26__TCZsRunRW {
  
  private:

    SInt32 PrivFInitForNewRunLoading ();

    // Object
    
    FIL__TCBinFile* ProPtBinFile;
    
  protected:
  
    // Parameters
  
    char   ProParRunDir[GLB_FILE_PATH_SZ];
    SInt32 ProParRunNo;
    SInt32 ProParCurMi26No;
    SInt32 ProParCurEvNo;
    
    SInt8  ProParMeasTime;
    SInt8  ProParPrintMsg;
    SInt8  ProParPrintEvHeader;
    
    // Informations from run conf file OR calculated
    
    char   ProRunCnfFile[GLB_FILE_PATH_SZ];
    char   ProRunResFile[GLB_FILE_PATH_SZ];
    
    MI26__TZSRunCnf ProRecZSRunCnf;
    MI26__TZSRunRes ProRecZSRunRes;
    
    SInt32 ProInfRunMi26Nb;
    SInt32 ProInfRunEvNb;
    SInt32 ProInfRunFileSz;
    SInt32 ProInfRunEvNbPerFile;
    SInt32 ProInfRunBlocNbPerFile;
    SInt32 ProInfZsFFrameRawSz;
    
    // Intermediate variables for processing
    
    SInt8  ProConfDone;
    SInt8  ProParEnableErrLog;
    SInt8  ProParErrLogLvl;
    
    char   ProParErrLogFile[GLB_FILE_PATH_SZ];
    
  
    SInt8  ProLastEvAccessDoneInOneMi26Mode;
    SInt32 ProCurBlocNoInRun;
    SInt32 ProCurFileNo;
    SInt32 ProCurBlocNoInFile;
    char   ProCurFileName[GLB_FILE_PATH_SZ];
    
    MI26__TZsFFrameRaw* ProPtFFrameRaw;


  public:
  
    MI26__TCZsRunRW ();
    ~MI26__TCZsRunRW ();
    
    SInt32              PubFBegin            ( char* ErrLogFile, SInt8 EnableErrLog, SInt8 ErrLogLvl );
    
    SInt32              PubFSetMeasTime      ( SInt8 Yes   );
    SInt32              PubFSetPrintMsg      ( SInt8 Print );
    SInt32              PubFSetPrintEvHeader ( SInt8 Print );
    
    SInt32              PubFGetMeasTime      ();
    SInt32              PubFGetPrintMsg      ();
    SInt32              PubFGetPrintEvHeader ();
    
    SInt32              PubFGetMi26Nb ();
    SInt32              PubFGetEvNb ();
    SInt32              PubFGetEvNbPerFile ();
    
    SInt32              PubFLoadRun    ( char* RunDir, UInt32 RunNo );    
    
    MI26__TZsFFrameRaw* PubFGotoEvOneMi26 ( SInt8 Mi26No, SInt32 EvNo );
    MI26__TZsFFrameRaw* PubFGotoEvAllMi26 (  SInt32 EvNo );
    
    SInt32              PubFCloseRun  ();
  
};

#endif

// 01/08/09

typedef struct {
  
  UInt32             IndexInEvWithHitList;
  ASIC__TFrameStatus FrStatus;
  
} MI26__TCTelMon_TEltListEvWithHitAllPlanes;


// 02/08/09
  
typedef struct {
  
  SInt16 x;
  SInt16 y;  
  
} MI26__TCTelMon_THit;

// 02/08/09

typedef struct {

  MI26__TCTelMon_THit AAHits[MAPS__TCDigTelMon_MAX_PLANE_NB][MAPS__TCDigTelMon_MAX_RES_RUN_HIT_NB_PER_PLANE];
  SInt32              AHitsNbPerPlane[MAPS__TCDigTelMon_MAX_PLANE_NB];
  
} MI26__TCTelMon_TTrack;

// 02/08/09

typedef struct {
  
  SInt8  Full;
  SInt8  PlaneNb;
  SInt32 EvNb;
  MI26__TCTelMon_TTrack ATracks[MAPS__TCDigTelMon_MAX_RES_RUN_EV_NB];
  
  
} MI26__TCTelMon_TResRun;


// 17/07/09

#ifndef ROOT_ROOT

#ifndef APP__RMV_CLASSES

class MI26__TCTelMon : public MAPS__TCDigTelMon {
  
  private:
    
    SInt8 PrivStopProc;
    
    // -------------------------------------------
    // Plot colors of planes for coincidence mode
    // -------------------------------------------
    
    MI26__TColor                   PrivAPlanePlotColor[MAPS__TCDigTelMon_MAX_PLANE_NB];
    
    // ------------------------------------------------------
    // Variables to store frames : pixel, cumul, plot color
    // ------------------------------------------------------
    
    MI26__TZsFFrameRaw*            PrivPtAcqData;             // Copy of Acq data, enabled if MakeLocalCpy parameter of PubFAddEvents (...) is set
                                                              // PubFAddEvents ( SInt8 MapsName, SInt8 MapsNb, SInt16 EvNb, MI26__TZsFFrameRaw* PtSrc, SInt8 MakeLocalCpy, SInt8 OffLineCall );
    
    SInt32                         PrivAcqEvNb;               // Event nb of current Acq = parameter EvNb of last PubFAddEvents (...) call
                                                              // PubFAddEvents ( SInt8 MapsName, SInt8 MapsNb, SInt16 EvNb, MI26__TZsFFrameRaw* PtSrc, SInt8 MakeLocalCpy, SInt8 OffLineCall );
                                                                                                                            
    
    MI26__TZsFFrame                PrivZsFFrame;              // Tmp var to store ZsFFrameRaw converted to ZsFFrame
                                                              // Used for each frame converted
    
    MI26__TMatDiscriBit            PrivMatDiscriCoin;         // Result for planeS coin plot AS PIXELS full scale matrix
    MI26__TMatDiscriCumul          PrivMatDiscriCoinCum;      // Result for planeS coin cumul plot AS COUNT full scale matrix
    
    MI26__TMatDiscriBitHalfScale   PrivMatDiscriBitHalfScale; // Result for planeS coin plot AS PIXELS 1/2 scale matrix
    MI26__TMatDiscriCumulHalfScale PrivMatDiscriCumHalfScale; // Result for planeS coin cumul plot AS COUNT 1/2 scale matrix
    
    MI26__TMatDiscriColor          PrivMatDispColor;          // Result in matrix color data for ALL KINDS of plots     
    MI26__TMatDiscriColorHalfScale PrivMatDispColorHalfScale; // Result in 1/2 scale matrix color data for ALL KINDS of plots
    
    // =====================================================================
    // Lists to store informations results of events processing
    // =====================================================================
    
    // Flag to enable / disable list update
    // It's useful to disable list update for data re-processing ( off-line ) with information of list
    // because list informations should not been updates while using them for current data processing ...
    //
    // The following methods set this flag to one 
    // - PubFProcOnlyEvWithHitOnAllPlanes ( SInt8 Yes )
    
    SInt8 PriEnListUpdate;
    
    // ---------------------------
    // List of events with trigger
    // ---------------------------
    
    SInt32 PrivListEvWithTrigIndex;  // Current elt index
    SInt8  PrivListEvWithTrigFull;   // Full flag
    
    // The list array ( dynamic allocation )
    // Rq : AA and only one dimention because dynamic allocation of each elt of this array is done in PrivFInit ()
    
    ASIC__TFrameStatus* PrivAAListEvWithTrig[MI26__TCTelMon__EV_LIST_MAX_CHAN_NB]; 
    
    // ---------------------------
    // List of events with hit(s)
    // ---------------------------
    
    SInt32 PrivListEvWithHitIndex;   // Current elt index
    SInt8  PrivListEvWithHitFull;    // Full flag

    // The list array ( dynamic allocation )
    // Rq : AA and only one dimention because dynamic allocation of each elt of this array is done in PrivFInit ()
    
    ASIC__TFrameStatus* PrivAAListEvWithHit[MI26__TCTelMon__EV_LIST_MAX_CHAN_NB]; // List
    
    // -----------------------------------------
    // List of events with hit(s) on all planes
    // -----------------------------------------
    
    SInt32 PrivListEvWithHitAllPlanesIndex;       // Currenr elt index
    SInt8  PrivListEvWithHitAllPlanesFull;        // Full flag
    SInt8  PrivListEvWithHitAllPlanesCurProcMode; // Current processing mode
                                                  // Uses to know if data must be reprocessed in cas user
                                                  // has changed list mode via ProPar.ListHitAllPlanesMode

    // The list array ( dynamic allocation )
    
    MI26__TCTelMon_TEltListEvWithHitAllPlanes* PrivAListEvWithHitAllPlanes;

    // ---------------------------------------------------------------------
    // Result run after data processing
    // ---------------------------------------------------------------------
    
    MI26__TCTelMon_TResRun* PrivPtResRun;
    
    // ---------------------------------------------------------------------
    // General init function : reset all variables, called by constructor
    // ---------------------------------------------------------------------
    
    SInt32 PrivFInit ();

    // ---------------------------------------------------------------------
    // Processing function
    // ---------------------------------------------------------------------
    
    SInt32 PrivFConvZsFFrameToMatDiscriBitAndCumul ( SInt32 DbgEvNo, SInt8 Mode, SInt8 PlaneNo, SInt8 PlaneSelForCoin, SInt8 EvNo, SInt8 EvSelForPlot, MI26__TZsFFrame* PtSrc, MI26__TMatDiscriBit* PtDestFrameBit, MI26__TMatDiscriBit* PtDestCoinBit,  MI26__TMatDiscriCumul* PtDestFrameCum, MI26__TMatDiscriCumul* PtDestCoinCum, SInt8 PrintLvl );
  

    // ---------------------------------------------------------------------
    // Lists ( events with trigger / hit ) add events functions
    // ---------------------------------------------------------------------
    
    SInt32 PrivFAddEvInListEvWithTrig ( SInt8 PlaneNo, ASIC__TFrameStatus* PtFrStatus, SInt32 HitCnt );
    SInt32 PrivFAddEvInListEvWithHit  ( SInt8 PlaneNo, ASIC__TFrameStatus* PtFrStatus, SInt32 HitCnt, SInt8 HitOnAllPlanes, SInt8 SingleHitOnEachPlane );
    
  protected:
  
    // -------------------------------------------
    // Data processing methods
    // -------------------------------------------
    
    // Process one frame = one event of plane specified by Id (0..5)
  
    SInt32 ProFProcessPlane   ( SInt32 DbgEvNo, SInt8 Id, MI26__TZsFFrameRaw* PtSrc );
    
      
    
  public:

    // --------------------------------------------------------------------------
    // Flag to select one Acq off-line processing / full RUN off-line processing
    // --------------------------------------------------------------------------
    // Will be done via a method later
    // 06/08/2009
    // --------------------------------------------------------------------------
    
    SInt8 PubAcqOffLineProcOrRunOffLineProc;

    // -------------------------------------------
    // Constructor & Destructor
    // -------------------------------------------
    
    MI26__TCTelMon ();
    ~MI26__TCTelMon ();

    // ------------------------------------------------------------
    // Begin method => MUST be called before ANY other function !
    // ------------------------------------------------------------
    
    SInt32   PubFBegin ( char* ErrLogFile, SInt8 EnableErrLog, SInt8 ErrLogLvl, SInt32 MaxBuffEvNb );

    // -------------------------------------------
    // GUI handling methods
    // -------------------------------------------
    
    #ifndef ROOT_ROOT
    
      // GUI interface
      
      SInt32 PubFConnectGui ();
      
      SInt32 PubFGetGuiPar ();
    
    #endif
    
    // -------------------------------------------
    // Run control methods
    // -------------------------------------------
    // Allocate / free buffers
    // -------------------------------------------
    
    SInt32 PubFStartRun ( SInt8 MapsNb );
    SInt32 PubFStopRun ();
    
    // -------------------------------------------
    // Monitoring control methods
    // -------------------------------------------
    
    SInt32 PubFStartMon ();
    SInt32 PubFStopMon ();
    
    // -------------------------------------------
    // Data processing methods
    // -------------------------------------------
    
    // Add events in on-line monitoring mode and call data processing methods ( protected )
    
    SInt32 PubFAddEvents ( SInt8 MapsName, SInt8 MapsNb, SInt32 EvNb, MI26__TZsFFrameRaw* PtSrc, SInt8 MakeLocalCpy, SInt8 OffLineCall );
        
    // Off-line processing of data
    // WARNING : NOW - 26/07/2009 - IT'S ONLY data of LAST acquisition ( data provided by last call to PubFAddEvents )
    // => Uses as events number the one of last acq
    // => Force mode to "Process one Acq"  AND update GUI parameters
  
    SInt32 PubFProcessOffLineCurAcq ();
    SInt32 PubFProcessOffLineCurAcq2 ();
  
    // Select frame to plot
    
    SInt32 PubFGotoFirstFrame ();
    SInt32 PubFGotoPrevFrame ();
    SInt32 PubFGotoNextFrame ();
    SInt32 PubFGotoLastFrame ();
    
    // -------------------------------------------
    // Results print methods
    // -------------------------------------------
    
    // Print list of events with a trigger detected => print SStatus record
    
    SInt32 PubPrintListEvWithTrig ( SInt8 Verbose );
    
    // Print list of events with at least one hit in one plane => print SStatus record 
    
    SInt32 PubPrintListEvWithHit  ( SInt8 Verbose );

    // Print list of events with hit on ALL planes => print SStatus record
    
    SInt32 PubPrintListEvWithHitOnAllPlanes  ( SInt8 Verbose );
    
    // Print res run
    
    SInt32 PubAllocResRun ();
    SInt32 PubFreeResRun ();
    SInt32 PubPrintResRun ();
    SInt32 PubSaveResRun  ( char* FileName );
    
    
    SInt32 PubFIsEvInListEvWithHitOnAllPlanes ( SInt32 EvNo );
    SInt32 PubFIsEvLastOfListEvWithHitOnAllPlanes ( SInt32 EvNo );

    SInt32 PubFProcOnlyEvWithHitOnAllPlanes ( SInt8 Yes );


    // -------------------------------------------
    // Results plot methods
    // -------------------------------------------

    // There is NO plot method in class.
    //
    // Because a call of a plot function ( from plot lib ) in DAQ supervisor thread doesn't work
    // - nothing is displayed !
    // - this may crash the application
    // I don't know why and have no time to study and understand this bug. I have found a workarround
    // by calling the plot function in an application timer.
    //
    // The class request plot by setting the flag ProRequestPlot to 1
    // plot is done by "plot timer" callback in application which
    // reset the flag ProRequestPlot after plot.
    
    // The application "plot timer" must know :
    // - when he must plot                       --> Test variable pointed by PubFGetPtPlotRq  ()
    // - the matrix to plot : full / half scale  --> Test variable pointed by PubFGetPtDispFullMatrix ()
    // - the data to plot                        --> Read via pointer given by PubFGetPtFullMatCol () / PubFGetPtHalfMatCol ()
    //
    // Polling of plot request and access to data is NOT DONE via method calls because
    // - calling class methods in timer while other methods may be called in DAQ supervisor thread MAY cause problems => No detailed understanding, no time ...
    // - it will save execution time by avoiding function call
       
    SInt8*                            PubFGetPtDispFullMatrix ();
    MI26__TMatDiscriColor*            PubFGetPtFullMatCol ();
    MI26__TMatDiscriColorHalfScale*   PubFGetPtHalfMatCol ();
    
    
};

#endif


#endif


#endif

