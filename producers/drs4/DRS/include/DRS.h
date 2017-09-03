/********************************************************************
  DRS.h, S.Ritt, M. Schneebeli - PSI

  $Id: DRS.h 21309 2014-04-11 14:51:29Z ritt $

********************************************************************/
#ifndef DRS_H
#define DRS_H
#include <stdio.h>
#include <string.h>
#include "averager.h"

#ifdef HAVE_LIBUSB
#   ifndef HAVE_USB
#      define HAVE_USB
#   endif
#endif

#ifdef HAVE_USB
#   include <musbstd.h>
#endif                          // HAVE_USB

#ifdef HAVE_VME
#   include <mvmestd.h>
#endif                          // HAVE_VME

/* disable "deprecated" warning */
#ifdef _MSC_VER
#pragma warning(disable: 4996)
#endif

#ifndef NULL
#define NULL 0
#endif

int drs_kbhit();
unsigned int millitime();

/* transport mode */
#define TR_VME   1
#define TR_USB   2
#define TR_USB2  3

/* address types */
#ifndef T_CTRL
#define T_CTRL   1
#define T_STATUS 2
#define T_RAM    3
#define T_FIFO   4
#endif

/*---- Register addresses ------------------------------------------*/

#define REG_CTRL                     0x00000    /* 32 bit control reg */
#define REG_DAC_OFS                  0x00004
#define REG_DAC0                     0x00004
#define REG_DAC1                     0x00006
#define REG_DAC2                     0x00008
#define REG_DAC3                     0x0000A
#define REG_DAC4                     0x0000C
#define REG_DAC5                     0x0000E
#define REG_DAC6                     0x00010
#define REG_DAC7                     0x00012
#define REG_CHANNEL_CONFIG           0x00014    // low byte
#define REG_CONFIG                   0x00014    // high byte
#define REG_CHANNEL_MODE             0x00016
#define REG_ADCCLK_PHASE             0x00016
#define REG_FREQ_SET_HI              0x00018    // DRS2
#define REG_FREQ_SET_LO              0x0001A    // DRS2
#define REG_TRG_DELAY                0x00018    // DRS4
#define REG_FREQ_SET                 0x0001A    // DRS4
#define REG_TRIG_DELAY               0x0001C
#define REG_LMK_MSB                  0x0001C    // DRS4 Mezz
#define REG_CALIB_TIMING             0x0001E    // DRS2
#define REG_EEPROM_PAGE_EVAL         0x0001E    // DRS4 Eval
#define REG_EEPROM_PAGE_MEZZ         0x0001A    // DRS4 Mezz
#define REG_TRG_CONFIG               0x0001C    // DRS4 Eval4
#define REG_LMK_LSB                  0x0001E    // DRS4 Mezz
#define REG_WARMUP                   0x00020    // DRS4 Mezz
#define REG_COOLDOWN                 0x00022    // DRS4 Mezz
#define REG_READ_POINTER             0x00026    // DRS4 Mezz

#define REG_MAGIC                    0x00000
#define REG_BOARD_TYPE               0x00002
#define REG_STATUS                   0x00004
#define REG_RDAC_OFS                 0x0000E
#define REG_RDAC0                    0x00008
#define REG_STOP_CELL0               0x00008
#define REG_RDAC1                    0x0000A
#define REG_STOP_CELL1               0x0000A
#define REG_RDAC2                    0x0000C
#define REG_STOP_CELL2               0x0000C
#define REG_RDAC3                    0x0000E
#define REG_STOP_CELL3               0x0000E
#define REG_RDAC4                    0x00000
#define REG_RDAC5                    0x00002
#define REG_STOP_WSR0                0x00010
#define REG_STOP_WSR1                0x00011
#define REG_STOP_WSR2                0x00012
#define REG_STOP_WSR3                0x00013
#define REG_RDAC6                    0x00014
#define REG_RDAC7                    0x00016
#define REG_EVENTS_IN_FIFO           0x00018
#define REG_EVENT_COUNT              0x0001A
#define REG_FREQ1                    0x0001C
#define REG_FREQ2                    0x0001E
#define REG_WRITE_POINTER            0x0001E
#define REG_TEMPERATURE              0x00020
#define REG_TRIGGER_BUS              0x00022
#define REG_SERIAL_BOARD             0x00024
#define REG_VERSION_FW               0x00026
#define REG_SCALER0                  0x00028
#define REG_SCALER1                  0x0002C
#define REG_SCALER2                  0x00030
#define REG_SCALER3                  0x00034
#define REG_SCALER4                  0x00038
#define REG_SCALER5                  0x0003C

/*---- Control register bit definitions ----------------------------*/

#define BIT_START_TRIG        (1<<0)    // write a "1" to start domino wave
#define BIT_REINIT_TRIG       (1<<1)    // write a "1" to stop & reset DRS
#define BIT_SOFT_TRIG         (1<<2)    // write a "1" to stop and read data to RAM
#define BIT_EEPROM_WRITE_TRIG (1<<3)    // write a "1" to write into serial EEPROM
#define BIT_EEPROM_READ_TRIG  (1<<4)    // write a "1" to read from serial EEPROM
#define BIT_MULTI_BUFFER     (1<<16)    // Use multi buffering when "1"
#define BIT_DMODE            (1<<17)    // (*DRS2*) 0: single shot, 1: circular
#define BIT_ADC_ACTIVE       (1<<17)    // (*DRS4*) 0: stop ADC when running, 1: ADC always clocked
#define BIT_LED              (1<<18)    // 1=on, 0=blink during readout
#define BIT_TCAL_EN          (1<<19)    // switch on (1) / off (0) for 33 MHz calib signal
#define BIT_TCAL_SOURCE      (1<<20)
#define BIT_REFCLK_SOURCE    (1<<20)
#define BIT_FREQ_AUTO_ADJ    (1<<21)    // DRS2/3
#define BIT_TRANSP_MODE      (1<<21)    // DRS4
#define BIT_ENABLE_TRIGGER1  (1<<22)    // External LEMO/FP/TRBUS trigger
#define BIT_LONG_START_PULSE (1<<23)    // (*DRS2*) 0:short start pulse (>0.8GHz), 1:long start pulse (<0.8GHz)
#define BIT_READOUT_MODE     (1<<23)    // (*DRS3*,*DRS4*) 0:start from first bin, 1:start from domino stop
#define BIT_DELAYED_START    (1<<24)    // DRS2: start domino wave 400ns after soft trigger, used for waveform
                                        // generator startup
#define BIT_NEG_TRIGGER      (1<<24)    // DRS4: use high-to-low trigger if set
#define BIT_ACAL_EN          (1<<25)    // connect DRS to inputs (0) or to DAC6 (1)
#define BIT_TRIGGER_DELAYED  (1<<26)    // select delayed trigger from trigger bus
#define BIT_ADCCLK_INVERT    (1<<26)    // invert ADC clock
#define BIT_REFCLK_EXT       (1<<26)    // use external MMCX CLKIN refclk
#define BIT_DACTIVE          (1<<27)    // keep domino wave running during readout
#define BIT_STANDBY_MODE     (1<<28)    // put chip in standby mode
#define BIT_TR_SOURCE1       (1<<29)    // trigger source selection bits
#define BIT_DECIMATION       (1<<29)    // drop all odd samples (DRS4 mezz.)
#define BIT_TR_SOURCE2       (1<<30)    // trigger source selection bits
#define BIT_ENABLE_TRIGGER2  (1<<31)    // analog threshold (internal) trigger

/* DRS4 configuration register bit definitions */
#define BIT_CONFIG_DMODE      (1<<8)    // 0: single shot, 1: circular
#define BIT_CONFIG_PLLEN      (1<<9)    // write a "1" to enable the internal PLL
#define BIT_CONFIG_WSRLOOP   (1<<10)    // write a "1" to connect WSROUT to WSRIN internally

/*---- Status register bit definitions -----------------------------*/

#define BIT_RUNNING           (1<<0)    // one if domino wave running or readout in progress
#define BIT_NEW_FREQ1         (1<<1)    // one if new frequency measurement available
#define BIT_NEW_FREQ2         (1<<2)
#define BIT_PLL_LOCKED0       (1<<1)    // 1 if PLL has locked (DRS4 evaluation board only)
#define BIT_PLL_LOCKED1       (1<<2)    // 1 if PLL DRS4 B has locked (DRS4 mezzanine board only)
#define BIT_PLL_LOCKED2       (1<<3)    // 1 if PLL DRS4 C has locked (DRS4 mezzanine board only)
#define BIT_PLL_LOCKED3       (1<<4)    // 1 if PLL DRS4 D has locked (DRS4 mezzanine board only)
#define BIT_SERIAL_BUSY       (1<<5)    // 1 if EEPROM operation in progress
#define BIT_LMK_LOCKED        (1<<6)    // 1 if PLL of LMK chip has locked (DRS4 mezzanine board only)
#define BIT_2048_MODE         (1<<7)    // 1 if 2048-bin mode has been soldered

enum DRSBoardConstants {
   kNumberOfChannelsMax         =   10,
   kNumberOfCalibChannelsV3     =   10,
   kNumberOfCalibChannelsV4     =    8,
   kNumberOfBins                = 1024,
   kNumberOfChipsMax            =    4,
   kFrequencyCacheSize          =   10,
   kBSplineOrder                =    4,
   kPreCaliculatedBSplines      = 1000,
   kPreCaliculatedBSplineGroups =    5,
   kNumberOfADCBins             = 4096,
   kBSplineXMinOffset           =   20,
   kMaxNumberOfClockCycles      =  100,
};

enum DRSErrorCodes {
   kSuccess                     =  0,
   kInvalidTriggerSignal        = -1,
   kWrongChannelOrChip          = -2,
   kInvalidTransport            = -3,
   kZeroSuppression             = -4,
   kWaveNotAvailable            = -5
};

/*---- callback class ----*/

class DRSCallback
{
public:
   virtual void Progress(int value) = 0;
   virtual ~DRSCallback() {};
};

/*------------------------*/

class DRSBoard;

class ResponseCalibration {
protected:

   class CalibrationData {
   public:
      class CalibrationDataChannel {
      public:
         unsigned char   fLimitGroup[kNumberOfBins];           //!
         float           fMin[kNumberOfBins];                  //!
         float           fRange[kNumberOfBins];                //!
         short           fOffset[kNumberOfBins];               //!
         short           fGain[kNumberOfBins];                 //!
         unsigned short  fOffsetADC[kNumberOfBins];            //!
         short          *fData[kNumberOfBins];                 //!
         unsigned char  *fLookUp[kNumberOfBins];               //!
         unsigned short  fLookUpOffset[kNumberOfBins];         //!
         unsigned char   fNumberOfLookUpPoints[kNumberOfBins]; //!
         float          *fTempData;                            //!

      private:
         CalibrationDataChannel(const CalibrationDataChannel &c);              // not implemented
         CalibrationDataChannel &operator=(const CalibrationDataChannel &rhs); // not implemented

      public:
         CalibrationDataChannel(int numberOfGridPoints)
         :fTempData(new float[numberOfGridPoints]) {
            int i;
            for (i = 0; i < kNumberOfBins; i++) {
               fData[i] = new short[numberOfGridPoints];
            }
            memset(fLimitGroup,           0, sizeof(fLimitGroup));
            memset(fMin,                  0, sizeof(fMin));
            memset(fRange,                0, sizeof(fRange));
            memset(fOffset,               0, sizeof(fOffset));
            memset(fGain,                 0, sizeof(fGain));
            memset(fOffsetADC,            0, sizeof(fOffsetADC));
            memset(fLookUp,               0, sizeof(fLookUp));
            memset(fLookUpOffset,         0, sizeof(fLookUpOffset));
            memset(fNumberOfLookUpPoints, 0, sizeof(fNumberOfLookUpPoints));
         }
         ~CalibrationDataChannel() {
            int i;
            delete fTempData;
            for (i = 0; i < kNumberOfBins; i++) {
               delete fData[i];
               delete fLookUp[i];
            }
         }
      };

      bool                    fRead;                                  //!
      CalibrationDataChannel *fChannel[10];                           //!
      unsigned char           fNumberOfGridPoints;                    //!
      int                     fHasOffsetCalibration;                  //!
      float                   fStartTemperature;                      //!
      float                   fEndTemperature;                        //!
      int                    *fBSplineOffsetLookUp[kNumberOfADCBins]; //!
      float                 **fBSplineLookUp[kNumberOfADCBins];       //!
      float                   fMin;                                   //!
      float                   fMax;                                   //!
      unsigned char           fNumberOfLimitGroups;                   //!
      static float            fIntRevers[2 * kBSplineOrder - 2];

   private:
      CalibrationData(const CalibrationData &c);              // not implemented
      CalibrationData &operator=(const CalibrationData &rhs); // not implemented

   public:
      CalibrationData(int numberOfGridPoints);
      ~CalibrationData();
      static int CalculateBSpline(int nGrid, float value, float *bsplines);
      void       PreCalculateBSpline();
      void       DeletePreCalculatedBSpline();
   };

   // General Fields
   DRSBoard        *fBoard;

   double           fPrecision;

   // Fields for creating the Calibration
   bool             fInitialized;
   bool             fRecorded;
   bool             fFitted;
   bool             fOffset;
   bool             fCalibrationValid[2];

   int              fNumberOfPointsLowVolt;
   int              fNumberOfPoints;
   int              fNumberOfMode2Bins;
   int              fNumberOfSamples;
   int              fNumberOfGridPoints;
   int              fNumberOfXConstPoints;
   int              fNumberOfXConstGridPoints;
   double           fTriggerFrequency;
   int              fShowStatistics;
   FILE            *fCalibFile;

   int              fCurrentLowVoltPoint;
   int              fCurrentPoint;
   int              fCurrentSample;
   int              fCurrentFitChannel;
   int              fCurrentFitBin;

   float           *fResponseX[10][kNumberOfBins];
   float           *fResponseY;
   unsigned short **fWaveFormMode3[10];
   unsigned short **fWaveFormMode2[10];
   short          **fWaveFormOffset[10];
   unsigned short **fWaveFormOffsetADC[10];
   unsigned short  *fSamples;
   int             *fSampleUsed;

   float           *fPntX[2];
   float           *fPntY[2];
   float           *fUValues[2];
   float           *fRes[kNumberOfBins];
   float           *fResX[kNumberOfBins];

   double          *fXXFit;
   double          *fYYFit;
   double          *fWWFit;
   double          *fYYFitRes;
   double          *fYYSave;
   double          *fXXSave;
   double          fGainMin;
   double          fGainMax;

   float          **fStatisticsApprox;
   float          **fStatisticsApproxExt;

   // Fields for applying the Calibration
   CalibrationData *fCalibrationData[kNumberOfChipsMax];

private:
         ResponseCalibration(const ResponseCalibration &c);              // not implemented
         ResponseCalibration &operator=(const ResponseCalibration &rhs); // not implemented

public:
   ResponseCalibration(DRSBoard* board);
   ~ResponseCalibration();

   void   SetCalibrationParameters(int numberOfPointsLowVolt, int numberOfPoints, int numberOfMode2Bins,
                                   int numberOfSamples, int numberOfGridPoints, int numberOfXConstPoints,
                                   int numberOfXConstGridPoints, double triggerFrequency, int showStatistics = 0);
   void   ResetCalibration();
   bool   RecordCalibrationPoints(int chipNumber);
   bool   RecordCalibrationPointsV3(int chipNumber);
   bool   RecordCalibrationPointsV4(int chipNumber);
   bool   FitCalibrationPoints(int chipNumber);
   bool   FitCalibrationPointsV3(int chipNumber);
   bool   FitCalibrationPointsV4(int chipNumber);
   bool   OffsetCalibration(int chipNumber);
   bool   OffsetCalibrationV3(int chipNumber);
   bool   OffsetCalibrationV4(int chipNumber);
   double GetTemperature(unsigned int chipIndex);

   bool   WriteCalibration(unsigned int chipIndex);
   bool   WriteCalibrationV3(unsigned int chipIndex);
   bool   WriteCalibrationV4(unsigned int chipIndex);
   bool   ReadCalibration(unsigned int chipIndex);
   bool   ReadCalibrationV3(unsigned int chipIndex);
   bool   ReadCalibrationV4(unsigned int chipIndex);
   bool   Calibrate(unsigned int chipIndex, unsigned int channel, unsigned short *adcWaveform, short *uWaveform,
                    int triggerCell, float threshold, bool offsetCalib);
   bool   SubtractADCOffset(unsigned int chipIndex, unsigned int channel, unsigned short *adcWaveform,
                            unsigned short *adcCalibratedWaveform, unsigned short newBaseLevel);
   bool   IsRead(int chipIndex) const { return fCalibrationValid[chipIndex]; }
   double GetPrecision() const { return fPrecision; };

   double GetOffsetAt(int chip,int chn,int bin) const { return fCalibrationData[chip]->fChannel[chn]->fOffset[bin]; };
   double GetGainAt(int chip,int chn,int bin) const { return fCalibrationData[chip]->fChannel[chn]->fGain[bin]; };
   double GetMeasPointXAt(int ip) const { return fXXSave[ip]; };
   double GetMeasPointYAt(int ip) const { return fYYSave[ip]; };

protected:
   void   InitFields(int numberOfPointsLowVolt, int numberOfPoints, int numberOfMode2Bins, int numberOfSamples,
                     int numberOfGridPoints, int numberOfXConstPoints, int numberOfXConstGridPoints,
                     double triggerFrequency, int showStatistics);
   void   DeleteFields();
   void   CalibrationTrigger(int mode, double voltage);
   void   CalibrationStart(double voltage);

   static float  GetValue(float *coefficients, float u, int n);
   static int    Approx(float *p, float *uu, int np, int nu, float *coef);
   static void   LeastSquaresAccumulation(float **matrix, int nb, int *ip, int *ir, int mt, int jt);
   static int    LeastSquaresSolving(float **matrix, int nb, int ip, int ir, float *x, int n);
   static void   Housholder(int lpivot, int l1, int m, float **u, int iU1, int iU2, float *up, float **c, int iC1,
                            int iC2, int ice, int ncv);

   static int    MakeDir(const char *path);
   static void   Average(int method,float *samples,int numberOfSamples,float &mean,float &error,float sigmaBoundary);
};


class DRSBoard {
protected:
   class TimeData {
   public:
      class FrequencyData {
      public:
         int    fFrequency;
         double fBin[kNumberOfBins];
      };

      enum {
         kMaxNumberOfFrequencies = 4000
      };
      int            fChip;
      int            fNumberOfFrequencies;
      FrequencyData *fFrequency[kMaxNumberOfFrequencies];

   private:
      TimeData(const TimeData &c);              // not implemented
      TimeData &operator=(const TimeData &rhs); // not implemented

   public:
      TimeData()
      :fChip(0)
      ,fNumberOfFrequencies(0) {
      }
      ~TimeData() {
         int i;
         for (i = 0; i < fNumberOfFrequencies; i++) {
            delete fFrequency[i];
         }
      }
   };

public:
   // DAC channels (CMC Version 1 : DAC_COFSA,DAC_COFSB,DAC_DRA,DAC_DSA,DAC_TLEVEL,DAC_ACALIB,DAC_DSB,DAC_DRB)
   unsigned int         fDAC_COFSA;
   unsigned int         fDAC_COFSB;
   unsigned int         fDAC_DRA;
   unsigned int         fDAC_DSA;
   unsigned int         fDAC_TLEVEL;
   unsigned int         fDAC_ACALIB;
   unsigned int         fDAC_DSB;
   unsigned int         fDAC_DRB;
   // DAC channels (CMC Version 2+3 : DAC_COFS,DAC_DSA,DAC_DSB,DAC_TLEVEL,DAC_ADCOFS,DAC_CLKOFS,DAC_ACALIB)
   unsigned int         fDAC_COFS;
   unsigned int         fDAC_ADCOFS;
   unsigned int         fDAC_CLKOFS;
   // DAC channels (CMC Version 4 : DAC_ROFS_1,DAC_DSA,DAC_DSB,DAC_ROFS_2,DAC_ADCOFS,DAC_ACALIB,DAC_INOFS,DAC_BIAS)
   unsigned int         fDAC_ROFS_1;
   unsigned int         fDAC_ROFS_2;
   unsigned int         fDAC_INOFS;
   unsigned int         fDAC_BIAS;
   // DAC channels (USB EVAL1 (fBoardType 5) : DAC_ROFS_1,DAC_CMOFS,DAC_CALN,DAC_CALP,DAC_BIAS,DAC_TLEVEL,DAC_ONOFS)
   // DAC channels (USB EVAL3 (fBoardType 7) : DAC_ROFS_1,DAC_CMOFS,DAC_CALN,DAC_CALP,DAC_BIAS,DAC_TLEVEL,DAC_ONOFS)
   unsigned int         fDAC_CMOFS;
   unsigned int         fDAC_CALN;
   unsigned int         fDAC_CALP;
   unsigned int         fDAC_ONOFS;
   // DAC channels (DRS4 MEZZ1 (fBoardType 6) : DAC_ONOFS,DAC_CMOFSP,DAC_CALN,DAC_CALP,DAC_BIAS,DAC_CMOFSN,DAC_ROFS_1)
   unsigned int         fDAC_CMOFSP;
   unsigned int         fDAC_CMOFSN;
   // DAC channels (DRS4 EVAL4 (fBoardType 8) : DAC_ONOFS,DAC_TLEVEL4,DAC_CALN,DAC_CALP,DAC_BIAS,DAC_TLEVEL1,DAC_TLEVEL2,DAC_TLEVEL3)
   unsigned int         fDAC_TLEVEL1;
   unsigned int         fDAC_TLEVEL2;
   unsigned int         fDAC_TLEVEL3;
   unsigned int         fDAC_TLEVEL4;

protected:
   // Fields for DRS
   int                  fDRSType;
   int                  fBoardType;
   int                  fNumberOfChips;
   int                  fNumberOfChannels;
   int                  fRequiredFirmwareVersion;
   int                  fFirmwareVersion;
   int                  fBoardSerialNumber;
   int                  fHasMultiBuffer;
   unsigned int         fTransport;
   unsigned int         fCtrlBits;
   int                  fNumberOfReadoutChannels;
   int                  fReadoutChannelConfig;
   int                  fADCClkPhase;
   bool                 fADCClkInvert;
   double               fExternalClockFrequency;
#ifdef HAVE_USB
   MUSB_INTERFACE      *fUsbInterface;
#endif
#ifdef HAVE_VME
   MVME_INTERFACE      *fVmeInterface;
   mvme_addr_t          fBaseAddress;
#endif
   int                  fSlotNumber;
   double               fNominalFrequency;
   double               fTrueFrequency;
   double               fTCALFrequency;
   double               fRefClock;
   int                  fMultiBuffer;
   int                  fDominoMode;
   int                  fDominoActive;
   int                  fADCActive;
   int                  fChannelConfig;
   int                  fChannelCascading;
   int                  fChannelDepth;
   int                  fWSRLoop;
   int                  fReadoutMode;
   unsigned short       fReadPointer;
   int                  fNMultiBuffer;
   int                  fTriggerEnable1;
   int                  fTriggerEnable2;
   int                  fTriggerSource;
   int                  fTriggerDelay;
   double               fTriggerDelayNs;
   int                  fSyncDelay;
   int                  fDelayedStart;
   int                  fTranspMode;
   int                  fDecimation;
   unsigned short       fStopCell[4];
   unsigned char        fStopWSR[4];
   unsigned short       fTriggerBus;
   double               fROFS;
   double               fRange;
   double               fCommonMode;
   int                  fAcalMode;
   int                  fbkAcalMode;
   double               fAcalVolt;
   double               fbkAcalVolt;
   int                  fTcalFreq;
   int                  fbkTcalFreq;
   int                  fTcalLevel;
   int                  fbkTcalLevel;
   int                  fTcalPhase;
   int                  fTcalSource;
   int                  fRefclk;

   unsigned char        fWaveforms[kNumberOfChipsMax * kNumberOfChannelsMax * 2 * kNumberOfBins];

   // Fields for Calibration
   int                  fMaxChips;
   char                 fCalibDirectory[1000];

   // Fields for Response Calibration old method
   ResponseCalibration *fResponseCalibration;

   // Fields for Calibration new method
   bool                 fVoltageCalibrationValid;
   double               fCellCalibratedRange;
   double               fCellCalibratedTemperature;
   unsigned short       fCellOffset[kNumberOfChipsMax * kNumberOfChannelsMax][kNumberOfBins];
   unsigned short       fCellOffset2[kNumberOfChipsMax * kNumberOfChannelsMax][kNumberOfBins];
   double               fCellGain[kNumberOfChipsMax * kNumberOfChannelsMax][kNumberOfBins];

   double               fTimingCalibratedFrequency;
   double               fCellDT[kNumberOfChipsMax][kNumberOfChannelsMax][kNumberOfBins];

   // Fields for Time Calibration
   TimeData           **fTimeData;
   int                  fNumberOfTimeData;

   // General debugging flag
   int fDebug;

   // Fields for wave transfer
   bool                 fWaveTransferred[kNumberOfChipsMax * kNumberOfChannelsMax];

   // Waveform Rotation
   int                  fTriggerStartBin; // Start Bin of the trigger

private:
   DRSBoard(const DRSBoard &c);              // not implemented
   DRSBoard &operator=(const DRSBoard &rhs); // not implemented

public:
   // Public Methods
#ifdef HAVE_USB
   DRSBoard(MUSB_INTERFACE * musb_interface, int usb_slot);
#endif
#ifdef HAVE_VME
   DRSBoard(MVME_INTERFACE * mvme_interface, mvme_addr_t base_address, int slot_number);

   MVME_INTERFACE *GetVMEInterface() const { return fVmeInterface; };
#endif
   ~DRSBoard();

   int          SetBoardSerialNumber(unsigned short serialNumber);
   int          GetBoardSerialNumber() const { return fBoardSerialNumber; }
   int          HasMultiBuffer() const { return fHasMultiBuffer; }
   int          GetFirmwareVersion() const { return fFirmwareVersion; }
   int          GetRequiredFirmwareVersion() const { return fRequiredFirmwareVersion; }
   int          GetDRSType() const { return fDRSType; }
   int          GetBoardType() const { return fBoardType; }
   int          GetNumberOfChips() const { return fNumberOfChips; }
   // channel         : Flash ADC index
   // readout channel : VME readout index
   // input           : Input on board
   int          GetNumberOfChannels() const { return fNumberOfChannels; }
   int          GetChannelDepth() const { return fChannelDepth; }
   int          GetChannelCascading() const { return fChannelCascading; }
   inline int   GetNumberOfReadoutChannels() const;
   inline int   GetWaveformBufferSize() const;
   inline int   GetNumberOfInputs() const;
   inline int   GetNumberOfCalibInputs() const;
   inline int   GetClockChannel() const;
   inline int   GetTriggerChannel() const;
   inline int   GetClockInput() const { return Channel2Input(GetClockChannel()); }
   inline int   GetTriggerInput() const { return fDRSType < 4 ? Channel2Input(GetTriggerChannel()) : -1; }
   inline int   Channel2Input(int channel) const;
   inline int   Channel2ReadoutChannel(int channel) const;
   inline int   Input2Channel(int input, int ind = 0) const;
   inline int   Input2ReadoutChannel(int input, int ind = 0) const;
   inline int   ReadoutChannel2Channel(int readout) const;
   inline int   ReadoutChannel2Input(int readout) const;

   inline bool  IsCalibChannel(int ch) const;
   inline bool  IsCalibInput(int input) const;
   int          GetSlotNumber() const { return fSlotNumber; }
   int          InitFPGA(void);
   int          Write(int type, unsigned int addr, void *data, int size);
   int          Read(int type, void *data, unsigned int addr, int size);
   int          GetTransport() const { return fTransport; }
   void         RegisterTest(void);
   int          RAMTest(int flag);
   int          ChipTest();
   unsigned int GetCtrlReg(void);
   unsigned short GetConfigReg(void);
   unsigned int GetStatusReg(void);
   void         SetLED(int state);
   int          SetChannelConfig(int firstChannel, int lastChannel, int nConfigChannels);
   void         SetADCClkPhase(int phase, bool invert);
   void         SetWarmup(unsigned int ticks);
   void         SetCooldown(unsigned int ticks);
   int          GetReadoutChannelConfig() { return fReadoutChannelConfig; }
   void         SetNumberOfChannels(int nChannels);
   int          EnableTrigger(int flag1, int flag2);
   int          GetTriggerEnable(int i) { return i?fTriggerEnable2:fTriggerEnable1; }
   int          SetDelayedTrigger(int flag);
   int          SetTriggerDelayPercent(int delay);
   int          SetTriggerDelayNs(int delay);
   int          GetTriggerDelay() { return fTriggerDelay; }
   double       GetTriggerDelayNs() { return fTriggerDelayNs; }
   int          SetSyncDelay(int ticks);
   int          SetTriggerLevel(double value);
   int          SetIndividualTriggerLevel(int channel, double voltage);
   int          SetTriggerPolarity(bool negative);
   int          SetTriggerSource(int source);
   int          GetTriggerSource() { return fTriggerSource; }
   int          SetDelayedStart(int flag);
   int          SetTranspMode(int flag);
   int          SetStandbyMode(int flag);
   int          SetDecimation(int flag);
   int          GetDecimation() { return fDecimation; }
   int          IsBusy(void);
   int          IsEventAvailable(void);
   int          IsPLLLocked(void);
   int          IsLMKLocked(void);
   int          IsNewFreq(unsigned char chipIndex);
   int          SetDAC(unsigned char channel, double value);
   int          ReadDAC(unsigned char channel, double *value);
   int          GetRegulationDAC(double *value);
   int          StartDomino();
   int          StartClearCycle();
   int          FinishClearCycle();
   int          Reinit();
   int          Init();
   void         SetDebug(int debug) { fDebug = debug; }
   int          Debug() { return fDebug; }
   int          SetDominoMode(unsigned char mode);
   int          SetDominoActive(unsigned char mode);
   int          SetReadoutMode(unsigned char mode);
   int          SoftTrigger(void);
   int          ReadFrequency(unsigned char chipIndex, double *f);
   int          SetFrequency(double freq, bool wait);
   double       VoltToFreq(double volt);
   double       FreqToVolt(double freq);
   double       GetNominalFrequency() const { return fNominalFrequency; }
   double       GetTrueFrequency();
   int          RegulateFrequency(double freq);
   int          SetExternalClockFrequency(double frequencyMHz);
   double       GetExternalClockFrequency();
   int          SetMultiBuffer(int flag);
   int          IsMultiBuffer() { return fMultiBuffer; }
   void         ResetMultiBuffer(void);
   int          GetMultiBufferRP(void);
   int          SetMultiBufferRP(unsigned short rp);
   int          GetMultiBufferWP(void);
   void         IncrementMultiBufferRP(void);
   void         SetVoltageOffset(double offset1, double offset2);
   int          SetInputRange(double center);
   double       GetInputRange(void) { return fRange; }
   double       GetCalibratedInputRange(void) { return fCellCalibratedRange; }
   double       GetCalibratedTemperature(void) { return fCellCalibratedTemperature; }
   double       GetCalibratedFrequency(void) { return fTimingCalibratedFrequency; }
   int          TransferWaves(int numberOfChannels = kNumberOfChipsMax * kNumberOfChannelsMax);
   int          TransferWaves(unsigned char *p, int numberOfChannels = kNumberOfChipsMax * kNumberOfChannelsMax);
   int          TransferWaves(int firstChannel, int lastChannel);
   int          TransferWaves(unsigned char *p, int firstChannel, int lastChannel);
   int          DecodeWave(unsigned char *waveforms, unsigned int chipIndex, unsigned char channel,
                           unsigned short *waveform);
   int          DecodeWave(unsigned int chipIndex, unsigned char channel, unsigned short *waveform);
   int          GetWave(unsigned char *waveforms, unsigned int chipIndex, unsigned char channel, short *waveform,
                        bool responseCalib = false, int triggerCell = -1, int wsr = -1, bool adjustToClock = false,
                        float threshold = 0, bool offsetCalib = true);
   int          GetWave(unsigned char *waveforms, unsigned int chipIndex, unsigned char channel, float *waveform,
                        bool responseCalib = false, int triggerCell = -1, int wsr = -1, bool adjustToClock = false,
                        float threshold = 0, bool offsetCalib = true);
   int          GetWave(unsigned int chipIndex, unsigned char channel, short *waveform, bool responseCalib = false,
                        int triggerCell = -1, int wsr = -1, bool adjustToClock = false, float threshold = 0, bool offsetCalib = true);
   int          GetWave(unsigned int chipIndex, unsigned char channel, float *waveform, bool responseCalib,
                        int triggerCell = -1, int wsr = -1, bool adjustToClock = false, float threshold = 0, bool offsetCalib = true);
   int          GetWave(unsigned int chipIndex, unsigned char channel, float *waveform);
   int          GetRawWave(unsigned int chipIndex, unsigned char channel, unsigned short *waveform, bool adjustToClock = false);
   int          GetRawWave(unsigned char *waveforms,unsigned int chipIndex, unsigned char channel,
                           unsigned short *waveform, bool adjustToClock = false);
   bool         IsTimingCalibrationValid(void);
   bool         IsVoltageCalibrationValid(void) { return fVoltageCalibrationValid; }
   int          GetTime(unsigned int chipIndex, int channelIndex, double freq, int tc, float *time, bool tcalibrated=true, bool rotated=true);
   int          GetTime(unsigned int chipIndex, int channelIndex, int tc, float *time, bool tcalibrated=true, bool rotated=true);
   int          GetTimeCalibration(unsigned int chipIndex, int channelIndex, int mode, float *time, bool force=false);
   int          GetTriggerCell(unsigned int chipIndex);
   int          GetStopCell(unsigned int chipIndex);
   unsigned char GetStopWSR(unsigned int chipIndex);
   int          GetTriggerCell(unsigned char *waveforms,unsigned int chipIndex);
   void         TestDAC(int channel);
   void         MeasureSpeed();
   void         InteractSpeed();
   void         MonitorFrequency();
   int          TestShift(int n);
   int          EnableAcal(int mode, double voltage);
   int          GetAcalMode() { return fAcalMode; }
   double       GetAcalVolt() { return fAcalVolt; }
   int          EnableTcal(int freq, int level=0, int phase=0);
   int          SelectClockSource(int source);
   int          SetRefclk(int source);
   int          GetRefclk() { return fRefclk; }
   int          GetTcalFreq() { return fTcalFreq; }
   int          GetTcalLevel() { return fTcalLevel; }
   int          GetTcalPhase() { return fTcalPhase; }
   int          GetTcalSource() { return fTcalSource; }
   int          SetCalibVoltage(double value);
   int          SetCalibTiming(int t1, int t2);
   double       GetTemperature();
   int          Is2048ModeCapable();
   int          GetTriggerBus();
   unsigned int GetScaler(int channel);
   int          ReadEEPROM(unsigned short page, void *buffer, int size);
   int          WriteEEPROM(unsigned short page, void *buffer, int size);
   bool         HasCorrectFirmware();
   int          ConfigureLMK(double sampFreq, bool freqChange, int calFreq, int calPhase);

   bool         InitTimeCalibration(unsigned int chipIndex);
   void         SetCalibrationDirectory(const char *calibrationDirectoryPath);
   void         GetCalibrationDirectory(char *calibrationDirectoryPath);

   ResponseCalibration *GetResponseCalibration() const { return fResponseCalibration; }

   double       GetPrecision() const { return fResponseCalibration ? fResponseCalibration->GetPrecision() : 0.1; }
   int          CalibrateWaveform(unsigned int chipIndex, unsigned char channel, unsigned short *adcWaveform,
                                  short *waveform, bool responseCalib, int triggerCell, bool adjustToClock,
                                  float threshold, bool offsetCalib);

   static void  LinearRegression(double *x, double *y, int n, double *a, double *b);
   
   void         ReadSingleWaveform(int nChips, int nChan, 
                                  unsigned short wfu[kNumberOfChipsMax][kNumberOfChannelsMax][kNumberOfBins], bool rotated);
   int          AverageWaveforms(DRSCallback *pcb, int chipIndex, int nChan, int prog1, int prog2, unsigned short *awf, int n, bool rotated);
   int          RobustAverageWaveforms(DRSCallback *pcb, int chipIndex, int nChan, int prog1, int prog2, unsigned short *awf, int n, bool rotated);
   int          CalibrateVolt(DRSCallback *pcb);
   int          AnalyzePeriod(Averager *ave, int iIter, int nIter, int channel, float wf[kNumberOfBins], int tCell, double cellDV[kNumberOfBins], double cellDT[kNumberOfBins]);
   int          AnalyzeSlope(Averager *ave, int iIter, int nIter, int channel, float wf[kNumberOfBins], int tCell, double cellDV[kNumberOfBins], double cellDT[kNumberOfBins]);
   int          CalibrateTiming(DRSCallback *pcb);
   static void  RemoveSymmetricSpikes(short **wf, int nwf,
                                      short diffThreshold, int spikeWidth,
                                      short maxPeakToPeak, short spikeVoltage,
                                      int nTimeRegionThreshold);
protected:
   // Protected Methods
   void         ConstructBoard();
   void         ReadSerialNumber();
   void         ReadCalibration(void);

   TimeData    *GetTimeCalibration(unsigned int chipIndex, bool reinit = false);

   int          GetStretchedTime(float *time, float *measurement, int numberOfMeasurements, float period);
};

int DRSBoard::GetNumberOfReadoutChannels() const
{
   return (fDRSType == 4 && fReadoutChannelConfig == 4) ? 5 : fNumberOfChannels;
}

int DRSBoard::GetWaveformBufferSize() const
{
   int nbin=0;
   if (fDRSType < 4) {
      nbin = fNumberOfChips * fNumberOfChannels * kNumberOfBins;
   } else {
      if (fBoardType == 6) {
         if (fDecimation) {
            nbin = fNumberOfChips * (4 * kNumberOfBins + kNumberOfBins / 2);
         } else {
            nbin = fNumberOfChips * 5 * kNumberOfBins;
         }
      } else if (fBoardType == 7 || fBoardType == 8 || fBoardType == 9)
         nbin = fNumberOfChips * fNumberOfChannels * kNumberOfBins;
   }
   return nbin * static_cast<int>(sizeof(short int));
}

int DRSBoard::GetNumberOfInputs() const
{
   // return number of input channels excluding clock and trigger channels.
   if (fDRSType < 4) {
      return fNumberOfChannels - 2;
   } else {
      return fNumberOfChannels / 2;
   }
}
int DRSBoard::GetNumberOfCalibInputs() const
{
   return (fDRSType < 4) ? 2 : 1;
}
int DRSBoard::GetClockChannel() const
{
   return fDRSType < 4 ? 9 : 8;
}
int DRSBoard::GetTriggerChannel() const
{
   return fDRSType < 4 ? 8 : -1;
}

int DRSBoard::Channel2Input(int channel) const
{
   return (fDRSType < 4) ? channel : channel / 2;
}
int DRSBoard::Channel2ReadoutChannel(int channel) const
{
   if (fDRSType < 4) {
      return channel;
   } else {
      if (fReadoutChannelConfig == 4) {
         return channel / 2;
      } else {
         return channel;
      }
   }
}
int DRSBoard::Input2Channel(int input, int ind) const
{
   if (fChannelCascading == 1) {
      return (fDRSType < 4) ? input : (input * 2 + ind);
   } else {
      if (input == 4) { // clock
         return 8;
      } else {
         return input;
      }
   }
}
int DRSBoard::Input2ReadoutChannel(int input, int ind) const
{
   if (fDRSType < 4) {
      return input;
   } else {
      if (fReadoutChannelConfig == 4) {
         return input;
      } else {
         return (input * 2 + ind);
      }
   }
}
int DRSBoard::ReadoutChannel2Channel(int readout) const
{
   if (fDRSType < 4) {
      return readout;
   } else {
      if (fReadoutChannelConfig == 4) {
         return readout * 2;
      } else {
         return readout;
      }
   }
}
int DRSBoard::ReadoutChannel2Input(int readout) const
{
   if (fDRSType < 4) {
      return readout;
   } else {
      if (fReadoutChannelConfig == 4) {
         return readout;
      } else {
         return readout / 2;
      }
   }
}
bool DRSBoard::IsCalibChannel(int ch) const
{
   // return if it is clock or trigger channel
   if (fDRSType < 4)
      return ch == GetClockChannel() || ch == GetTriggerChannel();
   else
      return ch == GetClockChannel();
}
bool DRSBoard::IsCalibInput(int input) const
{
   // return if it is clock or trigger channel
   int ch = Input2Channel(input);
   if (fDRSType < 4)
      return ch == GetClockChannel() || ch == GetTriggerChannel();
   else
      return ch == GetClockChannel();
}

class DRS {
protected:
   // constants
   enum {
      kMaxNumberOfBoards = 40
   };

protected:
   DRSBoard       *fBoard[kMaxNumberOfBoards];
   int             fNumberOfBoards;
   char            fError[256];
#ifdef HAVE_VME
   MVME_INTERFACE *fVmeInterface;
#endif

private:
   DRS(const DRS &c);              // not implemented
   DRS &operator=(const DRS &rhs); // not implemented

public:
   // Public Methods
   DRS();
   ~DRS();

   DRSBoard        *GetBoard(int i) { return fBoard[i]; }
   void             SetBoard(int i, DRSBoard *b);
   DRSBoard       **GetBoards() { return fBoard; }
   int              GetNumberOfBoards() const { return fNumberOfBoards; }
   bool             GetError(char *str, int size);
   void             SortBoards();

#ifdef HAVE_VME
   MVME_INTERFACE *GetVMEInterface() const { return fVmeInterface; };
#endif
};

#endif                          // DRS_H
