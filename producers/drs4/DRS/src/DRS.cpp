/********************************************************************

  Name:         DRS.cpp
  Created by:   Stefan Ritt, Matthias Schneebeli

  Contents:     Library functions for DRS mezzanine and USB boards

  $Id: DRS.cpp 22289 2016-04-27 09:40:58Z ritt $

\********************************************************************/

#define NEW_TIMING_CALIBRATION

#ifdef USE_DRS_MUTEX 
#include "wx/wx.h"    // must be before <windows.h>
#endif

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <algorithm>
#include <sys/stat.h>
#include <fcntl.h>
#include "strlcpy.h"
#include "DRS.h"

#ifdef _MSC_VER
#pragma warning(disable:4996)
#   include <windows.h>
#   include <direct.h>
#else
#   include <unistd.h>
#   include <sys/time.h>
inline void Sleep(useconds_t x)
{
   usleep(x * 1000);
}
#endif

#ifdef _MSC_VER
#include <conio.h>
#define drs_kbhit() kbhit()
#else
#include <sys/ioctl.h>

int drs_kbhit()
{
   int n;

   ioctl(0, FIONREAD, &n);
   return (n > 0);
}
static inline int getch()
{
   return getchar();
}
#endif

#include <DRS.h>

#ifdef _MSC_VER
extern "C" {
#endif

#include <mxml.h>

#ifdef _MSC_VER
}
#endif

/*---- minimal FPGA firmvare version required for this library -----*/
const int REQUIRED_FIRMWARE_VERSION_DRS2 = 5268;
const int REQUIRED_FIRMWARE_VERSION_DRS3 = 6981;
const int REQUIRED_FIRMWARE_VERSION_DRS4 = 15147;

/*---- calibration methods to be stored in EEPROMs -----------------*/

#define VCALIB_METHOD_V4 1
#define TCALIB_METHOD_V4 1

#define VCALIB_METHOD  2
#define TCALIB_METHOD  2 // correct for sampling frequency, calibrate every channel

/*---- VME addresses -----------------------------------------------*/
#ifdef HAVE_VME
/* assuming following DIP Switch settings:

   SW1-1: 1 (off)       use geographical addressing (1=left, 21=right)
   SW1-2: 1 (off)       \
   SW1-3: 1 (off)        >  VME_WINSIZE = 8MB, subwindow = 1MB
   SW1-4: 0 (on)        /
   SW1-5: 0 (on)        reserverd
   SW1-6: 0 (on)        reserverd
   SW1-7: 0 (on)        reserverd
   SW1-8: 0 (on)       \
                        |
   SW2-1: 0 (on)        |
   SW2-2: 0 (on)        |
   SW2-3: 0 (on)        |
   SW2-4: 0 (on)        > VME_ADDR_OFFSET = 0
   SW2-5: 0 (on)        |
   SW2-6: 0 (on)        |
   SW2-7: 0 (on)        |
   SW2-8: 0 (on)       /

   which gives
     VME base address = SlotNo * VME_WINSIZE + VME_ADDR_OFFSET
                      = SlotNo * 0x80'0000
*/
#define GEVPC_BASE_ADDR           0x00000000
#define GEVPC_WINSIZE               0x800000
#define GEVPC_USER_FPGA   (GEVPC_WINSIZE*2/8)
#define PMC1_OFFSET                  0x00000
#define PMC2_OFFSET                  0x80000
#define PMC_CTRL_OFFSET              0x00000    /* all registers 32 bit */
#define PMC_STATUS_OFFSET            0x10000
#define PMC_FIFO_OFFSET              0x20000
#define PMC_RAM_OFFSET               0x40000
#endif                          // HAVE_VME
/*---- USB addresses -----------------------------------------------*/
#define USB_TIMEOUT                     1000    // one second
#ifdef HAVE_USB
#define USB_CTRL_OFFSET                 0x00    /* all registers 32 bit */
#define USB_STATUS_OFFSET               0x40
#define USB_RAM_OFFSET                  0x80
#define USB_CMD_IDENT                      0    // Query identification
#define USB_CMD_ADDR                       1    // Address cycle
#define USB_CMD_READ                       2    // "VME" read <addr><size>
#define USB_CMD_WRITE                      3    // "VME" write <addr><size>
#define USB_CMD_READ12                     4    // 12-bit read <LSB><MSB>
#define USB_CMD_WRITE12                    5    // 12-bit write <LSB><MSB>

#define USB2_CMD_READ                      1
#define USB2_CMD_WRITE                     2
#define USB2_CTRL_OFFSET             0x00000    /* all registers 32 bit */
#define USB2_STATUS_OFFSET           0x10000
#define USB2_FIFO_OFFSET             0x20000
#define USB2_RAM_OFFSET              0x40000
#endif                          // HAVE_USB

/*------------------------------------------------------------------*/

using namespace std;

#ifdef HAVE_USB
#define USB2_BUFFER_SIZE (1024*1024+10)
unsigned char static *usb2_buffer = NULL;
#endif

/*------------------------------------------------------------------*/

#ifdef USE_DRS_MUTEX
static wxMutex *s_drsMutex = NULL; // used for wxWidgets multi-threaded programs
#endif

/*------------------------------------------------------------------*/

DRS::DRS()
:  fNumberOfBoards(0)
#ifdef HAVE_VME
    , fVmeInterface(0)
#endif
{
#ifdef HAVE_USB
   MUSB_INTERFACE *usb_interface;
#endif

#if defined(HAVE_VME) || defined(HAVE_USB)
   int index = 0, i=0;
#endif

   memset(fError, 0, sizeof(fError));

#ifdef HAVE_VME
   unsigned short type, fw, magic, serial, temperature;
   mvme_addr_t addr;

   if (mvme_open(&fVmeInterface, 0) == MVME_SUCCESS) {

      mvme_set_am(fVmeInterface, MVME_AM_A32);
      mvme_set_dmode(fVmeInterface, MVME_DMODE_D16);

      /* check all VME slave slots */
      for (index = 2; index <= 21; index++) {

         /* check PMC1 */
         addr = GEVPC_BASE_ADDR + index * GEVPC_WINSIZE;        // VME board base address
         addr += GEVPC_USER_FPGA;       // UsrFPGA base address
         addr += PMC1_OFFSET;   // PMC1 offset

         mvme_set_dmode(fVmeInterface, MVME_DMODE_D16);
         i = mvme_read(fVmeInterface, &magic, addr + PMC_STATUS_OFFSET + REG_MAGIC, 2);
         if (i == 2) {
            if (magic != 0xC0DE) {
               printf("Found old firmware, please upgrade immediately!\n");
               fBoard[fNumberOfBoards] = new DRSBoard(fVmeInterface, addr, (index - 2) << 1);
               fNumberOfBoards++;
            } else {

               /* read board type */
               mvme_read(fVmeInterface, &type, addr + PMC_STATUS_OFFSET + REG_BOARD_TYPE, 2);
               type &= 0xFF;
               if (type == 2 || type == 3 || type == 4) {    // DRS2 or DRS3 or DRS4

                  /* read firmware number */
                  mvme_read(fVmeInterface, &fw, addr + PMC_STATUS_OFFSET + REG_VERSION_FW, 2);

                  /* read serial number */
                  mvme_read(fVmeInterface, &serial, addr + PMC_STATUS_OFFSET + REG_SERIAL_BOARD, 2);

                  /* read temperature register to see if CMC card is present */
                  mvme_read(fVmeInterface, &temperature, addr + PMC_STATUS_OFFSET + REG_TEMPERATURE, 2);

                  /* LED blinking */
#if 0
                  do {
                     data = 0x00040000;
                     mvme_write(fVmeInterface, addr + PMC_CTRL_OFFSET + REG_CTRL, &data, sizeof(data));
                     mvme_write(fVmeInterface, addr + PMC2_OFFSET + PMC_CTRL_OFFSET + REG_CTRL, &data,
                                sizeof(data));

                     Sleep(500);

                     data = 0x00000000;
                     mvme_write(fVmeInterface, addr + PMC_CTRL_OFFSET + REG_CTRL, &data, sizeof(data));
                     mvme_write(fVmeInterface, addr + PMC2_OFFSET + PMC_CTRL_OFFSET + REG_CTRL, data,
                                sizeof(data));

                     Sleep(500);

                  } while (1);
#endif

                  if (temperature == 0xFFFF) {
                     printf("Found VME board in slot %d, fw %d, but no CMC board in upper slot\n", index, fw);
                  } else {
                     printf("Found DRS%d board %2d in upper VME slot %2d, serial #%d, firmware revision %d\n", type, fNumberOfBoards, index, serial, fw);

                     fBoard[fNumberOfBoards] = new DRSBoard(fVmeInterface, addr, (index - 2) << 1);
                     if (!fBoard[fNumberOfBoards]->HasCorrectFirmware())
                        sprintf(fError, "Wrong firmware version: board has %d, required is %d. Board may not work correctly.\n",
                                fBoard[fNumberOfBoards]->GetFirmwareVersion(),
                                fBoard[fNumberOfBoards]->GetRequiredFirmwareVersion());
                     fNumberOfBoards++;
                  }
               }
            }
         }

         /* check PMC2 */
         addr = GEVPC_BASE_ADDR + index * GEVPC_WINSIZE;        // VME board base address
         addr += GEVPC_USER_FPGA;       // UsrFPGA base address
         addr += PMC2_OFFSET;   // PMC2 offset

         mvme_set_dmode(fVmeInterface, MVME_DMODE_D16);
         i = mvme_read(fVmeInterface, &fw, addr + PMC_STATUS_OFFSET + REG_MAGIC, 2);
         if (i == 2) {
            if (magic != 0xC0DE) {
               printf("Found old firmware, please upgrade immediately!\n");
               fBoard[fNumberOfBoards] = new DRSBoard(fVmeInterface, addr, (index - 2) << 1 | 1);
               fNumberOfBoards++;
            } else {

               /* read board type */
               mvme_read(fVmeInterface, &type, addr + PMC_STATUS_OFFSET + REG_BOARD_TYPE, 2);
               type &= 0xFF;
               if (type == 2 || type == 3 || type == 4) {    // DRS2 or DRS3 or DRS4

                  /* read firmware number */
                  mvme_read(fVmeInterface, &fw, addr + PMC_STATUS_OFFSET + REG_VERSION_FW, 2);

                  /* read serial number */
                  mvme_read(fVmeInterface, &serial, addr + PMC_STATUS_OFFSET + REG_SERIAL_BOARD, 2);

                  /* read temperature register to see if CMC card is present */
                  mvme_read(fVmeInterface, &temperature, addr + PMC_STATUS_OFFSET + REG_TEMPERATURE, 2);

                  if (temperature == 0xFFFF) {
                     printf("Found VME board in slot %d, fw %d, but no CMC board in lower slot\n", index, fw);
                  } else {
                     printf("Found DRS%d board %2d in lower VME slot %2d, serial #%d, firmware revision %d\n", type, fNumberOfBoards, index, serial, fw);

                     fBoard[fNumberOfBoards] = new DRSBoard(fVmeInterface, addr, ((index - 2) << 1) | 1);
                     if (!fBoard[fNumberOfBoards]->HasCorrectFirmware())
                        sprintf(fError, "Wrong firmware version: board has %d, required is %d. Board may not work correctly.\n",
                                fBoard[fNumberOfBoards]->GetFirmwareVersion(),
                                fBoard[fNumberOfBoards]->GetRequiredFirmwareVersion());
                     fNumberOfBoards++;
                  }
               }
            }
         }
      }
   } else
      printf("Cannot access VME crate, check driver, power and connection\n");
#endif                          // HAVE_VME

#ifdef HAVE_USB
   unsigned char buffer[512];
   int found, one_found, usb_slot;

   one_found = 0;
   usb_slot = 0;
   for (index = 0; index < 127; index++) {
      found = 0;

      /* check for USB-Mezzanine test board */
      if (musb_open(&usb_interface, 0x10C4, 0x1175, index, 1, 0) == MUSB_SUCCESS) {

         /* check ID */
         buffer[0] = USB_CMD_IDENT;
         musb_write(usb_interface, 2, buffer, 1, USB_TIMEOUT);

         i = musb_read(usb_interface, 1, (char *) buffer, sizeof(buffer), USB_TIMEOUT);
         if (strcmp((char *) buffer, "USB_MEZZ2 V1.0") != 0) {
            /* no USB-Mezzanine board found */
            musb_close(usb_interface);
         } else {
            usb_interface->usb_type = 1;        // USB 1.1
            fBoard[fNumberOfBoards] = new DRSBoard(usb_interface, usb_slot++);
            if (!fBoard[fNumberOfBoards]->HasCorrectFirmware())
               sprintf(fError, "Wrong firmware version: board has %d, required is %d. Board may not work correctly.\n",
                       fBoard[fNumberOfBoards]->GetFirmwareVersion(),
                       fBoard[fNumberOfBoards]->GetRequiredFirmwareVersion());
            fNumberOfBoards++;
            found = 1;
            one_found = 1;
         }
      }

      /* check for DRS4 evaluation board */
      if (musb_open(&usb_interface, 0x04B4, 0x1175, index, 1, 0) == MUSB_SUCCESS) {

         /* check ID */
         if (musb_get_device(usb_interface) != 1) {
            /* no DRS evaluation board found */
            musb_close(usb_interface);
         } else {

            /* drain any data from Cy7C68013 FIFO if FPGA startup caused erratic write */
            do {
               i = musb_read(usb_interface, 8, buffer, sizeof(buffer), 100);
               if (i > 0)
                  printf("%d bytes stuck in buffer\n", i);
            } while (i > 0);

            usb_interface->usb_type = 2;        // USB 2.0
            fBoard[fNumberOfBoards] = new DRSBoard(usb_interface, usb_slot++);
            if (!fBoard[fNumberOfBoards]->HasCorrectFirmware())
               sprintf(fError, "Wrong firmware version: board has %d, required is %d. Board may not work correctly.\n",
                      fBoard[fNumberOfBoards]->GetFirmwareVersion(),
                      fBoard[fNumberOfBoards]->GetRequiredFirmwareVersion());
            fNumberOfBoards++;
            found = 1;
            one_found = 1;
         }
      }

      if (!found) {
         if (!one_found)
            printf("USB successfully scanned, but no boards found\n");
         break;
      }
   }
#endif                          // HAVE_USB

   return;
}

/*------------------------------------------------------------------*/

DRS::~DRS()
{
   int i;
   for (i = 0; i < fNumberOfBoards; i++) {
      delete fBoard[i];
   }

#ifdef HAVE_USB
   if (usb2_buffer) {
      free(usb2_buffer);
      usb2_buffer = NULL;
   }
#endif

#ifdef HAVE_VME
   mvme_close(fVmeInterface);
#endif
}

/*------------------------------------------------------------------*/

void DRS::SortBoards()
{
   /* sort boards according to serial number (simple bubble sort) */
   for (int i=0 ; i<fNumberOfBoards-1 ; i++) {
      for (int j=i+1 ; j<fNumberOfBoards ; j++) {
         if (fBoard[i]->GetBoardSerialNumber() < fBoard[j]->GetBoardSerialNumber()) {
            DRSBoard* b = fBoard[i];
            fBoard[i] = fBoard[j];
            fBoard[j] = b;
         }
      }
   }
}

/*------------------------------------------------------------------*/

void DRS::SetBoard(int i, DRSBoard *b)
{
   fBoard[i] = b;
}

/*------------------------------------------------------------------*/

bool DRS::GetError(char *str, int size)
{
   if (fError[0])
      strlcpy(str, fError, size);

   return fError[0] > 0;
}

/*------------------------------------------------------------------*/

#ifdef HAVE_USB
DRSBoard::DRSBoard(MUSB_INTERFACE * musb_interface, int usb_slot)
:  fDAC_COFSA(0)
    , fDAC_COFSB(0)
    , fDAC_DRA(0)
    , fDAC_DSA(0)
    , fDAC_TLEVEL(0)
    , fDAC_ACALIB(0)
    , fDAC_DSB(0)
    , fDAC_DRB(0)
    , fDAC_COFS(0)
    , fDAC_ADCOFS(0)
    , fDAC_CLKOFS(0)
    , fDAC_ROFS_1(0)
    , fDAC_ROFS_2(0)
    , fDAC_INOFS(0)
    , fDAC_BIAS(0)
    , fDRSType(0)
    , fBoardType(0)
    , fRequiredFirmwareVersion(0)
    , fFirmwareVersion(0)
    , fBoardSerialNumber(0)
    , fHasMultiBuffer(0)
    , fCtrlBits(0)
    , fNumberOfReadoutChannels(0)
    , fReadoutChannelConfig(0)
    , fADCClkPhase(0)
    , fADCClkInvert(0)
    , fExternalClockFrequency(0)
    , fUsbInterface(musb_interface)
#ifdef HAVE_VME
    , fVmeInterface(0)
    , fBaseAddress(0)
#endif
    , fSlotNumber(usb_slot)
    , fNominalFrequency(0)
    , fMultiBuffer(0)
    , fDominoMode(0)
    , fDominoActive(0)
    , fChannelConfig(0)
    , fChannelCascading(1)
    , fChannelDepth(1024)
    , fWSRLoop(0)
    , fReadoutMode(0)
    , fReadPointer(0)
    , fNMultiBuffer(0)
    , fTriggerEnable1(0)
    , fTriggerEnable2(0)
    , fTriggerSource(0)
    , fTriggerDelay(0)
    , fTriggerDelayNs(0)
    , fSyncDelay(0)
    , fDelayedStart(0)
    , fTranspMode(0)
    , fDecimation(0)
    , fRange(0)
    , fCommonMode(0.8)
    , fAcalMode(0)
    , fAcalVolt(0)
    , fTcalFreq(0)
    , fTcalLevel(0)
    , fTcalPhase(0)
    , fTcalSource(0)
    , fRefclk(0)
    , fMaxChips(0)
    , fResponseCalibration(0)
    , fVoltageCalibrationValid(false)
    , fCellCalibratedRange(0)
    , fCellCalibratedTemperature(0)
    , fTimeData(0)
    , fNumberOfTimeData(0)
    , fDebug(0)
    , fTriggerStartBin(0)
{
   if (musb_interface->usb_type == 1)
      fTransport = TR_USB;
   else
      fTransport = TR_USB2;
   memset(fStopCell, 0, sizeof(fStopCell));
   memset(fStopWSR, 0, sizeof(fStopWSR));
   fTriggerBus = 0;
   ConstructBoard();
}

#endif

#ifdef HAVE_VME
/*------------------------------------------------------------------*/

DRSBoard::DRSBoard(MVME_INTERFACE * mvme_interface, mvme_addr_t base_address, int slot_number)
:fDAC_COFSA(0)
, fDAC_COFSB(0)
, fDAC_DRA(0)
, fDAC_DSA(0)
, fDAC_TLEVEL(0)
, fDAC_ACALIB(0)
, fDAC_DSB(0)
, fDAC_DRB(0)
, fDAC_COFS(0)
, fDAC_ADCOFS(0)
, fDAC_CLKOFS(0)
, fDAC_ROFS_1(0)
, fDAC_ROFS_2(0)
, fDAC_INOFS(0)
, fDAC_BIAS(0)
, fDRSType(0)
, fBoardType(0)
, fRequiredFirmwareVersion(0)
, fFirmwareVersion(0)
, fBoardSerialNumber(0)
, fHasMultiBuffer(0)
, fTransport(TR_VME)
, fCtrlBits(0)
, fNumberOfReadoutChannels(0)
, fReadoutChannelConfig(0)
, fADCClkPhase(0)
, fADCClkInvert(0)
, fExternalClockFrequency(0)
#ifdef HAVE_USB
, fUsbInterface(0)
#endif
#ifdef HAVE_VME
, fVmeInterface(mvme_interface)
, fBaseAddress(base_address)
, fSlotNumber(slot_number)
#endif
, fNominalFrequency(0)
, fRefClock(0)
, fMultiBuffer(0)
, fDominoMode(1)
, fDominoActive(1)
, fChannelConfig(0)
, fChannelCascading(1)
, fChannelDepth(1024)
, fWSRLoop(1)
, fReadoutMode(0)
, fReadPointer(0)
, fNMultiBuffer(0)
, fTriggerEnable1(0)
, fTriggerEnable2(0)
, fTriggerSource(0)
, fTriggerDelay(0)
, fTriggerDelayNs(0)
, fSyncDelay(0)
, fDelayedStart(0)
, fTranspMode(0)
, fDecimation(0)
, fRange(0)
, fCommonMode(0.8)
, fAcalMode(0)
, fAcalVolt(0)
, fTcalFreq(0)
, fTcalLevel(0)
, fTcalPhase(0)
, fTcalSource(0)
, fRefclk(0)
, fMaxChips(0)
, fResponseCalibration(0)
, fTimeData(0)
, fNumberOfTimeData(0)
, fDebug(0)
, fTriggerStartBin(0)
{
   ConstructBoard();
}

#endif

/*------------------------------------------------------------------*/

DRSBoard::~DRSBoard()
{
   int i;
#ifdef HAVE_USB
   if (fTransport == TR_USB || fTransport == TR_USB2)
      musb_close(fUsbInterface);
#endif

#ifdef USE_DRS_MUTEX
   if (s_drsMutex)
      delete s_drsMutex;
   s_drsMutex = NULL;
#endif

   // Response Calibration
   delete fResponseCalibration;

   // Time Calibration
   for (i = 0; i < fNumberOfTimeData; i++) {
      delete fTimeData[i];
   }
   delete[]fTimeData;
}

/*------------------------------------------------------------------*/

void DRSBoard::ConstructBoard()
{
   unsigned char buffer[2];
   unsigned int bits;

   fDebug = 0;
   fWSRLoop = 1;
   fCtrlBits = 0;

   fExternalClockFrequency = 1000. / 30.;
   strcpy(fCalibDirectory, ".");

   /* check board communication */
   if (Read(T_STATUS, buffer, REG_MAGIC, 2) < 0) {
      InitFPGA();
      if (Read(T_STATUS, buffer, REG_MAGIC, 2) < 0)
         return;
   }

   ReadSerialNumber();

   /* set correct reference clock */
   if (fBoardType == 5 || fBoardType == 7 || fBoardType == 8 || fBoardType == 9)
      fRefClock = 60;
   else
      fRefClock = 33;

   /* get mode from hardware */
   bits = GetCtrlReg();
   fMultiBuffer = (bits & BIT_MULTI_BUFFER) > 0;
   fNMultiBuffer = 0;
   if (fHasMultiBuffer && fMultiBuffer)
      fNMultiBuffer = 3;
   if (fDRSType == 4) {
      fDominoMode = (bits & BIT_CONFIG_DMODE) > 0;
   } else {
      fDominoMode = (bits & BIT_DMODE) > 0;
   }
   fTriggerEnable1 = (bits & BIT_ENABLE_TRIGGER1) > 0;
   fTriggerEnable2 = (bits & BIT_ENABLE_TRIGGER2) > 0;
   fTriggerSource = ((bits & BIT_TR_SOURCE1) > 0) | (((bits & BIT_TR_SOURCE2) > 0) << 1);
   fReadoutMode = (bits & BIT_READOUT_MODE) > 0;
   Read(T_CTRL, &fReadPointer, REG_READ_POINTER, 2);
   fADCClkInvert = (bits & BIT_ADCCLK_INVERT) > 0;
   fDominoActive = (bits & BIT_DACTIVE) > 0;
   ReadFrequency(0, &fNominalFrequency);
   if (fNominalFrequency < 0.1 || fNominalFrequency > 6)
      fNominalFrequency = 1;

   /* initialize number of channels */
   if (fDRSType == 4) {
      if (fBoardType == 6) {
         unsigned short d;
         Read(T_CTRL, &d, REG_CHANNEL_MODE, 2);
         fReadoutChannelConfig = d & 0xFF;
         if (d == 7)
            fNumberOfReadoutChannels = 9;
         else
            fNumberOfReadoutChannels = 5;
      } else
         fNumberOfReadoutChannels = 9;
   } else
      fNumberOfReadoutChannels = 10;

   if (fBoardType == 1) {
      fDAC_COFSA = 0;
      fDAC_COFSB = 1;
      fDAC_DRA = 2;
      fDAC_DSA = 3;
      fDAC_TLEVEL = 4;
      fDAC_ACALIB = 5;
      fDAC_DSB = 6;
      fDAC_DRB = 7;
   } else if (fBoardType == 2 || fBoardType == 3) {
      fDAC_COFS = 0;
      fDAC_DSA = 1;
      fDAC_DSB = 2;
      fDAC_TLEVEL = 3;
      fDAC_CLKOFS = 5;
      fDAC_ACALIB = 6;
      fDAC_ADCOFS = 7;
   } else if (fBoardType == 4) {
      fDAC_ROFS_1 = 0;
      fDAC_DSA = 1;
      fDAC_DSB = 2;
      fDAC_ROFS_2 = 3;
      fDAC_BIAS = 4;
      fDAC_INOFS = 5;
      fDAC_ACALIB = 6;
      fDAC_ADCOFS = 7;
   } else if (fBoardType == 5) {
      fDAC_ROFS_1 = 0;
      fDAC_CMOFS = 1;
      fDAC_CALN = 2;
      fDAC_CALP = 3;
      fDAC_BIAS = 4;
      fDAC_TLEVEL = 5;
      fDAC_ONOFS = 6;
   } else if (fBoardType == 6) {
      fDAC_ONOFS = 0;
      fDAC_CMOFSP = 1;
      fDAC_CALN = 2;
      fDAC_CALP = 3;
      fDAC_CMOFSN = 5;
      fDAC_ROFS_1 = 6;
      fDAC_BIAS = 7;
   } else if (fBoardType == 7) {
      fDAC_ROFS_1 = 0;
      fDAC_CMOFS = 1;
      fDAC_CALN = 2;
      fDAC_CALP = 3;
      fDAC_BIAS = 4;
      fDAC_TLEVEL = 5;
      fDAC_ONOFS = 6;
   } else if (fBoardType == 8 || fBoardType == 9) {
      fDAC_ROFS_1 = 0;
      fDAC_TLEVEL4 = 1;
      fDAC_CALN = 2;
      fDAC_CALP = 3;
      fDAC_BIAS = 4;
      fDAC_TLEVEL1 = 5;
      fDAC_TLEVEL2 = 6;
      fDAC_TLEVEL3 = 7;
   }
   
   if (fDRSType < 4) {
      // Response Calibration
      fResponseCalibration = new ResponseCalibration(this);

      // Time Calibration
      fTimeData = new DRSBoard::TimeData *[kNumberOfChipsMax];
      fNumberOfTimeData = 0;
   }
}

/*------------------------------------------------------------------*/

void DRSBoard::ReadSerialNumber()
{
   unsigned char buffer[2];
   int number;

   // check magic number
   if (Read(T_STATUS, buffer, REG_MAGIC, 2) < 0) {
      printf("Cannot read from board\n");
      return;
   }

   number = (static_cast < int >(buffer[1]) << 8) +buffer[0];
   if (number != 0xC0DE) {
      printf("Invalid magic number: %04X\n", number);
      return;
   }
   
   // read board type
   Read(T_STATUS, buffer, REG_BOARD_TYPE, 2);
   fDRSType = buffer[0];
   fBoardType = buffer[1];

   // read firmware version
   Read(T_STATUS, buffer, REG_VERSION_FW, 2);
   fFirmwareVersion = (static_cast < int >(buffer[1]) << 8) +buffer[0];

   // retrieve board serial number
   Read(T_STATUS, buffer, REG_SERIAL_BOARD, 2);
   number = (static_cast < int >(buffer[1]) << 8) +buffer[0];
   fBoardSerialNumber = number;

   // determine DRS type and board type for old boards from setial number
   if (fBoardType == 0) {
      // determine board version from serial number
      if (number >= 2000 && number < 5000) {
         fBoardType = 6;
         fDRSType = 4;
      } else if (number >= 1000) {
         fBoardType = 4;
         fDRSType = 3;
      } else if (number >= 100)
         fBoardType = 3;
      else if (number > 0)
         fBoardType = 2;
      else {
         fBoardType = 3;
         fDRSType = 2;
         fRequiredFirmwareVersion = REQUIRED_FIRMWARE_VERSION_DRS2;
      }
   }

   // set constants according to board type
   if (fBoardType == 6)
      fNumberOfChips = 4;
   else
      fNumberOfChips = 1;

   if (fDRSType == 4)
      fNumberOfChannels = 9;
   else
      fNumberOfChannels = 10;

   // retrieve firmware version
   if (fDRSType == 2)
      fRequiredFirmwareVersion = REQUIRED_FIRMWARE_VERSION_DRS2;
   if (fDRSType == 3)
      fRequiredFirmwareVersion = REQUIRED_FIRMWARE_VERSION_DRS3;
   if (fDRSType == 4)
      fRequiredFirmwareVersion = REQUIRED_FIRMWARE_VERSION_DRS4;

   fHasMultiBuffer = ((fBoardType == 6) && fTransport == TR_VME);
}

/*------------------------------------------------------------------*/

void DRSBoard::ReadCalibration(void)
{
   unsigned short buf[1024*16]; // 32 kB
   int i, j, chip;

   fVoltageCalibrationValid = false;
   fTimingCalibratedFrequency = 0;

   memset(fCellOffset,  0, sizeof(fCellOffset));
   memset(fCellGain,    0, sizeof(fCellGain));
   memset(fCellOffset2, 0, sizeof(fCellOffset2));
   memset(fCellDT,      0, sizeof(fCellDT));

   /* read offsets and gain from eeprom */
   if (fBoardType == 9) {
      memset(buf, 0, sizeof(buf));
      ReadEEPROM(0, buf, 4096);
      
      /* check voltage calibration method */
      if ((buf[2] & 0xFF) == VCALIB_METHOD)
         fVoltageCalibrationValid = true;
      else {
         fCellCalibratedRange = 0;
         fCellCalibratedTemperature = -100;
         return;
      }
      
      /* check timing calibration method */
      if ((buf[2] >> 8) == TCALIB_METHOD) {
         float fl; // float from two 16-bit integers
         memcpy(&fl, &buf[8], sizeof(float));
         fTimingCalibratedFrequency = fl;
      } else
         fTimingCalibratedFrequency = -1;
      
      fCellCalibratedRange = ((int) (buf[10] & 0xFF)) / 100.0; // -50 ... +50 => -0.5 V ... +0.5 V
      fCellCalibratedTemperature = (buf[10] >> 8) / 2.0;
      
      ReadEEPROM(1, buf, 1024*32);
      for (i=0 ; i<8 ; i++)
         for (j=0 ; j<1024; j++) {
            fCellOffset[i][j] = buf[(i*1024+j)*2];
            fCellGain[i][j]   = buf[(i*1024+j)*2 + 1]/65535.0*0.4+0.7;
         }
      
      ReadEEPROM(2, buf, 1024*32);
      for (i=0 ; i<8 ; i++)
         for (j=0 ; j<1024; j++)
            fCellOffset2[i][j]   = buf[(i*1024+j)*2];
      
   } else if (fBoardType == 5 || fBoardType == 7 || fBoardType == 8) {
      memset(buf, 0, sizeof(buf));
      ReadEEPROM(0, buf, 32);

      /* check voltage calibration method */
      if ((buf[2] & 0xFF) == VCALIB_METHOD_V4) // board < 9 has "1", board 9 has "2"
         fVoltageCalibrationValid = true;
      else {
         fCellCalibratedRange = 0;
         return;
      }
      fCellCalibratedTemperature = -100;

      /* check timing calibration method */
      if ((buf[4] & 0xFF) == TCALIB_METHOD_V4) { // board < 9 has "1", board 9 has "2"
         fTimingCalibratedFrequency = buf[6] / 1000.0;
      } else
         fTimingCalibratedFrequency = -1;

      fCellCalibratedRange = ((int) (buf[2] >> 8)) / 100.0; // -50 ... +50 => -0.5 V ... +0.5 V
      ReadEEPROM(1, buf, 1024*32);
      for (i=0 ; i<8 ; i++)
         for (j=0 ; j<1024; j++) {
            fCellOffset[i][j] = buf[(i*1024+j)*2];
            fCellGain[i][j]   = buf[(i*1024+j)*2 + 1]/65535.0*0.4+0.7;
         }

      ReadEEPROM(2, buf, 1024*5*4);
      for (i=0 ; i<1 ; i++)
         for (j=0 ; j<1024; j++) {
            fCellOffset[i+8][j] = buf[(i*1024+j)*2];
            fCellGain[i+8][j]   = buf[(i*1024+j)*2 + 1]/65535.0*0.4+0.7;
         }

      for (i=0 ; i<4 ; i++)
         for (j=0 ; j<1024; j++) {
            fCellOffset2[i*2][j]   = buf[2*1024+(i*1024+j)*2];
            fCellOffset2[i*2+1][j] = buf[2*1024+(i*1024+j)*2+1];
         }

   } else if (fBoardType == 6) {
      ReadEEPROM(0, buf, 16);

      /* check voltage calibration method */
      if ((buf[2] & 0xFF) == VCALIB_METHOD)
         fVoltageCalibrationValid = true;
      else {
         fCellCalibratedRange = 0;
         return;
      }

      /* check timing calibration method */
      if ((buf[4] & 0xFF) == TCALIB_METHOD)
         fTimingCalibratedFrequency = buf[6] / 1000.0; // 0 ... 6000 => 0 ... 6 GHz
      else
         fTimingCalibratedFrequency = 0;

      fCellCalibratedRange = ((int) (buf[2] >> 8)) / 100.0; // -50 ... +50 => -0.5 V ... +0.5 V

      for (chip=0 ; chip<4 ; chip++) {
         ReadEEPROM(1+chip, buf, 1024*32);
         for (i=0 ; i<8 ; i++)
            for (j=0 ; j<1024; j++) {
               fCellOffset[i+chip*9][j] = buf[(i*1024+j)*2];
               fCellGain[i+chip*9][j]   = buf[(i*1024+j)*2 + 1]/65535.0*0.4+0.7;
            }
      }

      ReadEEPROM(5, buf, 1024*4*4);
      for (chip=0 ; chip<4 ; chip++)
         for (j=0 ; j<1024; j++) {
            fCellOffset[8+chip*9][j] = buf[j*2+chip*0x0800];
            fCellGain[8+chip*9][j]   = buf[j*2+1+chip*0x0800]/65535.0*0.4+0.7;
         }

      ReadEEPROM(7, buf, 1024*32);
      for (i=0 ; i<8 ; i++) {
         for (j=0 ; j<1024; j++) {
            fCellOffset2[i][j]   = buf[i*0x800 + j*2];
            fCellOffset2[i+9][j] = buf[i*0x800 + j*2+1];
         }
      }

      ReadEEPROM(8, buf, 1024*32);
      for (i=0 ; i<8 ; i++) {
         for (j=0 ; j<1024; j++) {
            fCellOffset2[i+18][j] = buf[i*0x800 + j*2];
            fCellOffset2[i+27][j] = buf[i*0x800 + j*2+1];
         }
      }

   } else
      return;

   /* read timing calibration from eeprom */
   if (fBoardType == 9) {
      if (fTimingCalibratedFrequency == 0) {
         for (i=0 ; i<8 ; i++)
            for (j=0 ; j<1024 ; j++) {
               fCellDT[0][i][j] = 1/fNominalFrequency;
            }
      } else {
         ReadEEPROM(2, buf, 1024*32);
         for (i=0 ; i<8 ; i++)
            for (j=0 ; j<1024; j++) {
               fCellDT[0][i][j]   = (buf[(i*1024+j)*2+1] - 1000) / 10000.0;
            }
      }
   } else if (fBoardType == 5 || fBoardType == 7 || fBoardType == 8) {
      if (fTimingCalibratedFrequency == 0) {
         for (i=0 ; i<1024 ; i++)
            fCellDT[0][0][i] = 1/fNominalFrequency;
      } else {
         ReadEEPROM(0, buf, 1024*sizeof(short)*2);
         for (i=0 ; i<8 ; i++) {
            for (j=0 ; j<1024; j++) {
               // use calibration for all channels
               fCellDT[0][i][j] = buf[j*2+1]/10000.0;
            }
         }
      }
   } else if (fBoardType == 6) {
      if (fTimingCalibratedFrequency == 0) {
         for (i=0 ; i<1024 ; i++)
            for (j=0 ; j<4 ; j++)
               fCellDT[0][j][i] = 1/fNominalFrequency;
      } else {
         ReadEEPROM(6, buf, 1024*sizeof(short)*4);
         for (i=0 ; i<1024; i++) {
            fCellDT[0][0][i] = buf[i*2]/10000.0;
            fCellDT[1][0][i] = buf[i*2+1]/10000.0;
            fCellDT[2][0][i] = buf[i*2+0x800]/10000.0;
            fCellDT[3][0][i] = buf[i*2+0x800+1]/10000.0;
         }
      }
   }
   
#if 0
   /* Read Daniel's file */
   int fh = open("cal_ch2.dat", O_RDONLY);
   float v;
   read(fh, &v, sizeof(float));
   for (i=0 ; i<1024 ; i++) {
      read(fh, &v, sizeof(float));
      fCellDT[0][2][(i+0) % 1024] = v;
   }
   close(fh);
   fh = open("cal_ch4.dat", O_RDONLY);
   read(fh, &v, sizeof(float));
   for (i=0 ; i<1024 ; i++) {
      read(fh, &v, sizeof(float));
      fCellDT[0][6][(i+0)%1024] = v;
   }
   close(fh);
#endif

#if 0
   /* write timing calibration to EEPROM page 0 */
   double t1, t2;
   ReadEEPROM(0, buf, sizeof(buf));
   for (i=0,t1=0 ; i<1024; i++) {
      t2 = fCellT[0][i] - t1;
      t2 = (unsigned short) (t2 * 10000 + 0.5);
      t1 += t2 / 10000.0;
      buf[i*2+1] = (unsigned short) t2;
   }
   
   /* write calibration method and frequency */
   buf[4] = TCALIB_METHOD;
   buf[6] = (unsigned short) (fNominalFrequency * 1000 + 0.5);
   fTimingCalibratedFrequency = buf[6] / 1000.0;
   WriteEEPROM(0, buf, sizeof(buf));
#endif
   
}

/*------------------------------------------------------------------*/

bool DRSBoard::HasCorrectFirmware()
{
   /* check for required firmware version */
   return (fFirmwareVersion >= fRequiredFirmwareVersion);
}

/*------------------------------------------------------------------*/

int DRSBoard::InitFPGA(void)
{

#ifdef HAVE_USB
   if (fTransport == TR_USB2) {
      unsigned char buffer[1];
      int i, status;

      /* blink Cy7C68013A LED and issue an FPGA reset */
      buffer[0] = 0;            // LED off
      musb_write(fUsbInterface, 1, buffer, 1, 100);
      Sleep(50);

      buffer[0] = 1;            // LED on
      musb_write(fUsbInterface, 1, buffer, 1, 100);

      /* wait until EEPROM page #0 has been read */
      for (i=0 ; i<100 ; i++) {
         Read(T_STATUS, &status, REG_STATUS, 4);
         if ((status & BIT_SERIAL_BUSY) == 0)
            break;
         Sleep(10);
      }
   }
#endif

   return 1;
}

/*------------------------------------------------------------------*/

/* Generic read function accessing VME or USB */

int DRSBoard::Write(int type, unsigned int addr, void *data, int size)
{
#ifdef USE_DRS_MUTEX
   if (!s_drsMutex) {
      s_drsMutex = new wxMutex();
      assert(s_drsMutex);
   }
   s_drsMutex->Lock();
#endif

   if (fTransport == TR_VME) {

#ifdef HAVE_VME
      unsigned int base_addr;

      base_addr = fBaseAddress;

      if (type == T_CTRL)
         base_addr += PMC_CTRL_OFFSET;
      else if (type == T_STATUS)
         base_addr += PMC_STATUS_OFFSET;
      else if (type == T_RAM)
         base_addr += PMC_RAM_OFFSET;

      if (size == 1) {
         /* 8-bit write access */
         mvme_set_dmode(fVmeInterface, MVME_DMODE_D8);
         mvme_write(fVmeInterface, base_addr + addr, static_cast < mvme_locaddr_t * >(data), size);
      } else if (size == 2) {
         /* 16-bit write access */
         mvme_set_dmode(fVmeInterface, MVME_DMODE_D16);
         mvme_write(fVmeInterface, base_addr + addr, static_cast < mvme_locaddr_t * >(data), size);
      } else {
         mvme_set_dmode(fVmeInterface, MVME_DMODE_D32);

         /* as long as no block transfer is supported, do pseudo block transfer */
         mvme_set_blt(fVmeInterface, MVME_BLT_NONE);

         mvme_write(fVmeInterface, base_addr + addr, static_cast < mvme_locaddr_t * >(data), size);
      }

#ifdef USE_DRS_MUTEX
      s_drsMutex->Unlock();
#endif
      return size;
#endif                          // HAVE_VME

   } else if (fTransport == TR_USB) {
#ifdef HAVE_USB
      unsigned char buffer[64], ack;
      unsigned int base_addr;
      int i, j, n;

      if (type == T_CTRL)
         base_addr = USB_CTRL_OFFSET;
      else if (type == T_STATUS)
         base_addr = USB_STATUS_OFFSET;
      else if (type == T_RAM)
         base_addr = USB_RAM_OFFSET;
      else
         base_addr = 0;

      if (type != T_RAM) {

         /*---- register access ----*/

         if (size == 2) {
            /* word swapping: first 16 bit sit at upper address */
            if ((addr % 4) == 0)
               addr = addr + 2;
            else
               addr = addr - 2;
         }

         buffer[0] = USB_CMD_WRITE;
         buffer[1] = base_addr + addr;
         buffer[2] = size;

         for (i = 0; i < size; i++)
            buffer[3 + i] = *((unsigned char *) data + i);

         /* try 10 times */
         ack = 0;
         for (i = 0; i < 10; i++) {
            n = musb_write(fUsbInterface, 2, buffer, 3 + size, USB_TIMEOUT);
            if (n == 3 + size) {
               for (j = 0; j < 10; j++) {
                  /* wait for acknowledge */
                  n = musb_read(fUsbInterface, 1, &ack, 1, USB_TIMEOUT);
                  if (n == 1 && ack == 1)
                     break;

                  printf("Redo receive\n");
               }
            }

            if (ack == 1) {
#ifdef USE_DRS_MUTEX
               s_drsMutex->Unlock();
#endif
               return size;
            }

            printf("Redo send\n");
         }
      } else {

         /*---- RAM access ----*/

         buffer[0] = USB_CMD_ADDR;
         buffer[1] = base_addr + addr;
         musb_write(fUsbInterface, 2, buffer, 2, USB_TIMEOUT);

         /* chop buffer into 60-byte packets */
         for (i = 0; i <= (size - 1) / 60; i++) {
            n = size - i * 60;
            if (n > 60)
               n = 60;
            buffer[0] = USB_CMD_WRITE12;
            buffer[1] = n;

            for (j = 0; j < n; j++)
               buffer[2 + j] = *((unsigned char *) data + j + i * 60);

            musb_write(fUsbInterface, 2, buffer, 2 + n, USB_TIMEOUT);

            for (j = 0; j < 10; j++) {
               /* wait for acknowledge */
               n = musb_read(fUsbInterface, 1, &ack, 1, USB_TIMEOUT);
               if (n == 1 && ack == 1)
                  break;

               printf("Redo receive acknowledge\n");
            }
         }

#ifdef USE_DRS_MUTEX
         s_drsMutex->Unlock();
#endif
         return size;
      }
#endif                          // HAVE_USB
   } else if (fTransport == TR_USB2) {
#ifdef HAVE_USB
      unsigned int base_addr;
      int i;

      if (usb2_buffer == NULL)
         usb2_buffer = (unsigned char *) malloc(USB2_BUFFER_SIZE);
      assert(usb2_buffer);

      /* only accept even address and number of bytes */
      assert(addr % 2 == 0);
      assert(size % 2 == 0);

      /* check for maximum size */
      assert(size <= USB2_BUFFER_SIZE - 10);

      if (type == T_CTRL)
         base_addr = USB2_CTRL_OFFSET;
      else if (type == T_STATUS)
         base_addr = USB2_STATUS_OFFSET;
      else if (type == T_FIFO)
         base_addr = USB2_FIFO_OFFSET;
      else if (type == T_RAM)
         base_addr = USB2_RAM_OFFSET;
      else
         base_addr = 0;

      if (type != T_RAM && size == 2) {
         /* word swapping: first 16 bit sit at upper address */
         if ((addr % 4) == 0)
            addr = addr + 2;
         else
            addr = addr - 2;
      }

      addr += base_addr;

      usb2_buffer[0] = USB2_CMD_WRITE;
      usb2_buffer[1] = 0;

      usb2_buffer[2] = (addr >> 0) & 0xFF;
      usb2_buffer[3] = (addr >> 8) & 0xFF;
      usb2_buffer[4] = (addr >> 16) & 0xFF;
      usb2_buffer[5] = (addr >> 24) & 0xFF;

      usb2_buffer[6] = (size >> 0) & 0xFF;
      usb2_buffer[7] = (size >> 8) & 0xFF;
      usb2_buffer[8] = (size >> 16) & 0xFF;
      usb2_buffer[9] = (size >> 24) & 0xFF;

      for (i = 0; i < size; i++)
         usb2_buffer[10 + i] = *((unsigned char *) data + i);

      i = musb_write(fUsbInterface, 4, usb2_buffer, 10 + size, USB_TIMEOUT);
      if (i != 10 + size)
         printf("musb_write error: %d\n", i);

#ifdef USE_DRS_MUTEX
      s_drsMutex->Unlock();
#endif
      return i;
#endif                          // HAVE_USB
   }

#ifdef USE_DRS_MUTEX
   s_drsMutex->Unlock();
#endif
   return 0;
}

/*------------------------------------------------------------------*/

/* Generic read function accessing VME or USB */

int DRSBoard::Read(int type, void *data, unsigned int addr, int size)
{
#ifdef USE_DRS_MUTEX
   if (!s_drsMutex) {
      s_drsMutex = new wxMutex();
      assert(s_drsMutex);
   }
   s_drsMutex->Lock();
#endif

   memset(data, 0, size);
 
   if (fTransport == TR_VME) {

#ifdef HAVE_VME
      unsigned int base_addr;
      int n, i;

      base_addr = fBaseAddress;

      if (type == T_CTRL)
         base_addr += PMC_CTRL_OFFSET;
      else if (type == T_STATUS)
         base_addr += PMC_STATUS_OFFSET;
      else if (type == T_RAM)
         base_addr += PMC_RAM_OFFSET;
      else if (type == T_FIFO)
         base_addr += PMC_FIFO_OFFSET;

      mvme_set_dmode(fVmeInterface, MVME_DMODE_D32);

      n = 0;
      if (size == 1) {
         /* 8-bit read access */
         mvme_set_dmode(fVmeInterface, MVME_DMODE_D8);
         n = mvme_read(fVmeInterface, static_cast < mvme_locaddr_t * >(data), base_addr + addr, size);
      } else if (size == 2) {
         /* 16-bit read access */
         mvme_set_dmode(fVmeInterface, MVME_DMODE_D16);
         n = mvme_read(fVmeInterface, static_cast < mvme_locaddr_t * >(data), base_addr + addr, size);
      } else {
         mvme_set_dmode(fVmeInterface, MVME_DMODE_D32);

         //mvme_set_blt(fVmeInterface, MVME_BLT_NONE); // pseudo block transfer
         mvme_set_blt(fVmeInterface, MVME_BLT_2EVME);   // 2eVME if implemented
         n = mvme_read(fVmeInterface, static_cast < mvme_locaddr_t * >(data), base_addr + addr, size);
         while (n != size) {
            printf("Only read %d out of %d, retry with %d: ", n, size, size - n);
            i = mvme_read(fVmeInterface, static_cast < mvme_locaddr_t * >(data) + n / 4, base_addr + addr + n,
                          size - n);
            printf("read %d\n", i);
            if (i == 0) {
               printf("Error reading VME\n");
               return n;
            }
            n += i;
         }

         //for (i = 0; i < size; i += 4)
         //   mvme_read(fVmeInterface, (mvme_locaddr_t *)((char *)data+i), base_addr + addr+i, 4);
      }

#ifdef USE_DRS_MUTEX
      s_drsMutex->Unlock();
#endif

      return n;

#endif                          // HAVE_VME
   } else if (fTransport == TR_USB) {
#ifdef HAVE_USB
      unsigned char buffer[64];
      unsigned int base_addr;
      int i, j, ret, n;

      if (type == T_CTRL)
         base_addr = USB_CTRL_OFFSET;
      else if (type == T_STATUS)
         base_addr = USB_STATUS_OFFSET;
      else if (type == T_RAM)
         base_addr = USB_RAM_OFFSET;
      else
         assert(0);             // FIFO not implemented

      if (type != T_RAM) {

         /*---- register access ----*/

         if (size == 2) {
            /* word swapping: first 16 bit sit at uppder address */
            if ((addr % 4) == 0)
               addr = addr + 2;
            else
               addr = addr - 2;
         }

         buffer[0] = USB_CMD_READ;
         buffer[1] = base_addr + addr;
         buffer[2] = size;

         musb_write(fUsbInterface, 2, buffer, 2 + size, USB_TIMEOUT);
         i = musb_read(fUsbInterface, 1, data, size, USB_TIMEOUT);

#ifdef USE_DRS_MUTEX
         s_drsMutex->Unlock();
#endif
         if (i != size)
            return 0;

         return size;
      } else {

         /*---- RAM access ----*/

         /* in RAM mode, only the 2048-byte page can be selected */
         buffer[0] = USB_CMD_ADDR;
         buffer[1] = base_addr + (addr >> 11);
         musb_write(fUsbInterface, 2, buffer, 2, USB_TIMEOUT);

         /* receive data in 60-byte packets */
         for (i = 0; i <= (size - 1) / 60; i++) {
            n = size - i * 60;
            if (n > 60)
               n = 60;
            buffer[0] = USB_CMD_READ12;
            buffer[1] = n;
            musb_write(fUsbInterface, 2, buffer, 2, USB_TIMEOUT);

            ret = musb_read(fUsbInterface, 1, buffer, n, USB_TIMEOUT);

            if (ret != n) {
               /* try again */
               ret = musb_read(fUsbInterface, 1, buffer, n, USB_TIMEOUT);
               if (ret != n) {
#ifdef USE_DRS_MUTEX
                  s_drsMutex->Unlock();
#endif
                  return 0;
               }
            }

            for (j = 0; j < ret; j++)
               *((unsigned char *) data + j + i * 60) = buffer[j];
         }

#ifdef USE_DRS_MUTEX
         s_drsMutex->Unlock();
#endif
         return size;
      }
#endif                          // HAVE_USB
   } else if (fTransport == TR_USB2) {
#ifdef HAVE_USB
      unsigned char buffer[10];
      unsigned int base_addr;
      int i;

      /* only accept even address and number of bytes */
      assert(addr % 2 == 0);
      assert(size % 2 == 0);

      /* check for maximum size */
      assert(size <= USB2_BUFFER_SIZE - 10);

      if (type == T_CTRL)
         base_addr = USB2_CTRL_OFFSET;
      else if (type == T_STATUS)
         base_addr = USB2_STATUS_OFFSET;
      else if (type == T_FIFO)
         base_addr = USB2_FIFO_OFFSET;
      else if (type == T_RAM)
         base_addr = USB2_RAM_OFFSET;
      else
         base_addr = 0;

      if (type != T_RAM && size == 2) {
         /* word swapping: first 16 bit sit at upper address */
         if ((addr % 4) == 0)
            addr = addr + 2;
         else
            addr = addr - 2;
      }

      addr += base_addr;

      buffer[0] = USB2_CMD_READ;
      buffer[1] = 0;

      buffer[2] = (addr >> 0) & 0xFF;
      buffer[3] = (addr >> 8) & 0xFF;
      buffer[4] = (addr >> 16) & 0xFF;
      buffer[5] = (addr >> 24) & 0xFF;

      buffer[6] = (size >> 0) & 0xFF;
      buffer[7] = (size >> 8) & 0xFF;
      buffer[8] = (size >> 16) & 0xFF;
      buffer[9] = (size >> 24) & 0xFF;

      i = musb_write(fUsbInterface, 4, buffer, 10, USB_TIMEOUT);
      if (i != 10)
         printf("musb_read error %d\n", i);

      i = musb_read(fUsbInterface, 8, data, size, USB_TIMEOUT);
#ifdef USE_DRS_MUTEX
      s_drsMutex->Unlock();
#endif
      return i;
#endif                          // HAVE_USB
   }

#ifdef USE_DRS_MUTEX
   s_drsMutex->Unlock();
#endif
   return 0;
}

/*------------------------------------------------------------------*/

void DRSBoard::SetLED(int state)
{
   // Set LED state
   if (state)
      fCtrlBits |= BIT_LED;
   else
      fCtrlBits &= ~BIT_LED;
   Write(T_CTRL, REG_CTRL, &fCtrlBits, 4);
}

/*------------------------------------------------------------------*/

int DRSBoard::SetChannelConfig(int firstChannel, int lastChannel, int nConfigChannels)
{
   unsigned short d;

   if (lastChannel < 0 || lastChannel > 10) {
      printf("Invalid number of channels: %d (must be between 0 and 10)\n", lastChannel);
      return 0;
   }

   // Set number of channels
   if (fDRSType == 2) {
      // register must contain last channel to read out starting from 9
      d = 9 - lastChannel;
      Write(T_CTRL, REG_CHANNEL_CONFIG, &d, 2);
   } else if (fDRSType == 3) {
      // upper four bits of register must contain last channel to read out starting from 9
      d = (firstChannel << 4) | lastChannel;
      Write(T_CTRL, REG_CHANNEL_MODE, &d, 2);

      // set bit pattern for write shift register
      switch (nConfigChannels) {
      case 1:
         d = 0x001;
         break;
      case 2:
         d = 0x041;
         break;
      case 3:
         d = 0x111;
         break;
      case 4:
         d = 0x249;
         break;
      case 6:
         d = 0x555;
         break;
      case 12:
         d = 0xFFF;
         break;
      default:
         printf("Invalid channel configuration\n");
         return 0;
      }
      Write(T_CTRL, REG_CHANNEL_CONFIG, &d, 2);
   } else if (fDRSType == 4) {
      
      int oldMultiBuffer = fMultiBuffer;

      // make sure FPGA state machine is idle
      if (fHasMultiBuffer) {
         SetMultiBuffer(0);
         if (IsBusy()) {
            SoftTrigger();
            while (IsBusy());
         }
      }

      if (fBoardType == 6) {
         // determined channel readout mode A/C[even/odd], B/D[even/odd] or A/B/C/D
         fReadoutChannelConfig = firstChannel;
         Read(T_CTRL, &d, REG_CHANNEL_MODE, 2);
         d = (d & 0xFF00) | firstChannel; // keep higher 8 bits which are ADClkPhase
         Write(T_CTRL, REG_CHANNEL_MODE, &d, 2);
      } else {
         // upper four bits of register must contain last channel to read out starting from 9
         Read(T_CTRL, &d, REG_CHANNEL_MODE, 2);
         d = (d & 0xFF00) | (firstChannel << 4) | lastChannel; // keep higher 8 bits which are ADClkPhase
         Write(T_CTRL, REG_CHANNEL_MODE, &d, 2);
      }

      // set bit pattern for write shift register
      fChannelConfig = 0;
      switch (nConfigChannels) {
      case 1:
         fChannelConfig = 0x01;
         fChannelCascading = 8;
         break;
      case 2:
         fChannelConfig = 0x11;
         fChannelCascading = 4;
         break;
      case 4:
         fChannelConfig = 0x55;
         fChannelCascading = 2;
         break;
      case 8:
         fChannelConfig = 0xFF;
         fChannelCascading = 1;
         break;
      default:
         printf("Invalid channel configuration\n");
         return 0;
      }
      d = fChannelConfig | (fDominoMode << 8) | (1 << 9) | (fWSRLoop << 10) | (0xF8 << 8);

      Write(T_CTRL, REG_CHANNEL_CONFIG, &d, 2);

      fChannelDepth = fChannelCascading * (fDecimation ? kNumberOfBins/2 : kNumberOfBins);

      if (fHasMultiBuffer && oldMultiBuffer) {
         Reinit(); // set WP=0
         SetMultiBuffer(oldMultiBuffer);
         SetMultiBufferRP(0);
      }
   }

   if (fBoardType == 6) {
      if (fReadoutChannelConfig == 7)
         fNumberOfReadoutChannels = 9;
      else
         fNumberOfReadoutChannels = 5;
   } else {
      fNumberOfReadoutChannels = lastChannel - firstChannel + 1;
   }

   return 1;
}

/*------------------------------------------------------------------*/

void DRSBoard::SetNumberOfChannels(int nChannels)
{
   SetChannelConfig(0, nChannels - 1, 12);
}

/*------------------------------------------------------------------*/

void DRSBoard::SetADCClkPhase(int phase, bool invert)
{
   unsigned short d = 0;

   /* Set the clock phase of the ADC via the variable phase shift
      in the Xilinx DCM. One unit is equal to the clock period / 256,
      so at 30 MHz this is about 130ps. The possible range at 30 MHz
      is -87 ... +87 */

   // keep lower 8 bits which are the channel mode
   Read(T_CTRL, &d, REG_ADCCLK_PHASE, 2);
   d = (d & 0x00FF) | (phase << 8);
   Write(T_CTRL, REG_ADCCLK_PHASE, &d, 2);

   if (invert)
      fCtrlBits |= BIT_ADCCLK_INVERT;
   else
      fCtrlBits &= ~BIT_ADCCLK_INVERT;
   Write(T_CTRL, REG_CTRL, &fCtrlBits, 4);

   fADCClkPhase = phase;
   fADCClkInvert = invert;
}

/*------------------------------------------------------------------*/

void DRSBoard::SetWarmup(unsigned int microseconds)
{
   /* Set the "warmup" time. When starting the domino wave, the DRS4
      chip together with its power supply need some time to stabilize
      before high resolution data can be taken (jumping baseline
      problem). This sets the time in ticks of 900ns before triggers
      are accepted */

   unsigned short ticks;

   if (microseconds == 0)
      ticks = 0;
   else
      ticks = (unsigned short) (microseconds / 0.9 + 0.5) - 1;
   Write(T_CTRL, REG_WARMUP, &ticks, 2);
}

/*------------------------------------------------------------------*/

void DRSBoard::SetCooldown(unsigned int microseconds)
{
   /* Set the "cooldown" time. When stopping the domino wave, the 
      power supply needs some time to stabilize before high resolution 
      data can read out (slanted baseline problem). This sets the 
      time in ticks of 900 ns before the readout is started */

   unsigned short ticks;

   ticks = (unsigned short) (microseconds / 0.9 + 0.5) - 1;
   Write(T_CTRL, REG_COOLDOWN, &ticks, 2);
}

/*------------------------------------------------------------------*/

int DRSBoard::SetDAC(unsigned char channel, double value)
{
   // Set DAC value
   unsigned short d;

   /* normalize to 2.5V for 16 bit */
   if (value < 0)
      value = 0;
   if (value > 2.5)
      value = 2.5;
   d = static_cast < unsigned short >(value / 2.5 * 0xFFFF + 0.5);

   Write(T_CTRL, REG_DAC_OFS + (channel * 2), &d, 2);

   return 1;
}

/*------------------------------------------------------------------*/

int DRSBoard::ReadDAC(unsigned char channel, double *value)
{
   // Readback DAC value from control register
   unsigned char buffer[2];

   /* map 0->1, 1->0, 2->3, 3->2, etc. */
   //ofs = channel + 1 - 2*(channel % 2);

   Read(T_CTRL, buffer, REG_DAC_OFS + (channel * 2), 2);

   /* normalize to 2.5V for 16 bit */
   *value = 2.5 * (buffer[0] + (buffer[1] << 8)) / 0xFFFF;

   return 1;
}

/*------------------------------------------------------------------*/

int DRSBoard::GetRegulationDAC(double *value)
{
   // Get DAC value from status register (-> freq. regulation)
   unsigned char buffer[2];

   if (fBoardType == 1)
      Read(T_STATUS, buffer, REG_RDAC3, 2);
   else if (fBoardType == 2 || fBoardType == 3 || fBoardType == 4)
      Read(T_STATUS, buffer, REG_RDAC1, 2);
   else
     memset(buffer, 0, sizeof(buffer));

   /* normalize to 2.5V for 16 bit */
   *value = 2.5 * (buffer[0] + (buffer[1] << 8)) / 0xFFFF;

   return 1;
}

/*------------------------------------------------------------------*/

int DRSBoard::StartDomino()
{
   // Start domino sampling
   fCtrlBits |= BIT_START_TRIG;
   Write(T_CTRL, REG_CTRL, &fCtrlBits, 4);
   fCtrlBits &= ~BIT_START_TRIG;

   return 1;
}

/*------------------------------------------------------------------*/

int DRSBoard::Reinit()
{
   // Stop domino sampling
   // reset readout state machine
   // reset FIFO counters
   fCtrlBits |= BIT_REINIT_TRIG;
   Write(T_CTRL, REG_CTRL, &fCtrlBits, 4);
   fCtrlBits &= ~BIT_REINIT_TRIG;

   return 1;
}

/*------------------------------------------------------------------*/

int DRSBoard::Init()
{
   // Init FPGA on USB2 board
   InitFPGA();

   // Turn off multi-buffer mode to avoid immediate startup
   SetMultiBuffer(0);

   // Reinitialize
   fCtrlBits |= BIT_REINIT_TRIG;        // reset readout state machine
   if (fDRSType == 2)
      fCtrlBits &= ~BIT_FREQ_AUTO_ADJ;  // turn auto. freq regul. off
   Write(T_CTRL, REG_CTRL, &fCtrlBits, 4);
   fCtrlBits &= ~BIT_REINIT_TRIG;

   if (fBoardType == 1) {
      // set max. domino speed
      SetDAC(fDAC_DRA, 2.5);
      SetDAC(fDAC_DSA, 2.5);
      SetDAC(fDAC_DRB, 2.5);
      SetDAC(fDAC_DSB, 2.5);
      // set readout offset
      SetDAC(fDAC_COFSA, 0.9);
      SetDAC(fDAC_COFSB, 0.9);
      SetDAC(fDAC_TLEVEL, 1.7);
   } else if (fBoardType == 2 || fBoardType == 3) {
      // set max. domino speed
      SetDAC(fDAC_DSA, 2.5);
      SetDAC(fDAC_DSB, 2.5);

      // set readout offset
      SetDAC(fDAC_COFS, 0.9);
      SetDAC(fDAC_TLEVEL, 1.7);
      SetDAC(fDAC_ADCOFS, 1.7); // 1.7 for DC coupling, 1.25 for AC
      SetDAC(fDAC_CLKOFS, 1);
   } else if (fBoardType == 4) {
      // set max. domino speed
      SetDAC(fDAC_DSA, 2.5);
      SetDAC(fDAC_DSB, 2.5);

      // set readout offset
      SetDAC(fDAC_ROFS_1, 1.25);        // LVDS level
      //SetDAC(fDAC_ROFS_2, 0.85);   // linear range  0.1V ... 1.1V
      SetDAC(fDAC_ROFS_2, 1.05);        // differential input from Lecce splitter

      SetDAC(fDAC_ADCOFS, 1.25);
      SetDAC(fDAC_ACALIB, 0.5);
      SetDAC(fDAC_INOFS, 0.6);
      SetDAC(fDAC_BIAS, 0.70);  // a bit above the internal bias of 0.68V
   
   } else if (fBoardType == 5) {
      // DRS4 USB Evaluation Board 1.1 + 2.0

      // set max. domino speed
      SetDAC(fDAC_DSA, 2.5);

      // set readout offset
      fROFS = 1.6;              // differential input range -0.5V ... +0.5V
      fRange = 0;
      SetDAC(fDAC_ROFS_1, fROFS);

      // set common mode offset
      fCommonMode = 0.8;        // 0.8V +- 0.5V inside NMOS range
      SetDAC(fDAC_CMOFS, fCommonMode);

      // calibration voltage
      SetDAC(fDAC_CALP, fCommonMode);
      SetDAC(fDAC_CALN, fCommonMode);

      // OUT- offset
      SetDAC(fDAC_ONOFS, 1.25);

      SetDAC(fDAC_BIAS, 0.70);

   } else if (fBoardType == 6) {
      // DRS4 Mezzanine Board 1.0
      
      // set readout offset
      fROFS = 1.6;              // differential input range -0.5V ... +0.5V
      fRange = 0;
      SetDAC(fDAC_ROFS_1, fROFS);

      // set common mode offset
      fCommonMode = 0.8;        // 0.8V +- 0.5V inside NMOS range
      SetDAC(fDAC_CMOFSP, fCommonMode);
      SetDAC(fDAC_CMOFSN, fCommonMode);

      // calibration voltage
      SetDAC(fDAC_CALN, fCommonMode);
      SetDAC(fDAC_CALP, fCommonMode);

      // OUT- offset
      SetDAC(fDAC_ONOFS, 1.25);

      SetDAC(fDAC_BIAS, 0.70);
   } else if (fBoardType == 7) {
      // DRS4 USB Evaluation 3.0

      // set max. domino speed
      SetDAC(fDAC_DSA, 2.5);

      // set readout offset
      fROFS = 1.6;              // differential input range -0.5V ... +0.5V
      fRange = 0;
      SetDAC(fDAC_ROFS_1, fROFS);

      // set common mode for THS4508
      SetDAC(fDAC_CMOFS, 2.4);

      // calibration voltage
      fCommonMode = 0.8;        // 0.8V +- 0.5V inside NMOS range
      SetDAC(fDAC_CALP, fCommonMode);
      SetDAC(fDAC_CALN, fCommonMode);

      // OUT- offset
      SetDAC(fDAC_ONOFS, 1.25);

      SetDAC(fDAC_BIAS, 0.70);
   } else if (fBoardType == 8 || fBoardType == 9) {
      // DRS4 USB Evaluation 4.0

      // set readout offset
      fROFS = 1.6;              // differential input range -0.5V ... +0.5V
      fRange = 0;
      SetDAC(fDAC_ROFS_1, fROFS);

      // calibration voltage
      fCommonMode = 0.8;        // 0.8V +- 0.5V inside NMOS range
      SetDAC(fDAC_CALP, fCommonMode);
      SetDAC(fDAC_CALN, fCommonMode);

      SetDAC(fDAC_BIAS, 0.70);
   }

   /* set default number of channels per chip */
   if (fDRSType == 4) {
      if (fTransport == TR_USB2)
         SetChannelConfig(0, fNumberOfReadoutChannels - 1, 8);
      else
         SetChannelConfig(7, fNumberOfReadoutChannels - 1, 8);
   } else
      SetChannelConfig(0, fNumberOfReadoutChannels - 1, 12);

   // set ADC clock phase
   if (fBoardType == 5 || fBoardType == 7 || fBoardType == 8 || fBoardType == 9) {
      fADCClkPhase = 0;
      fADCClkInvert = 0;
   } else if (fBoardType == 6) {
      fADCClkPhase = 65;
      fADCClkInvert = 0;
   }

   // default settings
   fMultiBuffer = 0;
   fNMultiBuffer = 0;
   fDominoMode = 1;
   fReadoutMode = 1;
   fReadPointer = 0;
   fTriggerEnable1 = 0;
   fTriggerEnable2 = 0;
   fTriggerSource = 0;
   fTriggerDelay = 0;
   fTriggerDelayNs = 0;
   fSyncDelay = 0;
   fNominalFrequency = 1;
   fDominoActive = 1;

   // load calibration from EEPROM
   ReadCalibration();

   // get some settings from hardware
   fRange = GetCalibratedInputRange();
   if (fRange < 0 || fRange > 0.5)
      fRange = 0;
   fNominalFrequency = GetCalibratedFrequency();
   if (fNominalFrequency < 0.1 || fNominalFrequency > 6)
      fNominalFrequency = 1;

   
   if (fHasMultiBuffer) {
      SetMultiBuffer(fMultiBuffer);
      SetMultiBufferRP(fReadPointer);
   }
   SetDominoMode(fDominoMode);
   SetReadoutMode(fReadoutMode);
   EnableTrigger(fTriggerEnable1, fTriggerEnable2);
   SetTriggerSource(fTriggerSource);
   SetTriggerDelayPercent(0);
   SetSyncDelay(fSyncDelay);
   SetDominoActive(fDominoActive);
   SetFrequency(fNominalFrequency, true);
   SetInputRange(fRange);
   if (fBoardType == 5 || fBoardType == 7 || fBoardType == 8 || fBoardType == 9)
      SelectClockSource(0); // FPGA clock
   if (fBoardType == 6) {
      SetADCClkPhase(fADCClkPhase, fADCClkInvert);
      SetWarmup(0);
      SetCooldown(100);
      SetDecimation(0);
   }

   // disable calibration signals
   EnableAcal(0, 0);
   SetCalibTiming(0, 0);
   EnableTcal(0);

   // got to idle state
   Reinit();

   return 1;
}

/*------------------------------------------------------------------*/

int DRSBoard::SetDominoMode(unsigned char mode)
{
   // Set domino mode
   // mode == 0: single sweep
   // mode == 1: run continously
   //
   fDominoMode = mode;

   if (fDRSType == 4) {
      unsigned short d;
      Read(T_CTRL, &d, REG_CONFIG, 2);
      fChannelConfig = d & 0xFF;

      d = fChannelConfig | (fDominoMode << 8) | (1 << 9) | (fWSRLoop << 10) | (0xF8 << 8);
      Write(T_CTRL, REG_CONFIG, &d, 2);
   } else {
      if (mode)
         fCtrlBits |= BIT_DMODE;
      else
         fCtrlBits &= ~BIT_DMODE;

      Write(T_CTRL, REG_CTRL, &fCtrlBits, 4);
   }

   return 1;
}

/*------------------------------------------------------------------*/

int DRSBoard::SetDominoActive(unsigned char mode)
{
   // Set domino activity
   // mode == 0: stop during readout
   // mode == 1: keep domino wave running
   //
   fDominoActive = mode;
   if (mode)
      fCtrlBits |= BIT_DACTIVE;
   else
      fCtrlBits &= ~BIT_DACTIVE;

   Write(T_CTRL, REG_CTRL, &fCtrlBits, 4);

   return 1;
}

/*------------------------------------------------------------------*/

int DRSBoard::SetReadoutMode(unsigned char mode)
{
   // Set readout mode
   // mode == 0: start from first bin
   // mode == 1: start from domino stop
   //
   fReadoutMode = mode;
   if (mode)
      fCtrlBits |= BIT_READOUT_MODE;
   else
      fCtrlBits &= ~BIT_READOUT_MODE;

   Write(T_CTRL, REG_CTRL, &fCtrlBits, 4);

   return 1;
}

/*------------------------------------------------------------------*/

int DRSBoard::SoftTrigger(void)
{
   // Send a software trigger
   fCtrlBits |= BIT_SOFT_TRIG;
   Write(T_CTRL, REG_CTRL, &fCtrlBits, 4);
   fCtrlBits &= ~BIT_SOFT_TRIG;

   return 1;
}

/*------------------------------------------------------------------*/

int DRSBoard::EnableTrigger(int flag1, int flag2)
{
   // Enable external trigger
   fTriggerEnable1 = flag1;
   fTriggerEnable2 = flag2;
   if (flag1)
      fCtrlBits |= BIT_ENABLE_TRIGGER1;
   else
      fCtrlBits &= ~BIT_ENABLE_TRIGGER1;

   if (flag2)
      fCtrlBits |= BIT_ENABLE_TRIGGER2;
   else
      fCtrlBits &= ~BIT_ENABLE_TRIGGER2;

   Write(T_CTRL, REG_CTRL, &fCtrlBits, 4);

   return 1;
}

/*------------------------------------------------------------------*/

int DRSBoard::SetDelayedTrigger(int flag)
{
   // Select delayed trigger from trigger bus
   if (flag)
      fCtrlBits |= BIT_TRIGGER_DELAYED;
   else
      fCtrlBits &= ~BIT_TRIGGER_DELAYED;

   Write(T_CTRL, REG_CTRL, &fCtrlBits, 4);

   return 1;
}

/*------------------------------------------------------------------*/

int DRSBoard::SetTriggerPolarity(bool negative)
{
   if (fBoardType == 5 || fBoardType == 7) {
      fTcalLevel = negative;
      
      if (negative)
         fCtrlBits |= BIT_NEG_TRIGGER;
      else
         fCtrlBits &= ~BIT_NEG_TRIGGER;
      
      Write(T_CTRL, REG_CTRL, &fCtrlBits, 4);
      
      return 1;
   } else if (fBoardType == 8 || fBoardType == 9) {
      fTcalLevel = negative;
      
      if (negative)
         fCtrlBits |= BIT_NEG_TRIGGER;
      else
         fCtrlBits &= ~BIT_NEG_TRIGGER;
      
      Write(T_CTRL, REG_CTRL, &fCtrlBits, 4);
      return 1;
   }
   
   return 0;
}

/*------------------------------------------------------------------*/

int DRSBoard::SetTriggerLevel(double voltage)
{
   if (fBoardType == 5 || fBoardType == 7) {
      return SetDAC(fDAC_TLEVEL, voltage/2 + 0.8);
   } else if (fBoardType == 8 || fBoardType == 9) {
      SetIndividualTriggerLevel(0, voltage);
      SetIndividualTriggerLevel(1, voltage);
      SetIndividualTriggerLevel(2, voltage);
      SetIndividualTriggerLevel(3, voltage);
      return 1;
   }

   return 0;
}

/*------------------------------------------------------------------*/

int DRSBoard::SetIndividualTriggerLevel(int channel, double voltage)
{
   if (fBoardType == 8 || fBoardType == 9) {
      switch (channel) {
            case 0: SetDAC(fDAC_TLEVEL1, voltage/2 + 0.8); break;
            case 1: SetDAC(fDAC_TLEVEL2, voltage/2 + 0.8); break;
            case 2: SetDAC(fDAC_TLEVEL3, voltage/2 + 0.8); break;
            case 3: SetDAC(fDAC_TLEVEL4, voltage/2 + 0.8); break;
         default: return -1;
      }
   }
   
   return 0;
}

/*------------------------------------------------------------------*/

#define LUT_DELAY_S3_8 6.2  // Spartan 3 Octal LUTs 2 GSPS
#define LUT_DELAY_S3_4 2.1  // Spartan 3 Quad LUTs
#define LUT_DELAY_V2_8 4.6  // Virtex PRO II Octal LUTs
#define LUT_DELAY_V2_4 2.3  // Virtex PRO II Quad LUTs


int DRSBoard::SetTriggerDelayPercent(int delay)
/* set trigger delay in percent 0..100 */
{
   short ticks, reg;
   fTriggerDelay = delay;

   if (fBoardType == 5 || fBoardType == 6 || fBoardType == 7 || fBoardType == 8 || fBoardType == 9) {
      // convert delay (0..100) into ticks
      ticks = (unsigned short) (delay/100.0*255+0.5);
      if (ticks > 255)
         ticks = 255;
      if (ticks < 0)
         ticks = 0;

      // convert delay into ns
      if (fBoardType == 7 || fBoardType == 8 || fBoardType == 9) {
         if (fFirmwareVersion >= 17147)
            fTriggerDelayNs = ticks * LUT_DELAY_S3_8;
         else
            fTriggerDelayNs = ticks * LUT_DELAY_S3_4;
      } else {
         if (fFirmwareVersion >= 17382)
            fTriggerDelayNs = ticks * LUT_DELAY_V2_8;
         else
            fTriggerDelayNs = ticks * LUT_DELAY_V2_4;
      }

      // adjust for fixed delay, measured and approximated experimentally
      fTriggerDelayNs += 23.5 + 28.2/fNominalFrequency;

      Read(T_CTRL, &reg, REG_TRG_DELAY, 2);
      reg = (reg & 0xFF00) | ticks;
      Write(T_CTRL, REG_TRG_DELAY, &ticks, 2);

      return 1;
   }

   return 0;
}

/*------------------------------------------------------------------*/

int DRSBoard::SetTriggerDelayNs(int delay)
/* set trigger delay in nanoseconds */
{
   short ticks, reg;
   fTriggerDelayNs = delay;

   if (fBoardType == 5 || fBoardType == 6 || fBoardType == 7 || fBoardType == 8 || fBoardType == 9) {

      // convert delay in ns into ticks
      if (fBoardType == 7 || fBoardType == 8 || fBoardType == 9) {
         if (fFirmwareVersion >= 17147)
            ticks = (short int)(delay / LUT_DELAY_S3_8 + 0.5);
         else
            ticks = (short int)(delay / LUT_DELAY_S3_4 + 0.5);
      } else {
         if (fFirmwareVersion >= 17382)
            ticks = (short int)(delay / LUT_DELAY_V2_8 + 0.5);
         else
            ticks = (short int)(delay / LUT_DELAY_V2_4 + 0.5);
      }

      if (ticks > 255)
         ticks = 255;
      if (ticks < 0)
         ticks = 0;

      fTriggerDelay = ticks / 255 * 100;

      Read(T_CTRL, &reg, REG_TRG_DELAY, 2);
      reg = (reg & 0xFF00) | ticks;
      Write(T_CTRL, REG_TRG_DELAY, &ticks, 2);

      return 1;
   }

   return 0;
}

/*------------------------------------------------------------------*/

int DRSBoard::SetSyncDelay(int ticks)
{
   short int reg;

   if (fBoardType == 5 || fBoardType == 6 || fBoardType == 7 || fBoardType == 8 || fBoardType == 9) {
      Read(T_CTRL, &reg, REG_TRG_DELAY, 2);
      reg = (reg & 0xFF) | (ticks << 8);
      Write(T_CTRL, REG_TRG_DELAY, &reg, 2);

      return 1;
   }

   return 0;
}

/*------------------------------------------------------------------*/

int DRSBoard::SetTriggerSource(int source)
{
   short int reg;

   fTriggerSource = source;
   if (fBoardType == 5 || fBoardType == 7) {
      // Set trigger source 
      // 0=CH1, 1=CH2, 2=CH3, 3=CH4
      if (source & 1)
         fCtrlBits |= BIT_TR_SOURCE1;
      else
         fCtrlBits &= ~BIT_TR_SOURCE1;
      if (source & 2)
         fCtrlBits |= BIT_TR_SOURCE2;
      else
         fCtrlBits &= ~BIT_TR_SOURCE2;

      Write(T_CTRL, REG_CTRL, &fCtrlBits, 4);
   } else if (fBoardType == 8 || fBoardType == 9) {
      // Set trigger configuration
      // OR  Bit0=CH1, Bit1=CH2,  Bit2=CH3,  Bit3=CH4,  Bit4=EXT
      // AND Bit8=CH1, Bit9=CH2, Bit10=CH3, Bit11=CH4, Bit12=EXT
      // TRANSP Bit15
      reg = (unsigned short) source;
      Write(T_CTRL, REG_TRG_CONFIG, &reg, 2);
   }

   return 1;
}

/*------------------------------------------------------------------*/

int DRSBoard::SetDelayedStart(int flag)
{
   // Enable external trigger
   fDelayedStart = flag;
   if (flag)
      fCtrlBits |= BIT_DELAYED_START;
   else
      fCtrlBits &= ~BIT_DELAYED_START;

   Write(T_CTRL, REG_CTRL, &fCtrlBits, 4);

   return 1;
}

/*------------------------------------------------------------------*/

int DRSBoard::SetTranspMode(int flag)
{
   // Enable/disable transparent mode
   fTranspMode = flag;
   if (flag)
      fCtrlBits |= BIT_TRANSP_MODE;
   else
      fCtrlBits &= ~BIT_TRANSP_MODE;

   Write(T_CTRL, REG_CTRL, &fCtrlBits, 4);

   return 1;
}

/*------------------------------------------------------------------*/

int DRSBoard::SetStandbyMode(int flag)
{
   // Enable/disable standby mode
   fTranspMode = flag;
   if (flag)
      fCtrlBits |= BIT_STANDBY_MODE;
   else
      fCtrlBits &= ~BIT_STANDBY_MODE;

   Write(T_CTRL, REG_CTRL, &fCtrlBits, 4);

   return 1;
}

/*------------------------------------------------------------------*/

int DRSBoard::SetDecimation(int flag)
{
   // Drop every odd sample
   fDecimation = flag;
   if (flag)
      fCtrlBits |= BIT_DECIMATION;
   else
      fCtrlBits &= ~BIT_DECIMATION;

   Write(T_CTRL, REG_CTRL, &fCtrlBits, 4);

   // Calculate channel depth
   fChannelDepth = fChannelCascading * (fDecimation ? kNumberOfBins/2 : kNumberOfBins);

   return 1;
}

/*------------------------------------------------------------------*/

int DRSBoard::IsBusy()
{
   // Get running flag
   unsigned int status;

   Read(T_STATUS, &status, REG_STATUS, 4);
   return (status & BIT_RUNNING) > 0;
}

/*------------------------------------------------------------------*/

int DRSBoard::IsEventAvailable()
{
   if (!fMultiBuffer)
      return !IsBusy();

   return GetMultiBufferWP() != fReadPointer;
}

/*------------------------------------------------------------------*/

int DRSBoard::IsPLLLocked()
{
   // Get running flag
   unsigned int status;

   Read(T_STATUS, &status, REG_STATUS, 4);
   if (GetBoardType() == 6)
      return ((status >> 1) & 0x0F) == 0x0F;
   return (status & BIT_PLL_LOCKED0) > 0;
}

/*------------------------------------------------------------------*/

int DRSBoard::IsLMKLocked()
{
   // Get running flag
   unsigned int status;

   Read(T_STATUS, &status, REG_STATUS, 4);
   if (GetBoardType() == 6)
      return (status & BIT_LMK_LOCKED) > 0;
   return 0;
}

/*------------------------------------------------------------------*/

int DRSBoard::IsNewFreq(unsigned char chipIndex)
{
   unsigned int status;

   Read(T_STATUS, &status, REG_STATUS, 4);
   if (chipIndex == 0)
      return (status & BIT_NEW_FREQ1) > 0;
   return (status & BIT_NEW_FREQ2) > 0;
}

/*------------------------------------------------------------------*/

int DRSBoard::ReadFrequency(unsigned char chipIndex, double *f)
{
   if (fDRSType == 4) {

      if (fBoardType == 6) {
         *f = fNominalFrequency;
         return 1;
      }

      unsigned short ticks;

      Read(T_CTRL, &ticks, REG_FREQ_SET, 2);
      ticks += 2;

      /* convert rounded ticks back to frequency */
      if (ticks > 2)
         *f = 1.024 / ticks * fRefClock;
      else
         *f = 0;
   } else {
      // Read domino sampling frequency
      unsigned char buffer[2];

      if (chipIndex == 0)
         Read(T_STATUS, buffer, REG_FREQ1, 2);
      else
         Read(T_STATUS, buffer, REG_FREQ2, 2);

      *f = (static_cast < unsigned int >(buffer[1]) << 8) +buffer[0];

      /* convert counts to frequency */
      if (*f != 0)
         *f = 1024 * 200 * (32.768E6 * 4) / (*f) / 1E9;
   }

   return 1;
}

/*------------------------------------------------------------------*/

double DRSBoard::VoltToFreq(double volt)
{
   if (fDRSType == 3) {
      if (volt <= 1.2001)
         return (volt - 0.6) / 0.2;
      else
         return 0.73 / 0.28 + sqrt((0.73 / 0.28) * (0.73 / 0.28) - 2.2 / 0.14 + volt / 0.14);
   } else
      return (volt - 0.5) / 0.2;
}

/*------------------------------------------------------------------*/

double DRSBoard::FreqToVolt(double freq)
{
   if (fDRSType == 3) {
      if (freq <= 3)
         return 0.6 + 0.2 * freq;
      else
         return 2.2 - 0.73 * freq + 0.14 * freq * freq;
   } else
      return 0.55 + 0.25 * freq;
}

/*------------------------------------------------------------------*/

int DRSBoard::ConfigureLMK(double sampFreq, bool freqChange, int calFreq, int calPhase)
{
   unsigned int data[] = { 0x80000100,   // RESET=1
                           0x0007FF00,   // CLKOUT0: EN=1, DIV=FF (=510) MUX=Div&Delay
                           0x00000101,   // CLKOUT1: Disabled
                           0x0082000B,   // R11: DIV4=0
                           0x028780AD,   // R13: VCO settings
                           0x0830000E,   // R14: PLL settings
                           0xC000000F }; // R15: PLL settings

   /* calculate dividing ratio */
   int divider, vco_divider, n_counter, r_counter;
   unsigned int status;
   double clk, vco;

   if (fTransport == TR_USB2) {
      /* 30 MHz clock */
      data[4]     = 0x028780AD;  // R13 according to CodeLoader 4
      clk         = 30;
      if (sampFreq < 1) {
         r_counter   = 1;
         vco_divider = 8;
         n_counter   = 5;
      } else {
         r_counter   = 1;
         vco_divider = 5;
         n_counter   = 8;
      }
   } else {
      
      if (fCtrlBits & BIT_REFCLK_SOURCE) {
         /* 19.44 MHz clock */
         data[4]     = 0x0284C0AD;  // R13 according to CodeLoader 4
         clk         = 19.44; // global clock through P2

         r_counter   = 2;
         vco_divider = 8;
         n_counter   = 16;
      } else {
         /* 33 MHz clock */
         data[4]     = 0x028840AD;  // R13 according to CodeLoader 4
         clk         = 33; // FPGA clock

         r_counter   = 2;
         vco_divider = 8;
         n_counter   = 9;
      }
   }

   vco = clk/r_counter*n_counter*vco_divider;
   divider = (int) ((vco / vco_divider / (sampFreq/2.048) / 2.0) + 0.5);

   /* return exact frequency */
   fNominalFrequency = vco/vco_divider/(divider*2)*2.048;

   /* return exact timing calibration frequency */
   fTCALFrequency = vco/vco_divider;

   /* change registers accordingly */
   data[1] = 0x00070000 | (divider << 8);   // R0
   data[5] = 0x0830000E | (r_counter << 8); // R14
   data[6] = 0xC000000F | (n_counter << 8) | (vco_divider << 26); // R15

   /* enable TCA output if requested */
   if (calFreq) {
      if (calFreq == 1)
         data[2] = 0x00050001 | (  1<<8) ; // 148.5 MHz  (33 MHz PLL)
                                           // 150 MHz    (30 MHz PLL)
                                           // 155.52 MHz (19.44 MHz PLL)
      else if (calFreq == 2) {
         data[2] = 0x00070001 | (  4<<8);  // above values divided by 8
         fTCALFrequency /= 8;
      } else if (calFreq == 3) {
         data[2] = 0x00070001 | (255<<8);  // above values divided by 510
         fTCALFrequency /= 510;
      }
   }

   /* set delay to adjsut phase */
   if (calPhase > 0)
      data[2] |= (( calPhase & 0x0F) << 4);
   else if (calPhase < 0)
      data[1] |= ((-calPhase & 0x0F) << 4);

   if (freqChange) {
      /* set all registers */    
      for (int i=0 ; i<(int)(sizeof(data)/sizeof(unsigned int)) ; i++) {
         Write(T_CTRL, REG_LMK_LSB, &data[i], 2);
         Write(T_CTRL, REG_LMK_MSB, ((char *)&data[i])+2, 2);
         // poll on serial_busy flag
         for (int j=0 ; j<100 ; j++) {
            Read(T_STATUS, &status, REG_STATUS, 4);
            if ((status & BIT_SERIAL_BUSY) == 0)
               break;
         }
      }
   } else {
      /* only enable/disable timing calibration frequency */
      Write(T_CTRL, REG_LMK_LSB, &data[1], 2);
      Write(T_CTRL, REG_LMK_MSB, ((char *)&data[1])+2, 2);

      /* poll on serial_busy flag */
      for (int j=0 ; j<100 ; j++) {
         Read(T_STATUS, &status, REG_STATUS, 4);
         if ((status & BIT_SERIAL_BUSY) == 0)
            break;
      }

      Write(T_CTRL, REG_LMK_LSB, &data[2], 2);
      Write(T_CTRL, REG_LMK_MSB, ((char *)&data[2])+2, 2);

      /* poll on serial_busy flag */
      for (int j=0 ; j<100 ; j++) {
         Read(T_STATUS, &status, REG_STATUS, 4);
         if ((status & BIT_SERIAL_BUSY) == 0)
            break;
      }
   }

   return 1;
}

/*------------------------------------------------------------------*/

int DRSBoard::SetFrequency(double demand, bool wait)
{
   // Set domino sampling frequency
   double freq, voltage, delta_voltage;
   unsigned short ticks;
   int i, index, timeout;
   int dominoModeSave = fDominoMode;
   int triggerEnableSave1 = fTriggerEnable1;
   int triggerEnableSave2 = fTriggerEnable2;

   if (fDRSType == 4) {
      /* allowed range is 100 MHz to 6 GHz */
      if (demand > 6 || demand < 0.1)
         return 0;

      if (fBoardType == 6) {
         for (i=1 ; i<100 ; i++) {
            ConfigureLMK(demand, true, fTcalFreq, fTcalPhase);
            Sleep(10);
            if (IsLMKLocked())
               return 1;
            printf("Re-start LMK in VME slot %2d %s\n",  
                    (GetSlotNumber() >> 1)+2, ((GetSlotNumber() & 1) == 0) ? "upper" : "lower");
         }
         return 0;
      }

      /* convert frequency in GHz into ticks counted by reference clock */
      if (demand == 0)
         ticks = 0;             // turn off frequency generation
      else
         ticks = static_cast < unsigned short >(1.024 / demand * fRefClock + 0.5);

      ticks -= 2;               // firmware counter need two additional clock cycles
      Write(T_CTRL, REG_FREQ_SET, &ticks, 2);
      ticks += 2;

      /* convert rounded ticks back to frequency */
      if (demand > 0)
         demand = 1.024 / ticks * fRefClock;
      fNominalFrequency = demand;

      /* wait for PLL lock if asked */
      if (wait) {
         StartDomino();
         for (i=0 ; i<1000 ; i++)
            if (GetStatusReg() & BIT_PLL_LOCKED0)
               break;
         SoftTrigger();
         if (i == 1000) {
            printf("PLL did not lock for frequency %lf\n", demand);
            return 0;
         }
      }
   } else {                     // fDRSType == 4
      SetDominoMode(1);
      EnableTrigger(0, 0);
      EnableAcal(0, 0);

      fNominalFrequency = demand;

      /* turn automatic adjustment off */
      fCtrlBits &= ~BIT_FREQ_AUTO_ADJ;

      /* disable external trigger */
      fCtrlBits &= ~BIT_ENABLE_TRIGGER1;
      fCtrlBits &= ~BIT_ENABLE_TRIGGER2;

      /* set start pulse length for future DRSBoard_domino_start() */
      if (fDRSType == 2) {
         if (demand < 0.8)
            fCtrlBits |= BIT_LONG_START_PULSE;
         else
            fCtrlBits &= ~BIT_LONG_START_PULSE;

         Write(T_CTRL, REG_CTRL, &fCtrlBits, 4);
      }

      /* stop any running domino wave */
      Reinit();

      /* estimate DAC setting */
      voltage = FreqToVolt(demand);

      SetDAC(fDAC_DSA, voltage);
      SetDAC(fDAC_DSB, voltage);

      /* wait until new DAC value has settled */
      Sleep(10);

      /* restart domino wave */
      StartDomino();

      ticks = static_cast < unsigned short >(1024 * 200 * (32.768E6 * 4) / demand / 1E9);

      /* iterate over both DRS chips */
      for (index = 0; index < 2; index++) {

         /* starting voltage */
         voltage = FreqToVolt(demand);

         for (i = 0; i < 100; i++) {

            /* wait until measurement finished */
            for (timeout = 0; timeout < 1000; timeout++)
               if (IsNewFreq(index))
                  break;

            freq = 0;
            if (timeout == 1000)
               break;

            ReadFrequency(index, &freq);

            delta_voltage = FreqToVolt(demand) - FreqToVolt(freq);

            if (fDebug) {
               if (fabs(freq - demand) < 0.001)
                  printf("CHIP-%d, iter%3d: %1.5lf(%05d) %7.5lf\n", index, i, voltage,
                         static_cast < int >(voltage / 2.5 * 65535 + 0.5), freq);
               else
                  printf("CHIP-%d, iter%3d: %1.5lf(%05d) %7.5lf %+5d\n", index, i, voltage,
                         static_cast < int >(voltage / 2.5 * 65535 + 0.5), freq,
                         static_cast < int >(delta_voltage / 2.5 * 65535 + 0.5));
            }

            if (fabs(freq - demand) < 0.001)
               break;

            voltage += delta_voltage;
            if (voltage > 2.5)
               voltage = 2.5;
            if (voltage < 0)
               voltage = 0;

            if (freq == 0)
               break;

            if (index == 0)
               SetDAC(fDAC_DSA, voltage);
            else
               SetDAC(fDAC_DSB, voltage);

            Sleep(10);
         }
         if (i == 100 || freq == 0 || timeout == 1000) {
            printf("Board %d --> Could not set frequency of CHIP-#%d to %1.3f GHz\n", GetBoardSerialNumber(),
                   index, demand);
            return 0;
         }
      }

      SetDominoMode(dominoModeSave);
      EnableTrigger(triggerEnableSave1, triggerEnableSave2);
   }

   return 1;
}

/*------------------------------------------------------------------*/

int DRSBoard::RegulateFrequency(double demand)
{
   // Set frequency regulation
   unsigned short target, target_hi, target_lo;

   if (demand < 0.42 || demand > 5.2)
      return 0;

   fNominalFrequency = demand;

   /* first iterate DAC value from host */
   if (!SetFrequency(demand, true))
      return 0;

   /* convert frequency in GHz into counts for 200 cycles */
   target = static_cast < unsigned short >(1024 * 200 * (32.768E6 * 4) / demand / 1E9);
   target_hi = target + 6;
   target_lo = target - 6;
   Write(T_CTRL, REG_FREQ_SET_HI, &target_hi, 2);
   Write(T_CTRL, REG_FREQ_SET_LO, &target_lo, 2);

   /* turn on regulation */
   fCtrlBits |= BIT_FREQ_AUTO_ADJ;
   Write(T_CTRL, REG_CTRL, &fCtrlBits, 4);

   /* optional monitoring code ... */
#if 0
   do {
      double freq;
      unsigned short dac, cnt;

      ReadFrequency(0, &freq);

      if (fBoardType == 1)
         Read(T_STATUS, &dac, REG_RDAC3, 2);
      else if (fBoardType == 2 || fBoardType == 3)
         Read(T_STATUS, &dac, REG_RDAC1, 2);

      Read(T_STATUS, &cnt, REG_FREQ1, 2);

      if (cnt < 65535)
         printf("%5d %5d %5d %1.5lf\n", dac, target, cnt, freq);

      Sleep(500);
   } while (1);
#endif

   return 1;
}

/*------------------------------------------------------------------*/

void DRSBoard::RegisterTest()
{
   // Register test
#define N_REG 8

   int i, n, n_err;
   unsigned int buffer[N_REG], ret[N_REG];

   /* test single register */
   buffer[0] = 0x12345678;
   Write(T_CTRL, 0, buffer, 4);
   memset(ret, 0, sizeof(ret));
   i = Read(T_CTRL, ret, 0, 4);
   while (i != 4)
      printf("Read error single register!\n");

   printf("Reg.0: %08X - %08X\n", buffer[0], ret[0]);

   n_err = 0;
   for (n = 0; n < 100; n++) {
      for (i = 0; i < N_REG; i++)
         buffer[i] = (rand() << 16) | rand();
      Write(T_CTRL, 0, buffer, sizeof(buffer));

      memset(ret, 0, sizeof(ret));
      i = Read(T_CTRL, ret, 0, sizeof(ret));
      while (i != sizeof(ret)) {
         printf("Read error!\n");
         return;
      }

      for (i = 0; i < N_REG; i++) {
         if (n == 0)
            printf("Reg.%d: %08X - %08X\n", i, buffer[i], ret[i]);
         if (buffer[i] != ret[i]) {
            n_err++;
         }
      }
   }

   printf("Register test: %d errors\n", n_err);
}

/*------------------------------------------------------------------*/

int DRSBoard::RAMTest(int flag)
{
#define MAX_N_BYTES  128*1024   // 128 kB

   int i, j, n, bits, n_bytes, n_words, n_dwords;
   unsigned int buffer[MAX_N_BYTES/4], ret[MAX_N_BYTES/4];
   time_t now;

   if (fBoardType == 6 && fTransport == TR_VME) {
      bits = 32;
      n_bytes = 128*1024; // test full 128 kB
      n_words = n_bytes/2;
      n_dwords = n_words/2;
   }  else {
      bits = 24;
      n_words = 9*1024;
      n_bytes = n_words * 2;
      n_dwords = n_words/2;
   }

   if (flag & 1) {
      /* integrety test */
      printf("Buffer size: %d (%1.1lfk)\n", n_words * 2, n_words * 2 / 1024.0);
      if (flag & 1) {
         for (i = 0; i < n_dwords; i++) {
            if (bits == 24)
               buffer[i] = (rand() | rand() << 16) & 0x00FFFFFF;   // random 24-bit values
            else
               buffer[i] = (rand() | rand() << 16);                // random 32-bit values
         }

         Reinit();
         Write(T_RAM, 0, buffer, n_bytes);
         memset(ret, 0, n_bytes);
         Read(T_RAM, ret, 0, n_bytes);
         Reinit();

         for (i = n = 0; i < n_dwords; i++) {
            if (buffer[i] != ret[i]) {
               n++;
            }
            if (i < 10)
               printf("written: %08X   read: %08X\n", buffer[i], ret[i]);
         }

         printf("RAM test: %d errors\n", n);
      }
   }

   /* speed test */
   if (flag & 2) {
      /* read continously to determine speed */
      time(&now);
      while (now == time(NULL));
      time(&now);
      i = n = 0;
      do {
         memset(ret, 0, n_bytes);

         for (j = 0; j < 10; j++) {
            Read(T_RAM, ret, 0, n_bytes);
            i += n_bytes;
         }

         if (flag & 1) {
            for (j = 0; j < n_dwords; j++)
               if (buffer[j] != ret[j])
                  n++;
         }

         if (now != time(NULL)) {
            if (flag & 1)
               printf("%d read/sec, %1.2lf MB/sec, %d errors\n", static_cast < int >(i / n_bytes),
                      i / 1024.0 / 1024.0, n);
            else
               printf("%d read/sec, %1.2lf MB/sec\n", static_cast < int >(i / n_bytes),
                      i / 1024.0 / 1024.0);
            time(&now);
            i = 0;
         }

         if (drs_kbhit())
            break;

      } while (1);

      while (drs_kbhit())
         getch();
   }

   return 0;
}

/*------------------------------------------------------------------*/

int DRSBoard::ChipTest()
{
   int i, j, t;
   double freq, old_freq, min, max, mean, std;
   float  waveform[1024];

   Init();
   SetChannelConfig(0, 8, 8);
   SetDominoMode(1);
   SetReadoutMode(1);
   SetDominoActive(1);
   SetTranspMode(0);
   EnableTrigger(0, 0);
   EnableTcal(1, 0);
   SelectClockSource(0);
   EnableAcal(1, 0);

   /* test 1 GHz */
   SetFrequency(1, true);
   StartDomino();
   Sleep(100);
   if (!(GetStatusReg() & BIT_PLL_LOCKED0)) {
      puts("PLL did not lock at 1 GHz");
      return 0;
   }

   /* test up to 6 GHz */
   for (freq = 5 ; freq < 6 ; freq += 0.1) {
      SetFrequency(freq, false);
      Sleep(10);
      if (!(GetStatusReg() & BIT_PLL_LOCKED0)) {
         printf("Max. frequency is %1.1lf GHz\n", old_freq);
         break;
      }
      ReadFrequency(0, &old_freq);
   }

   /* read and check at 0 calibration voltage */
   SetFrequency(5, true);
   Sleep(10);
   SoftTrigger();
   while (IsBusy());
   TransferWaves(0, 8);

   for (i=0 ; i<8 ; i++) {
      t = GetStopCell(0);
      GetWave(0, i, waveform, false, t, 0, false);
      for (j=0 ; j<1024; j++)
         if (waveform[j] < -100 || waveform[j] > 100) {
            if (j<5) {
               /* skip this cells */
            } else {
               printf("Cell error on channel %d, cell %d: %1.1lf mV instead 0 mV\n", i, j, waveform[j]);
               return 0;
            }
         }
   }

   /* read and check at +0.5V calibration voltage */
   EnableAcal(1, 0.5);
   StartDomino();
   SoftTrigger();
   while (IsBusy());
   TransferWaves(0, 8);

   for (i=0 ; i<8 ; i++) {
      t = GetStopCell(0);
      GetWave(0, i, waveform, false, t, 0, false);
      for (j=0 ; j<1024; j++)
         if (waveform[j] < 350) {
            if (j<5) {
               /* skip this cell */
            } else {
               printf("Cell error on channel %d, cell %d: %1.1lf mV instead 400 mV\n", i, j, waveform[j]);
               return 0;
            }
         }
   }

   /* read and check at -0.5V calibration voltage */
   EnableAcal(1, -0.5);
   StartDomino();
   Sleep(10);
   SoftTrigger();
   while (IsBusy());
   TransferWaves(0, 8);

   for (i=0 ; i<8 ; i++) {
      t = GetStopCell(0);
      GetWave(0, i, waveform, false, t, 0, false);
      for (j=0 ; j<1024; j++)
         if (waveform[j] > -350) {
            if (j<5) {
               /* skip this cell */
            } else {
               printf("Cell error on channel %d, cell %d: %1.1lf mV instead -400mV\n", i, j, waveform[j]);
               return 0;
            }
         }
   }

   /* check clock channel */
   GetWave(0, 8, waveform, false, 0, 0);
   min = max = mean = std = 0;
   for (j=0 ; j<1024 ; j++) {
      if (waveform[j] > max)
         max = waveform[j];
      if (waveform[j] < min)
         min = waveform[j];
      mean += waveform[j];
   }
   mean /= 1024.0;
   for (j=0 ; j<1024 ; j++)
      std += (waveform[j] - mean) * (waveform[j] - mean);
   std = sqrt(std/1024);

   if (max - min < 400) {
      printf("Error on clock channel amplitude: %1.1lf mV\n", max-min);
      return 0;
   }

   if (std < 100 || std > 300) {
      printf("Error on clock channel Std: %1.1lf mV\n", std);
      return 0;
   }

   return 1;
}

/*------------------------------------------------------------------*/

void DRSBoard::SetVoltageOffset(double offset1, double offset2)
{
   if (fDRSType == 3) {
      SetDAC(fDAC_ROFS_1, 0.95 - offset1);
      SetDAC(fDAC_ROFS_2, 0.95 - offset2);
   } else if (fDRSType == 2)
      SetDAC(fDAC_COFS, 0.9 - offset1);

   // let DAC settle
   Sleep(100);
}

/*------------------------------------------------------------------*/

int DRSBoard::SetInputRange(double center)
{
   if (fBoardType == 5 || fBoardType == 6 || fBoardType == 7 || fBoardType == 8 || fBoardType == 9) {
      // DRS4 USB Evaluation Boards + Mezzanine Board

      // only allow -0.5...0.5 to 0...1.0
      if (center < 0 || center > 0.5)
         return 0;

      // remember range
      fRange = center;

      // correct for sampling cell charge injection
      center *= 1.125;

      // set readout offset
      fROFS = 1.6 - center;
      SetDAC(fDAC_ROFS_1, fROFS);
   }

   return 1;
}

/*------------------------------------------------------------------*/

int DRSBoard::SetExternalClockFrequency(double frequencyMHz)
{
   // Set the frequency of the external clock
   fExternalClockFrequency = frequencyMHz;
   return 0;
}

/*------------------------------------------------------------------*/

double DRSBoard::GetExternalClockFrequency()
{
   // Return the frequency of the external clock
   return fExternalClockFrequency;
}

/*------------------------------------------------------------------*/

int DRSBoard::SetMultiBuffer(int flag)
{
   if (fHasMultiBuffer) {
      // Enable/disable multi-buffering
      fMultiBuffer = flag;
      if (flag)
         fCtrlBits |= BIT_MULTI_BUFFER;
      else
         fCtrlBits &= ~BIT_MULTI_BUFFER;

      if (flag) {
         if (fBoardType == 6)
            fNMultiBuffer = 3; // 3 buffers for VME board
      } else
         fNMultiBuffer = 0;

      Write(T_CTRL, REG_CTRL, &fCtrlBits, 4);
   }
   return 1;
}

/*------------------------------------------------------------------*/

void DRSBoard::ResetMultiBuffer(void)
{
   Reinit(); // set WP=0
   fReadPointer = 0;
}

/*------------------------------------------------------------------*/

int DRSBoard::GetMultiBufferRP(void)
{
   return fReadPointer;
}

/*------------------------------------------------------------------*/

int DRSBoard::SetMultiBufferRP(unsigned short rp)
{
   if (fHasMultiBuffer) {
      fReadPointer = rp;
      Write(T_CTRL, REG_READ_POINTER, &rp, 2);
   }
   return 1;
}

/*------------------------------------------------------------------*/

int DRSBoard::GetMultiBufferWP(void)
{
   unsigned short wp = 0;

   if (fHasMultiBuffer)
      Read(T_STATUS, &wp, REG_WRITE_POINTER, 2);
   else
      wp = 0;
   return wp;
}

/*------------------------------------------------------------------*/

void DRSBoard::IncrementMultiBufferRP()
{
   if (fHasMultiBuffer && fMultiBuffer)
      SetMultiBufferRP((fReadPointer + 1) % fNMultiBuffer);
}

/*------------------------------------------------------------------*/

int DRSBoard::TransferWaves(int numberOfChannels)
{
   return TransferWaves(fWaveforms, numberOfChannels);
}

/*------------------------------------------------------------------*/

int DRSBoard::TransferWaves(unsigned char *p, int numberOfChannels)
{
   return TransferWaves(p, 0, numberOfChannels - 1);
}

/*------------------------------------------------------------------*/

int DRSBoard::TransferWaves(int firstChannel, int lastChannel)
{
   int offset;

   if (fTransport == TR_USB)
      offset = firstChannel * sizeof(short int) * kNumberOfBins;
   else
      offset = 0;               //in VME and USB2, always start from zero

   return TransferWaves(fWaveforms + offset, firstChannel, lastChannel);
}

/*------------------------------------------------------------------*/

int DRSBoard::TransferWaves(unsigned char *p, int firstChannel, int lastChannel)
{
   // Transfer all waveforms at once from VME or USB to location
   int n, i, offset, n_requested, n_bins;
   unsigned int   dw;
   unsigned short w;
   unsigned char *ptr;

   if (lastChannel >= fNumberOfChips * fNumberOfChannels)
      lastChannel = fNumberOfChips * fNumberOfChannels - 1;
   if (lastChannel < 0) {
      printf("Error: Invalid channel index %d\n", lastChannel);
      return 0;
   }

   if (firstChannel < 0 || firstChannel > fNumberOfChips * fNumberOfChannels) {
      printf("Error: Invalid channel index %d\n", firstChannel);
      return 0;
   }

   if (fTransport == TR_VME) {
      /* in VME, always transfer all waveforms, since channels sit 'next' to each other */
      firstChannel = 0;
      lastChannel = fNumberOfChips * fNumberOfChannels - 1;
      if (fReadoutChannelConfig == 4)
         lastChannel = fNumberOfChips * 5 - 1; // special mode to read only even channels + clock
   }

   else if (fTransport == TR_USB2) {
      /* USB2 FPGA contains 9 (Eval) or 10 (Mezz) channels */
      firstChannel = 0;
      if (fBoardType == 5 || fBoardType == 7 || fBoardType == 8 || fBoardType == 9)
         lastChannel = 8;
      else if (fBoardType == 6)
         lastChannel = 9;
   }

   else if (fTransport == TR_USB) {
      /* USB1 FPGA contains only 16 channels */
      if (lastChannel > 15)
         lastChannel = 15;
   }

   n_bins = fDecimation ? kNumberOfBins/2 : kNumberOfBins;
   n_requested = (lastChannel - firstChannel + 1) * sizeof(short int) * n_bins;
   offset = firstChannel * sizeof(short int) * n_bins;

   if (fBoardType == 6 && fFirmwareVersion >= 17147)
      n_requested += 16; // add trailer four chips
      
   if ((fBoardType == 7  || fBoardType == 8 || fBoardType == 9) && fFirmwareVersion >= 17147)
      n_requested += 4;  // add trailer one chip   

   if (fMultiBuffer)
      offset += n_requested * fReadPointer;

   n = Read(T_RAM, p, offset, n_requested);

   if (fMultiBuffer)
      IncrementMultiBufferRP();

   if (n != n_requested) {
      printf("Error: only %d bytes read instead of %d\n", n, n_requested);
      return n;
   }

   // read trigger cells
   if (fDRSType == 4) {
      if (fBoardType == 5 || fBoardType == 7 || fBoardType == 8 || fBoardType == 9) {
         if ((fBoardType == 7  || fBoardType == 8 || fBoardType == 9) && fFirmwareVersion >= 17147) {
            // new code reading trailer
            ptr = p + n_requested - 4;
            fStopCell[0] = *((unsigned short *)(ptr));
            fStopWSR[0]  = *(ptr + 2);
         } else {
            // old code reading status register
            Read(T_STATUS, fStopCell, REG_STOP_CELL0, 2);
            Read(T_STATUS, &w, REG_STOP_WSR0, 2);
            fStopWSR[0] = (w >> 8) & 0xFFFF;
         }
      } else {

         if (fBoardType == 6) {
            // new code reading trailer
            ptr = p + n_requested - 16;
            for (i=0 ; i<4 ; i++) {
               fStopCell[i] = *((unsigned short *)(ptr + i*2));
               fStopWSR[i]  = *(ptr + 8 + i);
            }
            fTriggerBus = *((unsigned short *)(ptr + 12));
         } else {
            // old code reading registers
            Read(T_STATUS, &dw, REG_STOP_CELL0, 4);
            fStopCell[0] = (dw >> 16) & 0xFFFF;
            fStopCell[1] = (dw >>  0) & 0xFFFF;
            Read(T_STATUS, &dw, REG_STOP_CELL2, 4);
            fStopCell[2] = (dw >> 16) & 0xFFFF;
            fStopCell[3] = (dw >>  0) & 0xFFFF;

            Read(T_STATUS, &dw, REG_STOP_WSR0, 4);
            fStopWSR[0] = (dw >> 24) & 0xFF;
            fStopWSR[1] = (dw >> 16) & 0xFF;
            fStopWSR[2] = (dw >>  8) & 0xFF;
            fStopWSR[3] = (dw >>  0) & 0xFF;

            Read(T_STATUS, &fTriggerBus, REG_TRIGGER_BUS, 2);
         }
      }
   }
  
   return n;
}

/*------------------------------------------------------------------*/

int DRSBoard::DecodeWave(unsigned int chipIndex, unsigned char channel, unsigned short *waveform)
{
   return DecodeWave(fWaveforms, chipIndex, channel, waveform);
}

/*------------------------------------------------------------------*/

int DRSBoard::DecodeWave(unsigned char *waveforms, unsigned int chipIndex, unsigned char channel,
                         unsigned short *waveform)
{
   // Get waveform
   int i, offset=0, ind, n_bins;

   /* check valid parameters */
   assert((int)channel < fNumberOfChannels);
   assert((int)chipIndex < fNumberOfChips);

   /* remap channel */
   if (fBoardType == 1) {
      if (channel < 8)
         channel = 7 - channel;
      else
         channel = 16 - channel;
   } else if (fBoardType == 6) {
      if (fReadoutChannelConfig == 7) {
         if (channel < 8)
            channel = 7-channel;
      } else if (fReadoutChannelConfig == 4) {
         if (channel == 8)
            channel = 4;
         else
            channel = 3 - channel/2;
      } else {
         channel = channel / 2;
         if (channel != 4)
           channel = 3-channel;
      }
   } /* else
      channel = channel; */

   // Read channel
   if (fTransport == TR_USB) {
      offset = kNumberOfBins * 2 * (chipIndex * 16 + channel);
      for (i = 0; i < kNumberOfBins; i++) {
         // 12-bit data
         waveform[i] = ((waveforms[i * 2 + 1 + offset] & 0x0f) << 8) + waveforms[i * 2 + offset];
      }
   } else if (fTransport == TR_USB2) {

      if (fBoardType == 5 || fBoardType == 7 || fBoardType == 8 || fBoardType == 9)
         // see dpram_map_eval1.xls
         offset = kNumberOfBins * 2 * (chipIndex * 16 + channel);
      else if (fBoardType == 6) {
         // see dpram_map_mezz1.xls mode 0-3
         offset = (kNumberOfBins * 4) * (channel % 9) + 2 * (chipIndex/2);
      }
      for (i = 0; i < kNumberOfBins; i++) {
         // 16-bit data
         if (fBoardType == 5 || fBoardType == 7 || fBoardType == 8 || fBoardType == 9)
            waveform[i] = ((waveforms[i * 2 + 1 + offset] & 0xff) << 8) + waveforms[i * 2 + offset];
         else if (fBoardType == 6)
            waveform[i] = ((waveforms[i * 4 + 1 + offset] & 0xff) << 8) + waveforms[i * 4 + offset];
      }

   } else if (fTransport == TR_VME) {

      if (fBoardType == 6) {
         n_bins = fDecimation ? kNumberOfBins/2 : kNumberOfBins;
         if (fReadoutChannelConfig == 7)       // see dpram_map_mezz1.xls mode 7
            offset = (n_bins * 4) * (channel % 9 + 9*(chipIndex % 2)) + 2 * (chipIndex/2);
         else if (fReadoutChannelConfig == 4)  // see dpram_map_mezz1.xls mode 4
            offset = (n_bins * 4) * (channel % 5 + 5*(chipIndex % 2)) + 2 * (chipIndex/2);
         for (i = 0; i < n_bins; i++)
            waveform[i] = ((waveforms[i * 4 + 1 + offset] & 0xff) << 8) + waveforms[i * 4 + offset];
      } else {
         offset = (kNumberOfBins * 4) * channel;
         for (i = 0; i < kNumberOfBins; i++) {
            ind = i * 4 + offset;
            if (chipIndex == 0)
               // lower 12 bit
               waveform[i] = ((waveforms[ind + 1] & 0x0f) << 8) | waveforms[ind];
            else
               // upper 12 bit
               waveform[i] = (waveforms[ind + 2] << 4) | (waveforms[ind + 1] >> 4);
         }
      }
   } else {
      printf("Error: invalid transport %d\n", fTransport);
      return kInvalidTransport;
   }
   return kSuccess;
}

/*------------------------------------------------------------------*/

int DRSBoard::GetWave(unsigned int chipIndex, unsigned char channel, float *waveform)
{
   return GetWave(chipIndex, channel, waveform, true, fStopCell[chipIndex], -1, false, 0, true);
}

/*------------------------------------------------------------------*/

int DRSBoard::GetWave(unsigned int chipIndex, unsigned char channel, short *waveform, bool responseCalib,
                      int triggerCell, int wsr, bool adjustToClock, float threshold, bool offsetCalib)
{
   return GetWave(fWaveforms, chipIndex, channel, waveform, responseCalib, triggerCell, wsr, adjustToClock,
                  threshold, offsetCalib);
}

/*------------------------------------------------------------------*/

int DRSBoard::GetWave(unsigned int chipIndex, unsigned char channel, float *waveform, bool responseCalib,
                      int triggerCell, int wsr, bool adjustToClock, float threshold, bool offsetCalib)
{
   return GetWave(fWaveforms, chipIndex, channel, waveform, responseCalib, triggerCell, wsr, adjustToClock, threshold,
               offsetCalib);
}

/*------------------------------------------------------------------*/

int DRSBoard::GetWave(unsigned char *waveforms, unsigned int chipIndex, unsigned char channel,
                      float *waveform, bool responseCalib, int triggerCell, int wsr, bool adjustToClock,
                      float threshold, bool offsetCalib)
{
   int ret, i;
   short waveS[2*kNumberOfBins];
   ret =
       GetWave(waveforms, chipIndex, channel, waveS, responseCalib, triggerCell, wsr, adjustToClock, threshold,
               offsetCalib);
   if (responseCalib)
      for (i = 0; i < fChannelDepth ; i++)
         waveform[i] = static_cast < float >(static_cast <short> (waveS[i]) * GetPrecision());
   else {
      for (i = 0; i < fChannelDepth ; i++) {
         if (fBoardType == 4 || fBoardType == 5 || fBoardType == 6 || fBoardType == 7 || fBoardType == 8 || fBoardType == 9) {
            waveform[i] = static_cast < float >(waveS[i] * GetPrecision());
         } else
            waveform[i] = static_cast < float >(waveS[i]);
      }
   }
   return ret;
}

/*------------------------------------------------------------------*/

int DRSBoard::GetWave(unsigned char *waveforms, unsigned int chipIndex, unsigned char channel,
                      short *waveform, bool responseCalib, int triggerCell, int wsr, bool adjustToClock,
                      float threshold, bool offsetCalib)
{
   unsigned short adcWaveform[kNumberOfBins];
   int i, ret;

   if (fChannelCascading == 1 || channel == 8) {
      /* single channel configuration */
      ret = DecodeWave(waveforms, chipIndex, channel, adcWaveform);
      if (ret != kSuccess)
         return ret;

      ret = CalibrateWaveform(chipIndex, channel, adcWaveform, waveform, responseCalib,
                              triggerCell, adjustToClock, threshold, offsetCalib);

      return ret;
   } else if (fChannelCascading == 2) {
      /* double channel configuration */
      short wf1[kNumberOfBins];
      short wf2[kNumberOfBins];

      // first half
      ret = DecodeWave(waveforms, chipIndex, 2*channel, adcWaveform);
      if (ret != kSuccess)
         return ret;

      ret = CalibrateWaveform(chipIndex, 2*channel, adcWaveform, wf1, responseCalib,
                              triggerCell, adjustToClock, threshold, offsetCalib);

      // second half
      ret = DecodeWave(waveforms, chipIndex, 2*channel+1, adcWaveform);
      if (ret != kSuccess)
         return ret;

      ret = CalibrateWaveform(chipIndex, 2*channel+1, adcWaveform, wf2, responseCalib,
                              triggerCell, adjustToClock, threshold, offsetCalib);


      // combine two halfs correctly, see 2048_mode.ppt
      if (fBoardType == 5 || fBoardType == 7 || fBoardType == 8 || fBoardType == 9) {
         if ((wsr == 0 && triggerCell < 767) ||
             (wsr == 1 && triggerCell >= 767)) {
            for (i=0 ; i<kNumberOfBins-triggerCell ; i++)
               waveform[i] = wf1[i];
            for (; i<kNumberOfBins; i++)
               waveform[i] = wf2[i];
            for (i=0 ; i<kNumberOfBins-triggerCell ; i++)
               waveform[i+kNumberOfBins] = wf2[i];
            for (; i<kNumberOfBins; i++)
               waveform[i+kNumberOfBins] = wf1[i];
         } else {
            for (i=0 ; i<kNumberOfBins-triggerCell ; i++)
               waveform[i] = wf2[i];
            for (; i<kNumberOfBins; i++)
               waveform[i] = wf1[i];
            for (i=0 ; i<kNumberOfBins-triggerCell ; i++)
               waveform[i+kNumberOfBins] = wf1[i];
            for (; i<kNumberOfBins; i++)
               waveform[i+kNumberOfBins] = wf2[i];
         }
      } else {
         if (wsr == 1) {
            if (fDecimation) {
               for (i=0 ; i<kNumberOfBins/2-triggerCell/2 ; i++)
                  waveform[i] = wf1[i];
               for (; i<kNumberOfBins/2; i++)
                  waveform[i] = wf2[i];
               for (i=0 ; i<kNumberOfBins/2-triggerCell/2 ; i++)
                  waveform[i+kNumberOfBins/2] = wf2[i];
               for (; i<kNumberOfBins/2; i++)
                  waveform[i+kNumberOfBins/2] = wf1[i];
            } else {
               for (i=0 ; i<kNumberOfBins-triggerCell ; i++)
                  waveform[i] = wf1[i];
               for (; i<kNumberOfBins; i++)
                  waveform[i] = wf2[i];
               for (i=0 ; i<kNumberOfBins-triggerCell ; i++)
                  waveform[i+kNumberOfBins] = wf2[i];
               for (; i<kNumberOfBins; i++)
                  waveform[i+kNumberOfBins] = wf1[i];
            }
         } else {
            if (fDecimation) {
               for (i=0 ; i<kNumberOfBins/2-triggerCell/2 ; i++)
                  waveform[i] = wf2[i];
               for (; i<kNumberOfBins/2; i++)
                  waveform[i] = wf1[i];
               for (i=0 ; i<kNumberOfBins/2-triggerCell/2 ; i++)
                  waveform[i+kNumberOfBins/2] = wf1[i];
               for (; i<kNumberOfBins/2; i++)
                  waveform[i+kNumberOfBins/2] = wf2[i];
            } else {
               for (i=0 ; i<kNumberOfBins-triggerCell ; i++)
                  waveform[i] = wf2[i];
               for (; i<kNumberOfBins; i++)
                  waveform[i] = wf1[i];
               for (i=0 ; i<kNumberOfBins-triggerCell ; i++)
                  waveform[i+kNumberOfBins] = wf1[i];
               for (; i<kNumberOfBins; i++)
                  waveform[i+kNumberOfBins] = wf2[i];
            }
         }
      }

      return ret;
   } else
      assert(!"Not implemented");

   return 0;
}

/*------------------------------------------------------------------*/

int DRSBoard::GetRawWave(unsigned int chipIndex, unsigned char channel, unsigned short *waveform,
                         bool adjustToClock)
{
   return GetRawWave(fWaveforms, chipIndex, channel, waveform, adjustToClock);
}

/*------------------------------------------------------------------*/

int DRSBoard::GetRawWave(unsigned char *waveforms, unsigned int chipIndex, unsigned char channel, 
                         unsigned short *waveform, bool adjustToClock)
{
   int i, status, tc;
   unsigned short wf[kNumberOfBins];

   status = DecodeWave(waveforms, chipIndex, channel,  wf);

   if (adjustToClock) {
      tc = GetTriggerCell(chipIndex);
      for (i = 0 ; i < kNumberOfBins; i++)
         waveform[(i + tc) % kNumberOfBins] = wf[i];
   } else {
      for (i = 0 ; i < kNumberOfBins; i++)
         waveform[i] = wf[i];
   }

   return status;
}

/*------------------------------------------------------------------*/

int DRSBoard::CalibrateWaveform(unsigned int chipIndex, unsigned char channel, unsigned short *adcWaveform,
                                short *waveform, bool responseCalib,
                                int triggerCell, bool adjustToClock, float threshold, bool offsetCalib)
{
   int j, n_bins, skip;
   double value;
   short left, right;

   // calibrate waveform
   if (responseCalib && fVoltageCalibrationValid) {
      if (GetDRSType() == 4) {
         // if Mezz though USB2 -> select correct calibration channel
         if (fBoardType == 6 && (fReadoutChannelConfig == 0 || fReadoutChannelConfig == 2) &&
             channel != 8)
            channel++;

         // Channel readout mode #4 -> select correct calibration channel
         if (fBoardType == 6 && fReadoutChannelConfig == 4 && channel % 2 == 0 && channel != 8)
            channel++;

         n_bins = fDecimation ? kNumberOfBins/2 : kNumberOfBins;
         skip = fDecimation ? 2 : 1;
         for (j = 0; j < n_bins; j++) {
            value = adcWaveform[j] - fCellOffset[channel+chipIndex*9][(j*skip + triggerCell) % kNumberOfBins];
            value = value / fCellGain[channel+chipIndex*9][(j*skip + triggerCell) % kNumberOfBins];
            if (offsetCalib && channel != 8)
               value = value - fCellOffset2[channel+chipIndex*9][j*skip] + 32768;

            /* convert to units of 0.1 mV */
            value = value / 65536.0 * 1000 * 10; 

            /* apply clipping */
            if (channel != 8) {
               if (adcWaveform[j] >= 0xFFF0 || value > (fRange * 1000 + 500) * 10)
                  value = (fRange * 1000 + 500) * 10;
               if (adcWaveform[j] <  0x0010 || value < (fRange * 1000 - 500) * 10)
                  value = (fRange * 1000 - 500) * 10;
            }

            if (adjustToClock)          
               waveform[(j + triggerCell) % kNumberOfBins] = (short) (value + 0.5);
            else
               waveform[j] = (short) (value + 0.5); 
         }

         // check for stuck pixels and replace by average of neighbors
         for (j = 0 ; j < n_bins; j++) {
            if (adjustToClock) {
               if (fCellOffset[channel+chipIndex*9][j*skip] == 0) {
                  left = waveform[(j-1+kNumberOfBins) % kNumberOfBins];
                  right = waveform[(j+1) % kNumberOfBins];
                  waveform[j] = (short) ((left+right)/2);
               }
            } else {
               if (fCellOffset[channel+chipIndex*9][(j*skip + triggerCell) % kNumberOfBins] == 0) {
                  left = waveform[(j-1+kNumberOfBins) % kNumberOfBins];
                  right = waveform[(j+1) % kNumberOfBins];
                  waveform[j] = (short) ((left+right)/2);
               }
            }
         }

      } else {
         if (!fResponseCalibration->
             Calibrate(chipIndex, channel % 10, adcWaveform, waveform, triggerCell, threshold, offsetCalib))
            return kZeroSuppression;       // return immediately if below threshold
      }
   } else {
      if (GetDRSType() == 4) {
         // if Mezz though USB2 -> select correct calibration channel
         if (fBoardType == 6 && (fReadoutChannelConfig == 0 || fReadoutChannelConfig == 2) &&
             channel != 8)
            channel++;
         for (j = 0 ; j < kNumberOfBins; j++) {
            value = adcWaveform[j];

            /* convert to units of 0.1 mV */
            value = (value - 32768) / 65536.0 * 1000 * 10; 

            /* correct for range */
            value += fRange * 1000 * 10;

            if (adjustToClock)          
               waveform[(j + triggerCell) % kNumberOfBins] = (short) (value + 0.5);
            else
               waveform[j] = (short) (value + 0.5); 
         }
      } else {
         for (j = 0; j < kNumberOfBins; j++) {
            if (adjustToClock) {
               // rotate waveform such that waveform[0] corresponds to bin #0 on the chip
               waveform[j] = adcWaveform[(kNumberOfBins-triggerCell+j) % kNumberOfBins];
            } else {
               waveform[j] = adcWaveform[j];
            }
         }
      }
   }

   // fix bad cells for single turn mode
   if (GetDRSType() == 2) {
      if (fDominoMode == 0 && triggerCell == -1) {
         waveform[0] = 2 * waveform[1] - waveform[2];
         short m1 = (waveform[kNumberOfBins - 5] + waveform[kNumberOfBins - 6]) / 2;
         short m2 = (waveform[kNumberOfBins - 6] + waveform[kNumberOfBins - 7]) / 2;
         waveform[kNumberOfBins - 4] = m1 - 1 * (m2 - m1);
         waveform[kNumberOfBins - 3] = m1 - 2 * (m2 - m1);
         waveform[kNumberOfBins - 2] = m1 - 3 * (m2 - m1);
         waveform[kNumberOfBins - 1] = m1 - 4 * (m2 - m1);
      }
   }

   return kSuccess;
}

/*------------------------------------------------------------------*/

int DRSBoard::GetStretchedTime(float *time, float *measurement, int numberOfMeasurements, float period)
{
   int j;
   if (*time >= measurement[numberOfMeasurements - 1]) {
      *time -= measurement[numberOfMeasurements - 1];
      return 1;
   }
   if (*time < measurement[0]) {
      *time = *time - measurement[0] - (numberOfMeasurements - 1) * period / 2;
      return 1;
   }
   for (j = 0; j < numberOfMeasurements - 1; j++) {
      if (*time > measurement[j] && *time <= measurement[j + 1]) {
         *time =
             (period / 2) / (measurement[j + 1] - measurement[j]) * (*time - measurement[j + 1]) -
             (numberOfMeasurements - 2 - j) * period / 2;
         return 1;
      }
   }
   return 0;
}

/*------------------------------------------------------------------*/

int DRSBoard::GetTriggerCell(unsigned int chipIndex)
{
   if (fDRSType == 4)
      return GetStopCell(chipIndex);

   return GetTriggerCell(fWaveforms, chipIndex);
}

/*------------------------------------------------------------------*/

int DRSBoard::GetTriggerCell(unsigned char *waveforms, unsigned int chipIndex)
{
   int j, triggerCell;
   bool calib;
   unsigned short baseLevel = 1000;
   unsigned short triggerChannel[1024];

   if (fDRSType == 4)
      return GetStopCell(chipIndex);

   GetRawWave(waveforms, chipIndex, 8, triggerChannel);
   calib = fResponseCalibration->SubtractADCOffset(chipIndex, 8, triggerChannel, triggerChannel, baseLevel);

   triggerCell = -1;
   for (j = 0; j < kNumberOfBins; j++) {
      if (calib) {
         if (triggerChannel[j] <= baseLevel + 200
             && triggerChannel[(j + 1) % kNumberOfBins] > baseLevel + 200) {
            triggerCell = j;
            break;
         }
      } else {
         if (fDRSType == 3) {
            if (triggerChannel[j] <= 2000 && triggerChannel[(j + 1) % kNumberOfBins] > 2000) {
               triggerCell = j;
               break;
            }
         } else {
            if (triggerChannel[j] >= 2000 && triggerChannel[(j + 1) % kNumberOfBins] < 2000) {
               triggerCell = j;
               break;
            }
         }
      }
   }
   if (triggerCell == -1) {
      return kInvalidTriggerSignal;
   }
   fStopCell[0] = triggerCell;
   return triggerCell;
}

/*------------------------------------------------------------------*/

int DRSBoard::GetStopCell(unsigned int chipIndex)
{
   return fStopCell[chipIndex];
}

/*------------------------------------------------------------------*/

unsigned char DRSBoard::GetStopWSR(unsigned int chipIndex)
{
   return fStopWSR[chipIndex];
}

/*------------------------------------------------------------------*/

void DRSBoard::TestDAC(int channel)
{
   // Test DAC
   int status;

   do {
      status = SetDAC(channel, 0);
      Sleep(1000);
      status = SetDAC(channel, 0.5);
      Sleep(1000);
      status = SetDAC(channel, 1);
      Sleep(1000);
      status = SetDAC(channel, 1.5);
      Sleep(1000);
      status = SetDAC(channel, 2);
      Sleep(1000);
      status = SetDAC(channel, 2.5);
      Sleep(1000);
   } while (status);
}

/*------------------------------------------------------------------*/

void DRSBoard::MeasureSpeed()
{
   // Measure domino sampling speed
   FILE *f;
   double vdr, vds, freq;

   f = fopen("speed.txt", "wt");
   fprintf(f, "\t");
   printf("\t");
   for (vdr = 0.5; vdr <= 2.501; vdr += 0.05) {
      fprintf(f, "%1.2lf\t", vdr);
      printf("%1.2lf\t", vdr);
   }
   fprintf(f, "\n");
   printf("\n");

   for (vds = 0.5; vds <= 2.501; vds += 0.05) {
      fprintf(f, "%1.2lf\t", vds);
      printf("%1.2lf\t", vds);

      SetDAC(fDAC_DSA, vds);
      StartDomino();
      Sleep(1000);
      ReadFrequency(0, &freq);

      fprintf(f, "%1.3lf\t", freq);
      printf("%1.3lf\t", freq);

      fprintf(f, "\n");
      printf("\n");
      fflush(f);
   }
}

/*------------------------------------------------------------------*/

void DRSBoard::InteractSpeed()
{
   int status, i;
   double freq, vds;

   do {
      printf("DS: ");
      scanf("%lf", &vds);
      if (vds == 0)
         break;

      SetDAC(fDAC_DSA, vds);
      SetDAC(fDAC_DSB, vds);

      StartDomino();
      for (i = 0; i < 4; i++) {
         Sleep(1000);

         status = ReadFrequency(0, &freq);
         if (!status)
            break;
         printf("%1.6lf GHz\n", freq);
      }

      /* turn BOARD_LED off */
      SetLED(0);

   } while (1);
}

/*------------------------------------------------------------------*/

void DRSBoard::MonitorFrequency()
{
   // Monitor domino sampling frequency
   int status;
   unsigned int data;
   double freq, dac;
   FILE *f;
   time_t now;
   char str[256];

   f = fopen("DRSBoard.log", "w");

   do {
      Sleep(1000);

      status = ReadFrequency(0, &freq);
      if (!status)
         break;

      data = 0;
      if (fBoardType == 1)
         Read(T_STATUS, &data, REG_RDAC3, 2);
      else if (fBoardType == 2 || fBoardType == 3)
         Read(T_STATUS, &data, REG_RDAC1, 2);

      dac = data / 65536.0 * 2.5;
      printf("%1.6lf GHz, %1.4lf V\n", freq, dac);
      time(&now);
      strcpy(str, ctime(&now) + 11);
      str[8] = 0;

      fprintf(f, "%s %1.6lf GHz, %1.4lf V\n", str, freq, dac);
      fflush(f);

   } while (!drs_kbhit());

   fclose(f);
}

/*------------------------------------------------------------------*/

int DRSBoard::TestShift(int n)
{
   // Test shift register
   unsigned char buffer[3];

   memset(buffer, 0, sizeof(buffer));

#if 0
   buffer[0] = CMD_TESTSHIFT;
   buffer[1] = n;

   status = msend_usb(buffer, 2);
   if (status != 2)
      return status;

   status = mrecv_usb(buffer, sizeof(buffer));
   if (status != 1)
      return status;
#endif

   if (buffer[0] == 1)
      printf("Shift register %c works correctly\n", 'A' + n);
   else if (buffer[0] == 2)
      printf("SROUT%c does hot go high after reset\n", 'A' + n);
   else if (buffer[0] == 3)
      printf("SROUT%c does hot go low after 1024 clocks\n", 'A' + n);

   return 1;
}

/*------------------------------------------------------------------*/

unsigned int DRSBoard::GetCtrlReg()
{
   unsigned int status;

   Read(T_CTRL, &status, REG_CTRL, 4);
   return status;
}

/*------------------------------------------------------------------*/

unsigned short DRSBoard::GetConfigReg()
{
   unsigned short status;

   Read(T_CTRL, &status, REG_CONFIG, 2);
   return status;
}

/*------------------------------------------------------------------*/

unsigned int DRSBoard::GetStatusReg()
{
   unsigned int status;

   Read(T_STATUS, &status, REG_STATUS, 4);
   return status;
}

/*------------------------------------------------------------------*/

int DRSBoard::EnableTcal(int freq, int level, int phase)
{
   fTcalFreq = freq;
   fTcalLevel = level;
   fTcalPhase = phase;

   if (fBoardType == 6) {
      ConfigureLMK(fNominalFrequency, false, freq, phase);
   } else {
      if (fBoardType == 9) {
         // Enable clock a switch channel multiplexers
         if (freq) {
            fCtrlBits |= (BIT_TCAL_EN | BIT_ACAL_EN);
         } else
            fCtrlBits &= ~(BIT_TCAL_EN | BIT_ACAL_EN);
         
      } else {
         // Enable clock channel
         if (freq)
            fCtrlBits |= BIT_TCAL_EN;
         else
            fCtrlBits &= ~(BIT_TCAL_EN | BIT_TCAL_SOURCE);
         
         // Set output level, needed for gain calibration
         if (fDRSType == 4) {
            if (level)
               fCtrlBits |= BIT_NEG_TRIGGER;
            else
               fCtrlBits &= ~BIT_NEG_TRIGGER;
         }
      }

      Write(T_CTRL, REG_CTRL, &fCtrlBits, 4);
   }

   return 1;
}

/*------------------------------------------------------------------*/

int DRSBoard::SelectClockSource(int source)
{
   fTcalSource = source;

   // Select clock source:
   // EVAL1: synchronous (0) or asynchronous (1) (2nd quartz)
   if (fBoardType <= 8) {
      if (source)
         fCtrlBits |= BIT_TCAL_SOURCE;
      else
         fCtrlBits &= ~BIT_TCAL_SOURCE;
      
      Write(T_CTRL, REG_CTRL, &fCtrlBits, 4);
   }

   return 1;
}

/*------------------------------------------------------------------*/

int DRSBoard::SetRefclk(int source)
{
   // Select reference clock source to internal FPGA (0) or external P2 (1)
   if (fBoardType == 6) {
      if (source) 
         fCtrlBits |= BIT_REFCLK_SOURCE;
      else
         fCtrlBits &= ~BIT_REFCLK_SOURCE;
   } else if (fBoardType == 8 || fBoardType == 9) {
      if (source) 
         fCtrlBits |= BIT_REFCLK_EXT;
      else
         fCtrlBits &= ~BIT_REFCLK_EXT;
   }

   Write(T_CTRL, REG_CTRL, &fCtrlBits, 4);
   fRefclk = source;

   return 1;
}

/*------------------------------------------------------------------*/

int DRSBoard::EnableAcal(int mode, double voltage)
{
   double t1, t2;

   fAcalMode = mode;
   fAcalVolt = voltage;

   if (mode == 0) {
      /* turn calibration off */
      SetCalibTiming(0, 0);
      if (fBoardType == 5 || fBoardType == 6) {
         /* turn voltages off (50 Ohm analog switch!) */
         SetDAC(fDAC_CALP, 0);
         SetDAC(fDAC_CALN, 0);
      }
      if (fBoardType == 7 || fBoardType == 8 || fBoardType == 9)
         SetCalibVoltage(0);

      fCtrlBits &= ~BIT_ACAL_EN;
      Write(T_CTRL, REG_CTRL, &fCtrlBits, 4);
   } else if (mode == 1) {
      /* static calibration */
      SetCalibVoltage(voltage);
      SetCalibTiming(0, 0);
      fCtrlBits |= BIT_ACAL_EN;
      Write(T_CTRL, REG_CTRL, &fCtrlBits, 4);
   } else if (mode == 2) {
      /* first part calibration:
         stop domino wave after 1.2 revolutions
         turn on calibration voltage after 0.1 revolutions */

      /* ensure circulating domino wave */
      SetDominoMode(1);

      /* set calibration voltage but do not turn it on now */
      SetCalibVoltage(voltage);
      fCtrlBits &= ~BIT_ACAL_EN;
      Write(T_CTRL, REG_CTRL, &fCtrlBits, 4);

      /* calculate duration of DENABLE signal as 1.2 revolutions */
      t1 = 1 / fNominalFrequency * 1024 * 1.2; // ns
      t1 = static_cast < int >((t1 - 30) / 30 + 1);     // 30 ns offset, 30 ns units, rounded up
      t2 = 1 / fNominalFrequency * 1024 * 0.1; // ns
      t2 = static_cast < int >((t2 - 30) / 30 + 1);     // 30 ns offset, 30 ns units, rounded up
      SetCalibTiming(static_cast < int >(t1), static_cast < int >(t2));

   } else if (mode == 3) {
      /* second part calibration:
         stop domino wave after 1.05 revolutions */

      /* ensure circulating domino wave */
      SetDominoMode(1);

      /* turn on and let settle calibration voltage */
      SetCalibVoltage(voltage);
      fCtrlBits |= BIT_ACAL_EN;
      Write(T_CTRL, REG_CTRL, &fCtrlBits, 4);

      /* calculate duration of DENABLE signal as 1.1 revolutions */
      t1 = 1 / fNominalFrequency * 1024 * 1.05;        // ns
      t1 = static_cast < int >((t1 - 30) / 30 + 1);     // 30 ns offset, 30 ns units, rounded up
      SetCalibTiming(static_cast < int >(t1), 0);
   }

   return 1;
}

/*------------------------------------------------------------------*/

int DRSBoard::SetCalibTiming(int t_enable, int t_cal)
{
   unsigned short d;

   if (fDRSType == 2) {
      d = t_cal | (t_enable << 8);
      Write(T_CTRL, REG_CALIB_TIMING, &d, 2);
   }

   if (fDRSType == 3) {
      d = t_cal;
      Write(T_CTRL, REG_CALIB_TIMING, &d, 2);
   }

   return 1;
}

/*------------------------------------------------------------------*/

int DRSBoard::SetCalibVoltage(double value)
{
   // Set Calibration Voltage
   if (fBoardType == 5 || fBoardType == 6 || fBoardType == 7 || fBoardType == 8 || fBoardType == 9) {
      if (fBoardType == 5)
         value = value * (1+fNominalFrequency/65); // rough correction factor for input current
      if (fBoardType == 7 || fBoardType == 8 || fBoardType == 9)
         value = value * (1+fNominalFrequency/47); // rough correction factor for input current
      SetDAC(fDAC_CALP, fCommonMode + value / 2);
      SetDAC(fDAC_CALN, fCommonMode - value / 2);
   } else
      SetDAC(fDAC_ACALIB, value);
   return 1;
}

/*------------------------------------------------------------------*/

int DRSBoard::StartClearCycle()
{
   /* clear cycle is necessary for DRS4 to reduce noise */

   fbkAcalVolt  = fAcalVolt;
   fbkAcalMode  = fAcalMode;
   fbkTcalFreq  = fTcalFreq;
   fbkTcalLevel = fTcalLevel;

   /* switch all inputs to zero */
   EnableAcal(1, 0);

   /* start, stop and readout of zero */
   StartDomino();
   SoftTrigger();

   return 1;
}

/*------------------------------------------------------------------*/

int DRSBoard::FinishClearCycle()
{
   while (IsBusy());

   /* restore old values */
   EnableAcal(fbkAcalMode, fbkAcalVolt);

   return 1;
}

/*------------------------------------------------------------------*/

double DRSBoard::GetTemperature()
{
   // Read Out Temperature Sensor
   unsigned char buffer[2];
   unsigned short d;
   double temperature;

   Read(T_STATUS, buffer, REG_TEMPERATURE, 2);

   d = (static_cast < unsigned int >(buffer[1]) << 8) +buffer[0];
   temperature = ((d >> 3) & 0x0FFF) * 0.0625;

   return temperature;
}

/*------------------------------------------------------------------*/

int DRSBoard::Is2048ModeCapable()
{
   unsigned int status;
   
   if (fFirmwareVersion < 21305)
      return 0;
   
   // Read pin J44 and return 1 if 2048 mode has been soldered
   Read(T_STATUS, &status, REG_STATUS, 4);
   if ((status & BIT_2048_MODE))
      return 0;
   return 1;
}

/*------------------------------------------------------------------*/

int DRSBoard::GetTriggerBus()
{
   unsigned size, d;

   if (fBoardType == 6 && fTransport == TR_VME) {
      if (fReadoutChannelConfig == 4)
         size = (20 * sizeof(short int) * (fDecimation ? kNumberOfBins/2 : kNumberOfBins) + 16);
      else
         size = (36 * sizeof(short int) * (fDecimation ? kNumberOfBins/2 : kNumberOfBins) + 16);

      Read(T_RAM, &d, size * fReadPointer + size - 16 + 12, 4);
      fTriggerBus = (unsigned short)d;
   } else {
      Read(T_STATUS, &fTriggerBus, REG_TRIGGER_BUS, 2);
   }
   return static_cast < int >(fTriggerBus);
}


/*------------------------------------------------------------------*/

unsigned int DRSBoard::GetScaler(int channel)
{
   int reg = 0;
   unsigned d;
   
   if (fBoardType < 9 || fFirmwareVersion < 21000 || fTransport != TR_USB2)
      return 0;
   
   switch (channel ) {
      case 0: reg = REG_SCALER0; break;
      case 1: reg = REG_SCALER1; break;
      case 2: reg = REG_SCALER2; break;
      case 3: reg = REG_SCALER3; break;
      case 4: reg = REG_SCALER4; break;
      case 5: reg = REG_SCALER5; break;
   }
   
   Read(T_STATUS, &d, reg, 4);

   return static_cast < unsigned int >(d * 10); // measurement clock is 10 Hz
}


/*------------------------------------------------------------------*/

int DRSBoard::SetBoardSerialNumber(unsigned short serialNumber)
{
   unsigned char buf[32768];

   unsigned short dac;

   if (fDRSType < 4) {
      // read current DAC register
      Read(T_CTRL, &dac, REG_DAC0, 2);

      // put serial in DAC register
      Write(T_CTRL, REG_DAC0, &serialNumber, 2);

      // execute flash
      fCtrlBits |= BIT_EEPROM_WRITE_TRIG;
      Write(T_CTRL, REG_CTRL, &fCtrlBits, 4);
      fCtrlBits &= ~BIT_EEPROM_WRITE_TRIG;

      // wait 6ms per word
      Sleep(20);

      // write back old DAC registers
      Write(T_CTRL, REG_DAC0, &dac, 2);

      // read back serial number
      ReadSerialNumber();

   } else if (fDRSType == 4) {
      /* merge serial number into eeprom page #0 */
      ReadEEPROM(0, buf, sizeof(buf));
      buf[0] = serialNumber & 0xFF;
      buf[1] = serialNumber >> 8;
      WriteEEPROM(0, buf, sizeof(buf));

      /* erase DPRAM */
      memset(buf, 0, sizeof(buf));
      Write(T_RAM, 0, buf, sizeof(buf));

      /* read back EEPROM */
      ReadEEPROM(0, buf, sizeof(buf));

      /* check if correctly set */
      if (((buf[1] << 8) | buf[0]) != serialNumber)
         return 0;

      fBoardSerialNumber = serialNumber;
   }

   return 1;
}

/*------------------------------------------------------------------*/

int DRSBoard::ReadEEPROM(unsigned short page, void *buffer, int size)
{
   int i;
   unsigned long status;
   // write eeprom page number
   if (fBoardType == 5 || fBoardType == 7 || fBoardType == 8 || fBoardType == 9)
      Write(T_CTRL, REG_EEPROM_PAGE_EVAL, &page, 2);
   else if (fBoardType == 6)
      Write(T_CTRL, REG_EEPROM_PAGE_MEZZ, &page, 2);
   else 
      return -1;

   // execute eeprom read
   fCtrlBits |= BIT_EEPROM_READ_TRIG;
   Write(T_CTRL, REG_CTRL, &fCtrlBits, 4);
   fCtrlBits &= ~BIT_EEPROM_READ_TRIG;

   // poll on serial_busy flag
   for (i=0 ; i<100 ; i++) {
      Read(T_STATUS, &status, REG_STATUS, 4);
      if ((status & BIT_SERIAL_BUSY) == 0)
         break;
      Sleep(10);
   }

   return Read(T_RAM, buffer, 0, size);
}

/*------------------------------------------------------------------*/

int DRSBoard::WriteEEPROM(unsigned short page, void *buffer, int size)
{
   int i;
   unsigned long status;
   unsigned char buf[32768];

   // read previous page
   ReadEEPROM(page, buf, sizeof(buf));
   
   // combine with new page
   memcpy(buf, buffer, size);
   
   // write eeprom page number
   if (fBoardType == 5 || fBoardType == 7 || fBoardType == 8 || fBoardType == 9)
      Write(T_CTRL, REG_EEPROM_PAGE_EVAL, &page, 2);
   else if (fBoardType == 6)
      Write(T_CTRL, REG_EEPROM_PAGE_MEZZ, &page, 2);
   else 
      return -1;

   // write eeprom page to RAM
   Write(T_RAM, 0, buf, size);

   // execute eeprom write
   fCtrlBits |= BIT_EEPROM_WRITE_TRIG;
   Write(T_CTRL, REG_CTRL, &fCtrlBits, 4);
   fCtrlBits &= ~BIT_EEPROM_WRITE_TRIG;

   // poll on serail_busy flag
   for (i=0 ; i<500 ; i++) {
      Read(T_STATUS, &status, REG_STATUS, 4);
      if ((status & BIT_SERIAL_BUSY) == 0)
         break;
      Sleep(10);
   }

   return 1;
}

/*------------------------------------------------------------------*/

bool DRSBoard::IsTimingCalibrationValid()
{
   return fabs(fNominalFrequency - fTimingCalibratedFrequency) < 0.001;
}

double DRSBoard::GetTrueFrequency()
{
   int i;
   double f;
   
   if (IsTimingCalibrationValid()) {
      /* calculate true frequency */
      for (i=0,f=0 ; i<1024 ; i++)
         f += fCellDT[0][0][i];
      f = 1024.0 / f;
   } else
      f = fNominalFrequency;
   
   return f;
}

/*------------------------------------------------------------------*/

int DRSBoard::GetTime(unsigned int chipIndex, int channelIndex, int tc, float *time, bool tcalibrated, bool rotated)
{
   int i, scale, iend;
   double gt0, gt;

   /* for DRS2, please use function below */
   if (fDRSType < 4)
      return GetTime(chipIndex, channelIndex, fNominalFrequency, tc, time, tcalibrated, rotated);

   scale = fDecimation ? 2 : 1;

   if (!IsTimingCalibrationValid() || !tcalibrated) {
      double t0 = tc / fNominalFrequency;
      for (i = 0; i < fChannelDepth; i++) {
         if (rotated)
            time[i] = static_cast < float >(((i*scale+tc) % kNumberOfBins) / fNominalFrequency - t0);
         else
            time[i] = static_cast < float >((i*scale) / fNominalFrequency);
         if (time[i] < 0)
            time[i] += static_cast < float > (kNumberOfBins / fNominalFrequency);
         if (i*scale >= kNumberOfBins)
            time[i] += static_cast < float > (kNumberOfBins / fNominalFrequency);
      }
      return 1;
   }

   time[0] = 0;
   for (i=1 ; i<fChannelDepth ; i++) {
      if (rotated)
         time[i] = time[i-1] + (float)fCellDT[chipIndex][channelIndex][(i-1+tc) % kNumberOfBins];
      else
         time[i] = time[i-1] + (float)fCellDT[chipIndex][channelIndex][(i-1) % kNumberOfADCBins];
   }

   if (channelIndex > 0) {
      // correct all channels to channel 0 (Daniel's method)
      iend = tc >= 700 ? 700+1024 : 700;
      for (i=tc,gt0=0 ; i<iend ; i++)
         gt0 += fCellDT[chipIndex][0][i % 1024];
      
      for (i=tc,gt=0 ; i<iend ; i++)
         gt += fCellDT[chipIndex][channelIndex][i % 1024];
      
      for (i=0 ; i<fChannelDepth ; i++)
         time[i] += (float)(gt0 - gt);
   }
      
   return 1;
}

/*------------------------------------------------------------------*/

int DRSBoard::GetTimeCalibration(unsigned int chipIndex, int channelIndex, int mode, float *time, bool force)
{
   int i;
   float tint;

   /* not implemented for DRS2 */
   if (fDRSType < 4)
      return -1;

   if (!force && !IsTimingCalibrationValid()) {
      for (i = 0; i < kNumberOfBins; i++)
         time[i] = (float) (1/fNominalFrequency);
      return 1;
   }

   if (mode == 0) {
      /* differential nonlinearity */
      for (i=0 ; i<kNumberOfBins ; i++)
         time[i] = static_cast < float > (fCellDT[chipIndex][channelIndex][i]);
   } else {
      /* integral nonlinearity */
      for (i=0,tint=0; i<kNumberOfBins ; i++) {
         time[i] = static_cast < float > (tint - i/fNominalFrequency);
         tint += (float)fCellDT[chipIndex][channelIndex][i];
      }
   }

   return 1;
}

/*------------------------------------------------------------------*/

int DRSBoard::GetTime(unsigned int chipIndex, int channelIndex, double freqGHz, int tc, float *time, bool tcalibrated, bool rotated)
{
   /* for DRS4, use function above */
   if (fDRSType == 4)
      return GetTime(chipIndex, channelIndex, tc, time, tcalibrated, rotated);

   int i, irot;
   DRSBoard::TimeData * init;
   DRSBoard::TimeData::FrequencyData * freq;
   int frequencyMHz = (int)(freqGHz*1000);

   init = GetTimeCalibration(chipIndex);

   if (init == NULL) {
      for (i = 0; i < kNumberOfBins; i++)
         time[i] = static_cast < float >(i / fNominalFrequency);
      return 1;
   }
   freq = NULL;
   for (i = 0; i < init->fNumberOfFrequencies; i++) {
      if (init->fFrequency[i]->fFrequency == frequencyMHz) {
         freq = init->fFrequency[i];
         break;
      }
   }
   if (freq == NULL) {
      for (i = 0; i < kNumberOfBins; i++)
         time[i] = static_cast < float >(i / fNominalFrequency);
      return 1;
   }
   for (i = 0; i < kNumberOfBins; i++) {
      irot = (fStopCell[chipIndex] + i) % kNumberOfBins;
      if (fStopCell[chipIndex] + i < kNumberOfBins)
         time[i] = static_cast < float >((freq->fBin[irot] - freq->fBin[fStopCell[chipIndex]]) / fNominalFrequency);
      else
      time[i] =
          static_cast <
          float
          >((freq->fBin[irot] - freq->fBin[fStopCell[chipIndex]] + freq->fBin[kNumberOfBins - 1] - 2 * freq->fBin[0] +
             freq->fBin[1]) / fNominalFrequency);
   }
   return 1;
}

/*------------------------------------------------------------------*/

bool DRSBoard::InitTimeCalibration(unsigned int chipIndex)
{
   return GetTimeCalibration(chipIndex, true) != NULL;
}

/*------------------------------------------------------------------*/

DRSBoard::TimeData * DRSBoard::GetTimeCalibration(unsigned int chipIndex, bool reinit)
{
   int i, l, index;
   char *cstop;
   char fileName[500];
   char error[240];
   PMXML_NODE node, rootNode, mainNode;

   index = fNumberOfTimeData;
   for (i = 0; i < fNumberOfTimeData; i++) {
      if (fTimeData[i]->fChip == static_cast < int >(chipIndex)) {
         if (!reinit)
            return fTimeData[i];
         else {
            index = i;
            break;
         }
      }
   }

   fTimeData[index] = new DRSBoard::TimeData();
   DRSBoard::TimeData * init = fTimeData[index];

   init->fChip = chipIndex;

   for (i = 0; i < init->kMaxNumberOfFrequencies; i++) {
      if (i <= 499 || (i >= 501 && i <= 999) || (i >= 1001 && i <= 1499) || (i >= 1501 && i <= 1999) || 
          (i >= 2001 && i <= 2499) || i >= 2501)
         continue;
      sprintf(fileName, "%s/board%d/TimeCalib_board%d_chip%d_%dMHz.xml", fCalibDirectory, fBoardSerialNumber,
              fBoardSerialNumber, chipIndex, i);
      rootNode = mxml_parse_file(fileName, error, sizeof(error), NULL);
      if (rootNode == NULL)
         continue;

      init->fFrequency[init->fNumberOfFrequencies] = new DRSBoard::TimeData::FrequencyData();
      init->fFrequency[init->fNumberOfFrequencies]->fFrequency = i;

      mainNode = mxml_find_node(rootNode, "/DRSTimeCalibration");

      for (l = 0; l < kNumberOfBins; l++) {
         node = mxml_subnode(mainNode, l + 2);
         init->fFrequency[init->fNumberOfFrequencies]->fBin[l] = strtod(mxml_get_value(node), &cstop);
      }
      mxml_free_tree(rootNode);
      init->fNumberOfFrequencies++;
   }
   if (init->fNumberOfFrequencies == 0) {
      printf("Board %d --> Could not find time calibration file\n", GetBoardSerialNumber());
   }

   if (index == fNumberOfTimeData)
      fNumberOfTimeData++;

   return fTimeData[index];
}

/*------------------------------------------------------------------*/

void DRSBoard::SetCalibrationDirectory(const char *calibrationDirectoryPath)
{
   strncpy(fCalibDirectory, calibrationDirectoryPath, strlen(calibrationDirectoryPath));
   fCalibDirectory[strlen(calibrationDirectoryPath)] = 0;
};

/*------------------------------------------------------------------*/

void DRSBoard::GetCalibrationDirectory(char *calibrationDirectoryPath)
{
   strncpy(calibrationDirectoryPath, fCalibDirectory, strlen(fCalibDirectory));
   calibrationDirectoryPath[strlen(fCalibDirectory)] = 0;
};

/*------------------------------------------------------------------*/

void DRSBoard::LinearRegression(double *x, double *y, int n, double *a, double *b)
{
   int i;
   double sx, sxx, sy, sxy;

   sx = sxx = sy = sxy = 0;
   for (i = 0; i < n; i++) {
      sx += x[i];
      sxx += x[i] * x[i];
      sy += y[i];
      sxy += x[i] * y[i];
   }

   *a = (n * sxy - sx * sy) / (n * sxx - sx * sx);
   *b = (sy - *a * sx) / n;
}

/*------------------------------------------------------------------*/

void DRSBoard::ReadSingleWaveform(int nChip, int nChan, 
                                  unsigned short wf[kNumberOfChipsMax][kNumberOfChannelsMax][kNumberOfBins],
                                  bool rotated)
{
   int i, j, k, tc;

   StartDomino();
   SoftTrigger();
   while (IsBusy());
   TransferWaves();

   for (i=0 ; i<nChip ; i++) {
      tc = GetTriggerCell(i);

      for (j=0 ; j<nChan ; j++) {
         GetRawWave(i, j, wf[i][j], rotated);
         if (!rotated) {
            for (k=0 ; k<kNumberOfBins ; k++) {
               /* do primary offset calibration */
               wf[i][j][k] = wf[i][j][k] - fCellOffset[j+i*9][(k + tc) % kNumberOfBins] + 32768;
            }
         }
      }
   }
}
      
static unsigned short swf[kNumberOfChipsMax][kNumberOfChannelsMax][kNumberOfBins];
static float          center[kNumberOfChipsMax][kNumberOfChannelsMax][kNumberOfBins];

int DRSBoard::AverageWaveforms(DRSCallback *pcb, int nChip, int nChan, 
                               int prog1, int prog2, unsigned short *awf, int n, bool rotated)
{
   int i, j, k, l, prog, old_prog = 0;
   float cm;

   if (pcb != NULL)
      pcb->Progress(prog1);

   memset(center, 0, sizeof(center));

   for (i=0 ; i<n; i++) {
      ReadSingleWaveform(nChip, nChan, swf, rotated);

      for (j=0 ; j<nChip ; j++) {
         for (k=0 ; k<nChan ; k++) {
            if (i > 5) {
               /* calculate and subtract common mode */
               for (l=0,cm=0 ; l<kNumberOfBins ; l++)
                  cm += swf[j][k][l] - 32768;
               cm /= kNumberOfBins;
               for (l=0 ; l<kNumberOfBins ; l++)
                  center[j][k][l] += swf[j][k][l]- cm;
            }
         }
      }

      prog = (int)(((double)i/n)*(prog2-prog1)+prog1);
      if (prog > old_prog) {
         old_prog = prog;
         if (pcb != NULL)
            pcb->Progress(prog);
      }
   }

   for (i=0 ; i<nChip ; i++)
      for (j=0 ; j<nChan ; j++)
         for (k=0 ; k<kNumberOfBins ; k++)
            awf[(i*nChan+j)*kNumberOfBins+k] = (unsigned short)(center[i][j][k]/(n-6) + 0.5);
   
   return 1;
}

int DRSBoard::RobustAverageWaveforms(DRSCallback *pcb, int nChip, int nChan, 
                               int prog1, int prog2, unsigned short *awf, int n, bool rotated)
{
   int i, j, k, l, prog, old_prog = 0;

   if (pcb != NULL)
      pcb->Progress(prog1);

   Averager *ave = new Averager(nChip, nChan, kNumberOfBins, 200);
                                
   /* fill histograms */
   for (i=0 ; i<n ; i++) {
      ReadSingleWaveform(nChip, nChan, swf, rotated);
      for (j=0 ; j<nChip ; j++)
         for (k=0 ; k<nChan ; k++)
            for (l=0 ; l<kNumberOfBins ; l++)
               ave->Add(j, k, l, swf[j][k][l]);
               
      /* update progress bar */
      prog = (int)(((double)(i+10)/(n+10))*(prog2-prog1)+prog1);
      if (prog > old_prog) {
         old_prog = prog;
         if (pcb != NULL)
            pcb->Progress(prog);
      }
   }

   /*
   FILE *fh = fopen("calib.csv", "wt");
   for (i=40 ; i<60 ; i++) {
      for (j=0 ; j<WFH_SIZE ; j++)
         fprintf(fh, "%d;", wfh[0][0][i][j]);
      fprintf(fh, "\n");
   }
   fclose(fh);
   */

   for (i=0 ; i<nChip ; i++)
      for (j=0 ; j<nChan ; j++)
         for (k=0 ; k<kNumberOfBins ; k++)
            awf[(i*nChan+j)*kNumberOfBins+k] = (unsigned short)ave->RobustAverage(100, i, j, k);

   ave->SaveNormalizedDistribution("wv.csv", 0, 100);
   
   /*
   FILE *fh = fopen("calib.csv", "wt");
   for (i=40 ; i<60 ; i++) {
      fprintf(fh, "%d;", icenter[0][0][0] + (i - WFH_SIZE/2)*16);
      fprintf(fh, "%d;", wfh[0][0][0][i]);
      if (i == 50)
         fprintf(fh, "%d;", awf[0]);
      fprintf(fh, "\n");
   }
   fclose(fh);
   */

   if (pcb != NULL)
      pcb->Progress(prog2);
   
   delete ave;

   return 1;
}

/*------------------------------------------------------------------*/

int idx[4][10] = {
   {  0,  2,  4,  6,  8, 18, 20, 22, 24, 26 },
   {  1,  3,  5,  7, 39, 19, 21, 23, 25, 39 },
   {  9, 11, 13, 15, 17, 27, 29, 31, 33, 35 },
   { 10, 12, 14, 16, 39, 28, 30, 32, 34, 39 },
};

#define F1(x) ((int) (84.0/24 * (x)))
#define F2(x) ((int) (92.0/8 * (x)))

static unsigned short wft[kNumberOfChipsMax*kNumberOfChannelsMax*kNumberOfChipsMax][1024], 
                      wf1[kNumberOfChipsMax*kNumberOfChannelsMax*kNumberOfChipsMax][1024], 
                      wf2[kNumberOfChipsMax*kNumberOfChannelsMax*kNumberOfChipsMax][1024],
                      wf3[kNumberOfChipsMax*kNumberOfChannelsMax*kNumberOfChipsMax][1024];

int DRSBoard::CalibrateVolt(DRSCallback *pcb)
{
int    i, j, nChan, timingChan, chip, config, p, clkon, refclk, trg1, trg2, n_stuck, readchn, casc;
double f, r;
unsigned short buf[1024*16]; // 32 kB
   
   f       = fNominalFrequency;
   r       = fRange;
   clkon   = (GetCtrlReg() & BIT_TCAL_EN) > 0;
   refclk  = (GetCtrlReg() & BIT_REFCLK_SOURCE) > 0;
   trg1    = fTriggerEnable1;
   trg2    = fTriggerEnable2;
   readchn = fNumberOfReadoutChannels;
   casc    = fChannelCascading;

   Init();
   fNominalFrequency = f;
   SetRefclk(refclk);
   SetFrequency(fNominalFrequency, true);
   SetDominoMode(1);
   SetDominoActive(1);
   SetReadoutMode(1);
   SetInputRange(r);
   if (fBoardType == 5 || fBoardType == 7 || fBoardType == 8 || fBoardType == 9)
      SelectClockSource(0);
   else if (fBoardType == 6)
      SetRefclk(refclk);
   EnableTrigger(0, 0);
   if (readchn == 5)
      SetChannelConfig(4, 8, 8); // even channel readout mode
   
   // WSROUT toggling causes some noise, so calibrate that out
   if (casc == 2) {
      if (fTransport == TR_USB2)
         SetChannelConfig(0, 8, 4); 
      else
         SetChannelConfig(7, 8, 4); 
   }

   StartDomino();

   nChan = 0;
   timingChan = 0;

   if (fBoardType == 5 || fBoardType == 7 || fBoardType == 8 || fBoardType == 9) {
      if (fBoardType == 9) {
         nChan = 8;
         timingChan = -1;
      } else {
         nChan = 9;
         timingChan = 8;
      }

      /* measure offset */
      if (fBoardType == 5)
         EnableAcal(0, 0); // no inputs signal is allowed during calibration!
      else
         EnableAcal(1, 0); // disconnect analog inputs
      EnableTcal(0, 0);
      Sleep(100);
      RobustAverageWaveforms(pcb, 1, nChan, 0, 25, wf1[0], 200, true);

      /* measure gain at upper range */
      EnableAcal(1, fRange+0.4);
      EnableTcal(0, 1);
      Sleep(100);
      RobustAverageWaveforms(pcb, 1, nChan, 25, 50, wf2[0], 200, true);

   } else if (fBoardType == 6) {
      if (fTransport == TR_USB2) {
         nChan = 36;
         timingChan = 8;
         memset(wf1, 0, sizeof(wf1));
         memset(wf2, 0, sizeof(wf2));
         memset(wf3, 0, sizeof(wf3));
         for (config=p=0 ; config<4 ; config++) {
            SetChannelConfig(config, 8, 8);

            /* measure offset */
            EnableAcal(1, 0);
            EnableTcal(0, 0);
            Sleep(100);
            RobustAverageWaveforms(pcb, 0, 10, F1(p), F1(p+1), wft[0], 500, true); p++;
            for (i=0 ; i<5 ; i++)
               memcpy(wf1[idx[config][i]], wft[i*2], sizeof(float)*kNumberOfBins);
            RobustAverageWaveforms(pcb, 2, 10, F1(p), F1(p+1), wft[0], 500, true); p++;
            for (i=0 ; i<5 ; i++)
               memcpy(wf1[idx[config][i+5]], wft[i*2], sizeof(float)*kNumberOfBins);

            /* measure gain at +400 mV */
            EnableAcal(1, 0.4);
            EnableTcal(0, 0);
            Sleep(100);
            RobustAverageWaveforms(pcb, 0, 8, F1(p), F1(p+1), wft[0], 500, true); p++;
            for (i=0 ; i<4 ; i++)
               memcpy(wf2[idx[config][i]], wft[i*2], sizeof(float)*kNumberOfBins);
            RobustAverageWaveforms(pcb, 2, 8, F1(p), F1(p+1), wft[0], 500, true); p++;
            for (i=0 ; i<4 ; i++)
               memcpy(wf2[idx[config][i+5]], wft[i*2], sizeof(float)*kNumberOfBins);

            /* measure gain at -400 mV */
            EnableAcal(1, -0.4);
            EnableTcal(0, 1);
            Sleep(100);
            RobustAverageWaveforms(pcb, 0, 8, F1(p), F1(p+1), wft[0], 500, true); p++;
            for (i=0 ; i<4 ; i++)
               memcpy(wf3[idx[config][i]], wft[i], sizeof(float)*kNumberOfBins);
            RobustAverageWaveforms(pcb, 2, 8, F1(p), F1(p+1), wft[0], 500, true); p++;
            for (i=0 ; i<4 ; i++)
               memcpy(wf3[idx[config][i+5]], wft[i], sizeof(float)*kNumberOfBins);
         }
      } else {
         nChan = 36;
         timingChan = 8;
        
         /* measure offset */
         EnableAcal(0, 0); // no inputs signal is allowed during calibration!
         EnableTcal(0, 0);
         Sleep(100);
         RobustAverageWaveforms(pcb, 4, 9, 0, 25, wf1[0], 500, true);

         /* measure gain at upper range */
         EnableAcal(1, fRange+0.4);
         EnableTcal(0, 0);
         Sleep(100);
         RobustAverageWaveforms(pcb, 4, 9, 25, 50, wf2[0], 500, true);
      }
   }

   /* convert offsets and gains to 16-bit values */
   memset(fCellOffset, 0, sizeof(fCellOffset));
   n_stuck = 0;
   for (i=0 ; i<nChan ; i++) {
      for (j=0 ; j<kNumberOfBins; j++) {
         if (i % 9 == timingChan) {
            /* calculate offset and gain for timing channel */
            if (fBoardType == 5 || fBoardType == 7 || fBoardType == 8 || fBoardType == 9) {
               /* we have a +325mV and a -325mV value */
               fCellOffset[i][j] = (unsigned short) ((wf1[i][j]+wf2[i][j])/2+0.5);
               fCellGain[i][j]   = (wf2[i][j] - wf1[i][j])/65536.0*1000 / 650.0;
            } else {
               /* only have offset */
               fCellOffset[i][j] = wf1[i][j];
               fCellGain[i][j]   = 1;
            }
         } else {
            /* calculate offset and gain for data channel */
            fCellOffset[i][j] = wf1[i][j];
            if (fCellOffset[i][j] < 100) {
               // mark stuck pixel
               n_stuck ++;
               fCellOffset[i][j] = 0;
               fCellGain[i][j] = 1;
            } else
               fCellGain[i][j] = (wf2[i][j] - fCellOffset[i][j])/65536.0*1000 / ((0.4+fRange)*1000);
         }

         /* check gain */
         if (fCellGain[i][j] < 0.5 || fCellGain[i][j] > 1.1) {
            if ((fBoardType == 7 || fBoardType == 8 || fBoardType == 9) && i % 2 == 1) {
               /* channels are not connected, so don't print error */
            } else {
               printf("Gain of %6.3lf for channel %2d, cell %4d out of range 0.5 ... 1.1\n",
                  fCellGain[i][j], i, j);
            }
            fCellGain[i][j] = 1;
         }
      }
   }

   /*
   FILE *fh = fopen("calib.txt", "wt");
   for (i=0 ; i<nChan ; i++) {
      fprintf(fh, "CH%02d:", i);
      for (j=0 ; j<20 ; j++)
         fprintf(fh, " %5d", fCellOffset[i][j]-32768);
      fprintf(fh, "\n");
   }
   fclose(fh);
   */

   /* perform secondary calibration */
   if (fBoardType == 5 || fBoardType == 7 || fBoardType == 8 || fBoardType == 9) {
      nChan = 9;
      timingChan = 8;

      /* measure offset */
      if (fBoardType == 5)
         EnableAcal(0, 0); // no inputs signal is allowed during calibration!
      else
         EnableAcal(1, 0); // disconnect analog inputs
      EnableTcal(0, 0);
      Sleep(100);
      AverageWaveforms(pcb, 1, 9, 50, 90, wf1[0], 500, false);
   } else if (fBoardType == 6 && fTransport == TR_VME) {
      nChan = 36;
      timingChan = 8;

      /* measure offset */
      EnableAcal(0, 0); // no inputs signal is allowed during calibration!
      EnableTcal(0, 0);
      Sleep(100);
      AverageWaveforms(pcb, 4, 9, 50, 75, wf1[0], 500, false);
   }

   /* convert offset to 16-bit values */
   memset(fCellOffset2, 0, sizeof(fCellOffset2));
   for (i=0 ; i<nChan ; i++)
      for (j=0 ; j<kNumberOfBins; j++)
         if (i % 9 != timingChan)
            fCellOffset2[i][j] = wf1[i][j];

   /*
   FILE *fh = fopen("calib.txt", "wt");
   for (i=0 ; i<nChan ; i++) {
      for (j=0 ; j<kNumberOfBins; j++)
         fprintf(fh, "%5d: %5d %5d\n", j, fCellOffset2[0][j]-32768, fCellOffset2[1][j]-32768);
      fprintf(fh, "\n");
   }
   fclose(fh);
   */

   if (fBoardType == 9) {
      /* write calibration CH0-CH7 to EEPROM page 1 */
      for (i=0 ; i<8 ; i++)
         for (j=0 ; j<1024; j++) {
            buf[(i*1024+j)*2]   = fCellOffset[i][j];
            buf[(i*1024+j)*2+1] = (unsigned short) ((fCellGain[i][j] - 0.7) / 0.4 * 65535);
         }
      WriteEEPROM(1, buf, 1024*32);
      if (pcb != NULL)
         pcb->Progress(93);

      
      /* write secondary calibration to EEPROM page 2 */
      ReadEEPROM(2, buf, 1024*32);
      for (i=0 ; i<8 ; i++)
         for (j=0 ; j<1024; j++)
            buf[(i*1024+j)*2] = fCellOffset2[i][j];
      WriteEEPROM(2, buf, 1024*32);
      if (pcb != NULL)
         pcb->Progress(96);

      /* update page # 0 */
      ReadEEPROM(0, buf, 4096); // 0-0x0FFF
      /* write calibration method */
      buf[2]  = (buf[2] & 0xFF00) | VCALIB_METHOD;
      /* write temperature and range */
      buf[10] = ((unsigned short) (GetTemperature() * 2 + 0.5) << 8) | ((signed char)(fRange * 100));
      buf[11] = 1; // EEPROM page #1+2
      WriteEEPROM(0, buf, 4096);
      fCellCalibratedRange = fRange;
      if (pcb != NULL)
         pcb->Progress(100);

      
   } else if (fBoardType == 5 || fBoardType == 7 || fBoardType == 8) {
      /* write calibration CH0-CH7 to EEPROM page 1 */
      for (i=0 ; i<8 ; i++)
         for (j=0 ; j<1024; j++) {
            buf[(i*1024+j)*2]   = fCellOffset[i][j];
            buf[(i*1024+j)*2+1] = (unsigned short) ((fCellGain[i][j] - 0.7) / 0.4 * 65535);
         }
      WriteEEPROM(1, buf, 1024*32);

      /* write calibration CH8 and secondary calibration to EEPROM page 2 */
      for (j=0 ; j<1024; j++) {
         buf[j*2]   = fCellOffset[8][j];
         buf[j*2+1] = (unsigned short) ((fCellGain[8][j] - 0.7) / 0.4 * 65535);
      }
      for (i=0 ; i<4 ; i++)
         for (j=0 ; j<1024; j++) {
            buf[2*1024+(i*1024+j)*2]   = fCellOffset2[i*2][j];
            buf[2*1024+(i*1024+j)*2+1] = fCellOffset2[i*2+1][j];
         }
      WriteEEPROM(2, buf, 1024*5*4);

      /* write calibration method and range */
      ReadEEPROM(0, buf, 2048); // 0-0x0FFF
      buf[2] = VCALIB_METHOD_V4 | ((signed char)(fRange * 100)) << 8;
      WriteEEPROM(0, buf, 2048);
      fCellCalibratedRange = fRange;

   } else if (fBoardType == 6) {
      for (chip=0 ; chip<4 ; chip++) {
         /* write calibration of A0 to A7 to EEPROM page 1 
                                 B0 to B7 to EEPROM page 2 and so on */
         for (i=0 ; i<8 ; i++)
            for (j=0 ; j<1024; j++) {
               buf[(i*1024+j)*2]   = fCellOffset[i+chip*9][j];
               buf[(i*1024+j)*2+1] = (unsigned short) ((fCellGain[i+chip*9][j] - 0.7) / 0.4 * 65535);
            }
         WriteEEPROM(1+chip, buf, 1024*32);
         if (pcb != NULL)
            pcb->Progress(75+chip*4);
       }

      /* write calibration A/B/C/D/CLK to EEPROM page 5 */
      ReadEEPROM(5, buf, 1024*4*4);
      for (chip=0 ; chip<4 ; chip++) {
         for (j=0 ; j<1024; j++) {
            buf[j*2+chip*0x0800]   = fCellOffset[8+chip*9][j];
            buf[j*2+1+chip*0x0800] = (unsigned short) ((fCellGain[8+chip*9][j] - 0.7) / 0.4 * 65535);
         }
      }
      WriteEEPROM(5, buf, 1024*4*4);
      if (pcb != NULL)
         pcb->Progress(90);

      /* write secondary calibration to EEPROM page 7 and 8 */
      for (i=0 ; i<8 ; i++) {
         for (j=0 ; j<1024; j++) {
            buf[i*0x800 + j*2]   = fCellOffset2[i][j];
            buf[i*0x800 + j*2+1] = fCellOffset2[i+9][j];
         }
      }
      WriteEEPROM(7, buf, 1024*32);
      if (pcb != NULL)
         pcb->Progress(94);

      for (i=0 ; i<8 ; i++) {
         for (j=0 ; j<1024; j++) {
            buf[i*0x800 + j*2]   = fCellOffset2[i+18][j];
            buf[i*0x800 + j*2+1] = fCellOffset2[i+27][j];
         }
      }
      WriteEEPROM(8, buf, 1024*32);
      if (pcb != NULL)
         pcb->Progress(98);

      /* write calibration method and range */
      ReadEEPROM(0, buf, 2048); // 0-0x0FFF
      buf[2] = VCALIB_METHOD | ((signed char)(fRange * 100)) << 8;
      WriteEEPROM(0, buf, 2048);
      fCellCalibratedRange = fRange;
      if (pcb != NULL)
         pcb->Progress(100);
   }

   if (n_stuck)
      printf("\nFound %d stuck pixels on this board\n", n_stuck);

   fVoltageCalibrationValid = true;

   /* remove calibration voltage */
   EnableAcal(0, 0);
   EnableTcal(clkon, 0);
   EnableTrigger(trg1, trg2);

   return 1;
}

/*------------------------------------------------------------------*/

int DRSBoard::AnalyzeSlope(Averager *ave, int iIter, int nIter, int channel, float wf[kNumberOfBins], int tCell,
                           double cellDV[kNumberOfBins], double cellDT[kNumberOfBins])
{
   int i;
   float dv, llim, ulim;
   double sum, dtCell;
   
   if (fNominalFrequency > 3) {
      llim = -100;
      ulim =  100;
   } else {
      llim = -300;
      ulim =  300;
   }

   // rising edges ----

   // skip first cells after trigger cell
   for (i=tCell+5 ; i<tCell+kNumberOfBins-5 ; i++) {
      // test slope between previous and next cell to allow for negative cell width
      if (wf[(i+kNumberOfBins-1) % kNumberOfBins] < wf[(i+2) % kNumberOfBins] &&
          wf[i % kNumberOfBins] > llim &&
          wf[(i+1) % kNumberOfBins] < ulim) {
         
         // calculate delta_v
         dv = wf[(i+1) % kNumberOfBins] - wf[i % kNumberOfBins];
    
         // average delta_v
         ave->Add(0, channel, i % kNumberOfBins, dv);
      }
   }

   // falling edges ----
   
   // skip first cells after trigger cell
   for (i=tCell+5 ; i<tCell+kNumberOfBins-5 ; i++) {
      // test slope between previous and next cell to allow for negative cell width
      if (wf[(i+kNumberOfBins-1) % kNumberOfBins] > wf[(i+2) % kNumberOfBins] &&
          wf[i % kNumberOfBins] < ulim &&
          wf[(i+1) % kNumberOfBins] > llim) {
         
         // calcualte delta_v
         dv = wf[(i+1) % kNumberOfBins] - wf[i % kNumberOfBins];
         
         ave->Add(0, channel, i % kNumberOfBins, -dv);
      }
   }
   
   // calculate calibration every 100 events
   if ((iIter + 1) % 100 == 0) {
      // average over all 1024 dU
      sum = 0;
      for (i=0 ; i<kNumberOfBins ; i++) {
         
         if (fBoardType == 8)
            cellDV[i] = ave->Median(0, channel, i);
         else {
            // use empirically found limits for averaging
            if (fNominalFrequency >= 4)
               cellDV[i] = ave->RobustAverage(3, 0, channel, i);
            else if (fNominalFrequency >= 3)
               cellDV[i] = ave->RobustAverage(6, 0, channel, i);
            else
               cellDV[i] = ave->Median(0, channel, i);
         }
         
         sum += cellDV[i];
      }
      
      sum /= kNumberOfBins;
      dtCell = (float)1/fNominalFrequency;
      
      // here comes the central calculation, dT = dV/average * dt_cell
      for (i=0 ; i<kNumberOfBins ; i++)
         cellDT[i] = cellDV[i] / sum * dtCell;
   }
   
   return 1;
}

/*------------------------------------------------------------------*/

int DRSBoard::AnalyzePeriod(Averager *ave, int iIter, int nIter, int channel, float wf[kNumberOfBins], int tCell,
                            double cellDV[kNumberOfBins], double cellDT[kNumberOfBins])
{
int    i, i1, i2, j, nzx, zeroXing[1000], edge, n_correct, nest;
double damping, zeroLevel, tPeriod, corr, dv, dv_limit, corr_limit;
   
   /* calculate zero level */
   for (i=0,zeroLevel=0 ; i<1024 ; i++)
      zeroLevel += wf[i];
   zeroLevel /= 1024;

   /* correct for zero common mode */
   for (i=0 ; i<1024 ; i++)
      wf[i] -= (float)zeroLevel;

   /* estimate damping factor */
   if (fBoardType == 9)
      damping = 0.01;
   else
      damping = fNominalFrequency / nIter * 2   ;

   /* estimate number of zero crossings */
   nest = (int) (1/fNominalFrequency*1024 / (1/fTCALFrequency*1000));
   
   if (fNominalFrequency >= 4)
      dv_limit = 4;
   else if (fNominalFrequency >= 3)
      dv_limit = 6;
   else
      dv_limit = 10000;

   for (edge = 0 ; edge < 2 ; edge ++) {

      /* find edge zero crossing with wrap-around */
      for (i=tCell+5,nzx=0 ; i<tCell+1023-5 && nzx < (int)(sizeof(zeroXing)/sizeof(int)) ; i++) {
         dv = fabs(wf[(i+1) % 1024] - wf[i % 1024]);
         
         if (edge == 0) {
            if (wf[(i+1) % 1024] < 0 && wf[i % 1024] > 0) { // falling edge
               if (fBoardType != 9 || fabs(dv-cellDV[i % 1024]) < dv_limit) // only if DV is not more the dv_limit away from average
                  zeroXing[nzx++] = i % 1024;
            }
         } else {
            if (wf[(i+1) % 1024] > 0 && wf[i % 1024] < 0) { // rising edge
               if (fBoardType != 9 || fabs(dv-cellDV[i % 1024]) < dv_limit)
                  zeroXing[nzx++] = i % 1024;
            }
         }
      }

      /* abort if uncorrect number of edges is found */
      if (abs(nest - nzx) > nest / 3)
         return 0;

      for (i=n_correct=0 ; i<nzx-1 ; i++) {
         i1 = zeroXing[i];
         i2 = zeroXing[i+1];
         if (i1 == 1023 || i2 == 1023)
            continue;
         
         if (wf[(i1 + 1) % 1024] == 0 || wf[i2 % 1024] == 0)
            continue;
         
         /* first partial cell */
         tPeriod = cellDT[i1]*(1/(1-wf[i1]/wf[(i1 + 1) % 1024]));

         /* full cells between i1 and i2 */
         if (i2 < i1)
            i2 += 1024;
         
         for (j=i1+1 ; j<i2 ; j++)
            tPeriod += cellDT[j % 1024];
         
         /* second partial cell */
         tPeriod += cellDT[i2 % 1024]*(1/(1-wf[(i2+1) % 1024]/wf[i2 % 1024]));

         /* calculate correction to nominal period as a fraction */
         corr = (1/fTCALFrequency*1000) / tPeriod;

         /* skip very large corrections (noise) */
         if (fBoardType == 9 && fNominalFrequency >= 2)
            corr_limit = 0.001;
         else if (fBoardType == 9)
            corr_limit = 0.004;
         else
            corr_limit = 0.01;

         if (fBoardType == 9 && fabs(1-corr) > corr_limit)
            continue;

         /* remeber number of valid corrections */
         n_correct++;

         /* apply damping factor */
         corr = (corr-1)*damping + 1;

         /* apply from i1 to i2-1 inclusive */
         if (i1 == i2)
            continue;

         /* distribute correciton equally into bins inside the region ... */
         for (j=i1 ; j<=i2 ; j++)
            cellDT[j % 1024] *= corr;
            
         /* test correction */
         tPeriod = cellDT[i1]*(1/(1-wf[i1]/wf[(i1 + 1) % 1024]));
         for (j=i1+1 ; j<i2 ; j++)
            tPeriod += cellDT[j % 1024];
         tPeriod += cellDT[i2]*(1/(1-wf[(i2+1) % 1024]/wf[i2]));
      }

      if (n_correct < nzx/3)
         return 0;
   }

   return 1;
}

/*------------------------------------------------------------------*/


int DRSBoard::CalibrateTiming(DRSCallback *pcb)
{
   int    index, status, error, tCell, i, j, c, chip, mode, nIterPeriod, nIterSlope, clkon, phase, refclk, trg1, trg2, n_error, channel;
   double f, range, tTrue, tRounded, dT, t1[8], t2[8], cellDV[kNumberOfChipsMax*kNumberOfChannelsMax][kNumberOfBins];
   unsigned short buf[1024*16]; // 32 kB
   float  wf[1024];
   Averager *ave = NULL;
   
   nIterPeriod = 5000;
   nIterSlope  = 5000;
   n_error = 0;
   refclk  = 0;
   f       = fNominalFrequency;
   range   = fRange;
   clkon   = (GetCtrlReg() & BIT_TCAL_EN) > 0;
   if (fBoardType == 6)
      refclk  = (GetCtrlReg() & BIT_REFCLK_SOURCE) > 0;
   trg1    = fTriggerEnable1;
   trg2    = fTriggerEnable2;

   Init();
   fNominalFrequency = f;
   fTimingCalibratedFrequency = 0;
   if (fBoardType == 6) // don't set refclk for evaluation boards
      SetRefclk(refclk);
   SetFrequency(fNominalFrequency, true);
   if (fBoardType == 5 || fBoardType == 7 || fBoardType == 8)
      fTCALFrequency = 132; // 132 MHz for EVAL1, for MEZZ this is set by ConfigureLMK
   else if (fBoardType == 9)
      fTCALFrequency = 100;
   SetDominoMode(1);
   SetDominoActive(1);
   SetReadoutMode(1);
   EnableTrigger(0, 0);
   if (fBoardType == 5 || fBoardType == 7) {
      EnableTcal(1, 0, 0);
      SelectClockSource(1); // 2nd quartz
   } else if (fBoardType == 8) {
      nIterSlope  = 0;
      nIterPeriod = 1500;
      EnableTcal(1, 0, 0);
      SelectClockSource(1); // 2nd quartz
      ave = new Averager(1, 1, 1024, 500); // one chip, 1 channel @ 1024 bins
   } else if (fBoardType == 9) {
      EnableTcal(1);
      nIterSlope  = 500;
      nIterPeriod = 500;
      ave = new Averager(1, 9, 1024, 500); // one chip, 9 channels @ 1024 bins
   }
   StartDomino();

   /* initialize time array */
   for (i=0 ; i<1024 ; i++)
      for (chip=0 ; chip<4 ; chip++)
         for (channel = 0 ; channel < 8 ; channel++) {
            fCellDT[chip][channel][i] = (float)1/fNominalFrequency;  // [ns]
         }

   error = 0;
   
   for (index = 0 ; index < nIterSlope+nIterPeriod ; index++) {
      if (index % 10 == 0)
         if (pcb)
            pcb->Progress(100*index/(nIterSlope+nIterPeriod));

      if (fTransport == TR_VME) {
         SoftTrigger();
         while (IsBusy());

         /* select random phase */
         phase = (rand() % 30) - 15;
         if (phase == 0)
            phase = 15;
         EnableTcal(1, 0, phase);

         StartDomino();
         TransferWaves();

         for (chip=0 ; chip<4 ; chip++) {
            tCell = GetStopCell(chip);
            GetWave(chip, 8, wf, true, tCell, 0, true);
            status = AnalyzePeriod(ave, index, nIterPeriod, 0, wf, tCell, cellDV[chip], fCellDT[chip][0]);

            if (!status)
               n_error++;

            if (n_error > nIterPeriod / 10) {
               error = 1;
               break;
            }
         }
      } else {
         if (fBoardType == 5 || fBoardType == 7 || fBoardType == 8) { // DRS4 Evaluation board: 1 Chip
            SoftTrigger();
            while (IsBusy());

            StartDomino();
            TransferWaves();

            tCell = GetStopCell(0);
            GetWave(0, 8, wf, true, tCell, 0, true);
            
            if (index < nIterSlope)
               status = AnalyzeSlope(ave, index, nIterSlope, 0, wf, tCell, cellDV[0], fCellDT[0][0]);
            else
               status = AnalyzePeriod(ave, index, nIterPeriod, 0, wf, tCell, cellDV[0], fCellDT[0][0]);

            if (!status)
               n_error++;

            if (n_error > nIterPeriod / 10) {
               error = 1;
               break;
            }
         } else if (fBoardType == 9) { // DRS4 Evaluation board V5: all channels from one chip
            SoftTrigger();
            while (IsBusy());
            
            StartDomino();
            TransferWaves();
            
            // calibrate all channels individually
            for (channel = 0 ; channel < 8 ; channel+=2) {
               tCell = GetStopCell(0);
               GetWave(0, channel, wf, true, tCell, 0, true);
               
               if (index < nIterSlope)
                  status = AnalyzeSlope(ave, index, nIterSlope, channel, wf, tCell, cellDV[channel], fCellDT[0][channel]);
               else
                  status = AnalyzePeriod(ave, index, nIterPeriod, channel, wf, tCell, cellDV[channel], fCellDT[0][channel]);
               
               if (!status)
                  n_error++;
               
               if (n_error > nIterPeriod / 2) {
                  error = 1;
                  break;
               }
            }
            if (!status)
               break;

         } else {               // DRS4 Mezzanine board: 4 Chips
            for (mode=0 ; mode<2 ; mode++) {
               SetChannelConfig(mode*2, 8, 8);
               SoftTrigger();
               while (IsBusy());

               /* select random phase */
               phase = (rand() % 30) - 15;
               if (phase == 0)
                  phase = 15;
               EnableTcal(1, 0, phase);

               StartDomino();
               TransferWaves();

               for (chip=0 ; chip<4 ; chip+=2) {
                  tCell = GetStopCell(chip+mode);
                  GetWave(chip+mode, 8, wf, true, tCell, 0, true);
                  status = AnalyzePeriod(ave, index, nIterPeriod, 0, wf, tCell, cellDV[chip+mode], fCellDT[chip+mode][0]);

                  if (!status) {
                     error = 1;
                     break;
                  }
               }
               if (!status)
                  break;

            }
         }
      }
   }

   if (pcb)
      pcb->Progress(100);
   
   // DRS4 Evaluation board V5: copy even channels to odd channels (usually not connected)
   if (fBoardType == 9) {
      for (channel = 0 ; channel < 8 ; channel+=2)
         memcpy(fCellDT[0][channel+1], fCellDT[0][channel], sizeof(unsigned short)*1024);
   }

   // use following lines to save calibration into an ASCII file
#if 0
   FILE *fh;

   fh = fopen("cellt.csv", "wt");
   if (!fh)
      printf("Cannot open file \"cellt.csv\"\n");
   else {
      fprintf(fh, "index,dt_ch1,dt_ch2,dt_ch3,dt_ch4\n");
      for (i=0 ; i<1024 ; i++)
         fprintf(fh, "%4d,%5.3lf,%5.3lf,%5.3lf,%5.3lf\n", i, fCellDT[0][0][i], fCellDT[0][2][i], fCellDT[0][4][i], fCellDT[0][6][i]);
      fclose(fh);
   }
#endif
   
   if (fBoardType == 5 || fBoardType == 7 || fBoardType == 8) {
      /* write timing calibration to EEPROM page 0 */
      ReadEEPROM(0, buf, sizeof(buf));
      for (i=0,t1[0]=0 ; i<1024; i++)
         buf[i*2+1] = (unsigned short) (fCellDT[0][0][i] * 10000 + 0.5);

      /* write calibration method and frequency */
      buf[4] = TCALIB_METHOD_V4;
      buf[6] = (unsigned short)(fNominalFrequency*1000+0.5);
      
      fTimingCalibratedFrequency = fNominalFrequency;
      WriteEEPROM(0, buf, sizeof(buf));
      
      // copy calibration to all channels
      for (i=1 ; i<8 ; i++)
         for (j=0 ; j<1024; j++)
            fCellDT[0][i][j] = fCellDT[0][0][j];

   } else if (fBoardType == 9) {
  
      /* write timing calibration to EEPROM page 2 */
      ReadEEPROM(2, buf, sizeof(buf));
      for (i=0 ; i<8 ; i++) {
         tTrue = 0;    // true cellT
         tRounded = 0; // rounded cellT
         for (j=0 ; j<1024; j++) {
            tTrue += fCellDT[0][i][j];
            dT = tTrue - tRounded;
            // shift by 1 ns to allow negative widths
            dT = (unsigned short) (dT*10000+1000+0.5);
            tRounded += (dT - 1000) / 10000.0;
            buf[(i*1024+j)*2+1] = (unsigned short) dT;
         }
      }
      WriteEEPROM(2, buf, sizeof(buf));
      
      /* write calibration method and frequency to EEPROM page 0 */
      ReadEEPROM(0, buf, sizeof(buf));
      buf[4] = 1; // number of calibrations
      buf[2] = (TCALIB_METHOD << 8) | (buf[2] & 0xFF); // calibration method
      float fl = (float) fNominalFrequency;
      memcpy(&buf[8], &fl, sizeof(float)); // exact freqeuncy
      
      fTimingCalibratedFrequency = fNominalFrequency;
      WriteEEPROM(0, buf, sizeof(buf));
   
   } else {
   
      /* write timing calibration to EEPROM page 6 */
      ReadEEPROM(6, buf, sizeof(buf));
      for (c=0 ; c<4 ; c++)
         t1[c] = 0;
      for (i=0 ; i<1024; i++) {
         for (c=0 ; c<4 ; c++) {
            t2[c] = fCellDT[0][c][i] - t1[c];
            t2[c] = (unsigned short) (t2[c] * 10000 + 0.5);
            t1[c] += t2[c] / 10000.0;
         }
         buf[i*2]         = (unsigned short) t2[0];
         buf[i*2+1]       = (unsigned short) t2[1];
         buf[i*2+0x800]   = (unsigned short) t2[2];
         buf[i*2+0x800+1] = (unsigned short) t2[3];
      }
      WriteEEPROM(6, buf, sizeof(buf));

      /* write calibration method and frequency */
      ReadEEPROM(0, buf, 16);
      buf[4] = TCALIB_METHOD;
      buf[6] = (unsigned short) (fNominalFrequency * 1000 + 0.5);
      fTimingCalibratedFrequency = buf[6] / 1000.0;
      WriteEEPROM(0, buf, 16);
   }

   if (ave)
      delete ave;

   /* remove calibration voltage */
   EnableAcal(0, 0);
   EnableTcal(clkon, 0);
   SetInputRange(range);
   EnableTrigger(trg1, trg2);

   if (error)
      return 0;

   return 1;
}


/*------------------------------------------------------------------*/


void DRSBoard::RemoveSymmetricSpikes(short **wf, int nwf,
                                     short diffThreshold, int spikeWidth,
                                     short maxPeakToPeak, short spikeVoltage,
                                     int nTimeRegionThreshold)
{
   // Remove a specific kind of spike on DRS4.
   // This spike has some features,
   //  - Common on all the channels on a chip
   //  - Constant heigh and width
   //  - Two spikes per channel
   //  - Symmetric to cell #0.
   //
   // This is not general purpose spike-removing function.
   // 
   // wf                   : Waveform data. cell#0 must be at bin0,
   //                        and number of bins must be kNumberOfBins.
   // nwf                  : Number of channels which "wf" holds.
   // diffThreshold        : Amplitude threshold to find peak
   // spikeWidth           : Width of spike
   // maxPeakToPeak        : When peak-to-peak is larger than this, the channel
   //                        is not used to find spikes.
   // spikeVoltage         : Amplitude of spikes. When it is 0, it is calculated in this function
   //                        from voltage difference from neighboring bins.
   // nTimeRegionThreshold : Requirement of number of time regions having spike at common position.
   //                        Total number of time regions is 2*"nwf".

   if (!wf || !nwf || !diffThreshold || !spikeWidth) {
      return;
   }

   int          ibin, jbin, kbin;
   double       v;
   int          nbin;
   int          iwf;
   short        maximum, minimum;
   int          spikeCount[kNumberOfBins / 2];
   int          spikeCountSum[kNumberOfBins / 2] = {0};
   bool         largePulse[kNumberOfChannelsMax * 2] = {0};
   const short  diffThreshold2 = diffThreshold + diffThreshold;

   const short  maxShort = 0xFFFF>>1;
   const short  minShort = -maxShort - 1;

   // search spike
   for (iwf = 0; iwf < nwf; iwf++) {
      // first half
      memset(spikeCount, 0, sizeof(spikeCount));
      maximum = minShort;
      minimum = maxShort;
      for (ibin = 0; ibin < kNumberOfBins / 2; ibin++) {
         jbin = ibin;
         maximum = max(maximum, wf[iwf][jbin]);
         minimum = min(minimum, wf[iwf][jbin]);
         if (jbin - 1 >= 0 && jbin + spikeWidth < kNumberOfBins) {
            v = 0;
            nbin = 0;
            for (kbin = 0; kbin < spikeWidth; kbin++) {
               v += wf[iwf][jbin + kbin];
               nbin++;
            }
            if ((nbin == 2 && v - (wf[iwf][jbin - 1] + wf[iwf][jbin + spikeWidth]) > diffThreshold2) ||
                (nbin != 2 && nbin && v / nbin - (wf[iwf][jbin - 1] + wf[iwf][jbin + spikeWidth]) / 2 > diffThreshold)) {
               spikeCount[ibin]++;
            }
         }
      }
      if (maximum != minShort && minimum != maxShort &&
          (!maxPeakToPeak || maximum - minimum < maxPeakToPeak)) {
         for (ibin = 0; ibin < kNumberOfBins / 2; ibin++) {
            spikeCountSum[ibin] += spikeCount[ibin];
         }
         largePulse[iwf] = false;
#if 0 /* this part can be enabled to skip checking other channels */
         if (maximum != minShort && minimum != maxShort &&
             maximum - minimum < diffThreshold) {
            return;
         }
#endif
      } else {
         largePulse[iwf] = true;
      }

      // second half
      memset(spikeCount, 0, sizeof(spikeCount));
      maximum = minShort;
      minimum = maxShort;
      for (ibin = 0; ibin < kNumberOfBins / 2; ibin++) {
         jbin = kNumberOfBins - 1 - ibin;
         maximum = max(maximum, wf[iwf][jbin]);
         minimum = min(minimum, wf[iwf][jbin]);
         if (jbin + 1 < kNumberOfBins && jbin - spikeWidth >= 0) {
            v = 0;
            nbin = 0;
            for (kbin = 0; kbin < spikeWidth; kbin++) {
               v += wf[iwf][jbin - kbin];
               nbin++;
            }
            if ((nbin == 2 && v - (wf[iwf][jbin + 1] + wf[iwf][jbin - spikeWidth]) > diffThreshold2) ||
                (nbin != 2 && nbin && v / nbin - (wf[iwf][jbin + 1] + wf[iwf][jbin - spikeWidth]) / 2 > diffThreshold)) {
               spikeCount[ibin]++;
            }
         }
      }
      if (maximum != minShort && minimum != maxShort &&
          maximum - minimum < maxPeakToPeak) {
         for (ibin = 0; ibin < kNumberOfBins / 2; ibin++) {
            spikeCountSum[ibin] += spikeCount[ibin];
         }
         largePulse[iwf + nwf] = false;
#if 0 /* this part can be enabled to skip checking other channels */
         if (maximum != minShort && minimum != maxShort &&
             maximum - minimum < diffThreshold) {
            return;
         }
#endif
      } else {
         largePulse[iwf + nwf] = true;
      }
   }

   // Find common spike
   int commonSpikeBin = -1;
   int commonSpikeMax = -1;
   for (ibin = 0; ibin < kNumberOfBins / 2; ibin++) {
      if (commonSpikeMax < spikeCountSum[ibin]) {
         commonSpikeMax = spikeCountSum[ibin];
         commonSpikeBin = ibin;
      }
   } 

   if (spikeCountSum[commonSpikeBin] >= nTimeRegionThreshold) {
      if (spikeVoltage == 0) {
         // Estimate spike amplitude
         double  baseline      = 0;
         int    nBaseline      = 0;
         double  peakAmplitude = 0;
         int    nPeakAmplitude = 0;
         for (iwf = 0; iwf < nwf; iwf++) {
            // first half
            if (!largePulse[iwf]) {
               // baseline
               if ((jbin = commonSpikeBin - 1) >= 0 && jbin < kNumberOfBins) {
                  baseline += wf[iwf][jbin];
                  nBaseline++;
               }
               if ((jbin = commonSpikeBin + spikeWidth + 1) >= 0 && jbin < kNumberOfBins) {
                  baseline += wf[iwf][jbin];
                  nBaseline++;
               }
               // spike
               for (ibin = 0; ibin < spikeWidth; ibin++) {
                  if ((jbin = commonSpikeBin + ibin) >= 0 && jbin < kNumberOfBins) {
                     peakAmplitude += wf[iwf][jbin];
                     nPeakAmplitude++;
                  }
               }
            }

            // second half
            if (!largePulse[iwf + nwf]) {
               // baseline
               if ((jbin = kNumberOfBins - 1 - commonSpikeBin + 1) >= 0 && jbin < kNumberOfBins) {
                  baseline += wf[iwf][jbin];
                  nBaseline++;
               }
               if ((jbin = kNumberOfBins - 1 - commonSpikeBin - spikeWidth - 1) >= 0 && jbin < kNumberOfBins) {
                  baseline += wf[iwf][jbin];
                  nBaseline++;
               }
               // spike
               for (ibin = 0; ibin < spikeWidth; ibin++) {
                  if ((jbin = kNumberOfBins - 1 - commonSpikeBin - ibin) >= 0 && jbin < kNumberOfBins) {
                     peakAmplitude += wf[iwf][jbin];
                     nPeakAmplitude++;
                  }
               }
            }
         }
         if (nBaseline && nPeakAmplitude) {
            baseline /= nBaseline;
            peakAmplitude /= nPeakAmplitude;
            spikeVoltage = static_cast<short>(peakAmplitude - baseline);
         } else {
            spikeVoltage = 0;
         }
      }

      // Remove spike
      if (spikeVoltage > 0) {
         for (iwf = 0; iwf < nwf; iwf++) {
            for (ibin = 0; ibin < spikeWidth; ibin++) {
               if ((jbin = commonSpikeBin + ibin) >= 0 && jbin < kNumberOfBins) {
                  wf[iwf][jbin] -= spikeVoltage; 
               }
               if ((jbin = kNumberOfBins - 1 - commonSpikeBin - ibin) >= 0 && jbin < kNumberOfBins) {
                  wf[iwf][jbin] -= spikeVoltage; 
               }
            }
         }
      }
   }
}

/*------------------------------------------------------------------*/

void ResponseCalibration::SetCalibrationParameters(int numberOfPointsLowVolt, int numberOfPoints,
                                                   int numberOfMode2Bins, int numberOfSamples,
                                                   int numberOfGridPoints, int numberOfXConstPoints,
                                                   int numberOfXConstGridPoints, double triggerFrequency,
                                                   int showStatistics)
{
   DeleteFields();
   InitFields(numberOfPointsLowVolt, numberOfPoints, numberOfMode2Bins, numberOfSamples, numberOfGridPoints,
              numberOfXConstPoints, numberOfXConstGridPoints, triggerFrequency, showStatistics);
}

/*------------------------------------------------------------------*/

void ResponseCalibration::ResetCalibration()
{
   int i;
   for (i = 0; i < kNumberOfChipsMax; i++)
      fCalibrationData[i]->fRead = false;
   fCurrentPoint = 0;
   fCurrentLowVoltPoint = 0;
   fCurrentSample = 0;
   fCurrentFitChannel = 0;
   fCurrentFitBin = 0;
   fRecorded = false;
   fFitted = false;
   fOffset = false;
};

/*------------------------------------------------------------------*/

bool ResponseCalibration::WriteCalibration(unsigned int chipIndex)
{
   if (!fOffset)
      return false;
   if (fBoard->GetDRSType() == 3)
      return WriteCalibrationV4(chipIndex);
   else
      return WriteCalibrationV3(chipIndex);
}

/*------------------------------------------------------------------*/

bool ResponseCalibration::WriteCalibrationV3(unsigned int chipIndex)
{
   if (!fOffset)
      return false;

   int ii, j, k;
   char str[1000];
   char strt[1000];
   short tempShort;
   CalibrationData *data = fCalibrationData[chipIndex];
   CalibrationData::CalibrationDataChannel * chn;

   // Open File
   fBoard->GetCalibrationDirectory(strt);
   sprintf(str, "%s/board%d", strt, fBoard->GetBoardSerialNumber());
   if (MakeDir(str) == -1) {
      printf("Error: Cannot create directory \"%s\"\n", str);
      return false;
   }
   sprintf(str, "%s/board%d/ResponseCalib_board%d_chip%d_%dMHz.bin", strt, fBoard->GetBoardSerialNumber(),
           fBoard->GetBoardSerialNumber(), chipIndex, static_cast < int >(fBoard->GetNominalFrequency() * 1000));
   fCalibFile = fopen(str, "wb");
   if (fCalibFile == NULL) {
      printf("Error: Cannot write to file \"%s\"\n", str);
      return false;
   }
   // Write File
   fwrite(&data->fNumberOfGridPoints, 1, 1, fCalibFile);
   tempShort = static_cast < short >(data->fStartTemperature) * 10;
   fwrite(&tempShort, 2, 1, fCalibFile);
   tempShort = static_cast < short >(data->fEndTemperature) * 10;
   fwrite(&tempShort, 2, 1, fCalibFile);
   fwrite(&data->fMin, 4, 1, fCalibFile);
   fwrite(&data->fMax, 4, 1, fCalibFile);
   fwrite(&data->fNumberOfLimitGroups, 1, 1, fCalibFile);

   for (ii = 0; ii < kNumberOfCalibChannelsV3; ii++) {
      chn = data->fChannel[ii];
      for (j = 0; j < kNumberOfBins; j++) {
         fwrite(&chn->fLimitGroup[j], 1, 1, fCalibFile);
         fwrite(&chn->fLookUpOffset[j], 2, 1, fCalibFile);
         fwrite(&chn->fNumberOfLookUpPoints[j], 1, 1, fCalibFile);
         for (k = 0; k < chn->fNumberOfLookUpPoints[j]; k++) {
            fwrite(&chn->fLookUp[j][k], 1, 1, fCalibFile);
         }
         for (k = 0; k < data->fNumberOfGridPoints; k++) {
            fwrite(&chn->fData[j][k], 2, 1, fCalibFile);
         }
         fwrite(&chn->fOffsetADC[j], 2, 1, fCalibFile);
         fwrite(&chn->fOffset[j], 2, 1, fCalibFile);
      }
   }
   fclose(fCalibFile);

   printf("Calibration successfully written to\n\"%s\"\n", str);
   return true;
}

/*------------------------------------------------------------------*/

bool ResponseCalibration::WriteCalibrationV4(unsigned int chipIndex)
{
   if (!fOffset)
      return false;

   int ii, j;
   char str[1000];
   char strt[1000];
   CalibrationData *data = fCalibrationData[chipIndex];
   CalibrationData::CalibrationDataChannel * chn;

   // Open File
   fBoard->GetCalibrationDirectory(strt);
   sprintf(str, "%s/board%d", strt, fBoard->GetBoardSerialNumber());
   if (MakeDir(str) == -1) {
      printf("Error: Cannot create directory \"%s\"\n", str);
      return false;
   }
   sprintf(str, "%s/board%d/ResponseCalib_board%d_chip%d_%dMHz.bin", strt, fBoard->GetBoardSerialNumber(),
           fBoard->GetBoardSerialNumber(), chipIndex, static_cast < int >(fBoard->GetNominalFrequency() * 1000));
   fCalibFile = fopen(str, "wb");
   if (fCalibFile == NULL) {
      printf("Error: Cannot write to file \"%s\"\n", str);
      return false;
   }
   // Write File
   for (ii = 0; ii < kNumberOfCalibChannelsV4; ii++) {
      chn = data->fChannel[ii];
      for (j = 0; j < kNumberOfBins; j++) {
         fwrite(&chn->fOffset[j], 2, 1, fCalibFile);
         fwrite(&chn->fGain[j], 2, 1, fCalibFile);
      }
   }
   fclose(fCalibFile);

   printf("Calibration successfully written to\n\"%s\"\n", str);
   return true;
}

/*------------------------------------------------------------------*/

void ResponseCalibration::CalibrationTrigger(int mode, double voltage)
{
   fBoard->Reinit();
   fBoard->EnableAcal(mode, voltage);
   fBoard->StartDomino();
   fBoard->SoftTrigger();
   while (fBoard->IsBusy()) {
   }
}

/*------------------------------------------------------------------*/

void ResponseCalibration::CalibrationStart(double voltage)
{
   fBoard->SetDominoMode(1);
   fBoard->EnableAcal(0, voltage);
   fBoard->StartDomino();
   fBoard->IsBusy();
   fBoard->IsBusy();
   fBoard->IsBusy();
}

/*------------------------------------------------------------------*/

bool ResponseCalibration::RecordCalibrationPoints(int chipNumber)
{
   if (!fInitialized)
      return true;
   if (fBoard->GetDRSType() == 3)
      return RecordCalibrationPointsV4(chipNumber);
   else
      return RecordCalibrationPointsV3(chipNumber);
}

/*------------------------------------------------------------------*/

bool ResponseCalibration::RecordCalibrationPointsV3(int chipNumber)
{
   int j, k, ii;
   int notdone, nsample;
   double voltage;
   float mean;
   const double minVolt = 0.006;
   const double xpos[50] =
       { 0.010, 0.027, 0.052, 0.074, 0.096, 0.117, 0.136, 0.155, 0.173, 0.191, 0.208, 0.226, 0.243, 0.260,
      0.277, 0.294, 0.310,
      0.325, 0.342, 0.358, 0.374, 0.390, 0.406, 0.422, 0.439, 0.457, 0.477, 0.497, 0.520, 0.546, 0.577, 0.611,
      0.656, 0.710,
      0.772, 0.842, 0.916,
      0.995, 1.075, 1.157, 1.240, 1.323, 1.407, 1.490, 1.575, 1.659, 1.744, 1.829, 1.914, 2.000
   };

   // Initialisations
   if (fCurrentLowVoltPoint == 0) {
      fBoard->SetDAC(fBoard->fDAC_CLKOFS, 0);
      // Record Temperature
      fCalibrationData[chipNumber]->fStartTemperature = static_cast < float >(fBoard->GetTemperature());
   }
   // Record current Voltage
   if (fCurrentLowVoltPoint < fNumberOfPointsLowVolt)
      voltage =
          (xpos[0] - minVolt) * fCurrentLowVoltPoint / static_cast <
          double >(fNumberOfPointsLowVolt) + minVolt;
   else
   voltage = xpos[fCurrentPoint];
   fBoard->SetCalibVoltage(voltage);
   fResponseY[fCurrentPoint + fCurrentLowVoltPoint] = static_cast < float >(voltage) * 1000;

   // Loop Over Number Of Samples For Statistics
   for (j = 0; j < fNumberOfSamples; j++) {
      // Read Out Second Part of the Waveform
      CalibrationTrigger(3, voltage);
      fBoard->TransferWaves();
      for (ii = 0; ii < kNumberOfCalibChannelsV3; ii++) {
         fBoard->GetRawWave(chipNumber, ii, fWaveFormMode3[ii][j]);
      }
      // Read Out First Part of the Waveform
      CalibrationStart(voltage);
      CalibrationTrigger(2, voltage);
      fBoard->TransferWaves();
      for (ii = 0; ii < kNumberOfCalibChannelsV3; ii++) {
         fBoard->GetRawWave(chipNumber, ii, fWaveFormMode2[ii][j]);
      }
      CalibrationStart(voltage);
   }
   // Average Sample Points
   for (ii = 0; ii < kNumberOfCalibChannelsV3; ii++) {
      for (k = 0; k < kNumberOfBins; k++) {
         fResponseX[ii][k][fCurrentPoint + fCurrentLowVoltPoint] = 0;
         for (j = 0; j < fNumberOfSamples; j++) {
            fSampleUsed[j] = 1;
            if (k < fNumberOfMode2Bins)
               fSamples[j] = fWaveFormMode2[ii][j][k];
            else
               fSamples[j] = fWaveFormMode3[ii][j][k];
            fResponseX[ii][k][fCurrentPoint + fCurrentLowVoltPoint] += fSamples[j];
         }
         mean = fResponseX[ii][k][fCurrentPoint + fCurrentLowVoltPoint] / fNumberOfSamples;
         notdone = 1;
         nsample = fNumberOfSamples;
         while (notdone) {
            notdone = 0;
            for (j = 0; j < fNumberOfSamples; j++) {
               if (fSampleUsed[j] && abs(static_cast < int >(fSamples[j] - mean)) > 3) {
                  notdone = 1;
                  fSampleUsed[j] = 0;
                  nsample--;
                  fResponseX[ii][k][fCurrentPoint + fCurrentLowVoltPoint] -= fSamples[j];
                  mean = fResponseX[ii][k][fCurrentPoint + fCurrentLowVoltPoint] / nsample;
               }
            }
         }
         fResponseX[ii][k][fCurrentPoint + fCurrentLowVoltPoint] = mean;
      }
   }
   if (fCurrentLowVoltPoint < fNumberOfPointsLowVolt)
      fCurrentLowVoltPoint++;
   else
      fCurrentPoint++;

   if (fCurrentPoint == fNumberOfPoints) {
      fCalibrationData[chipNumber]->fEndTemperature = static_cast < float >(fBoard->GetTemperature());
      fRecorded = true;
      fFitted = false;
      fOffset = false;
      fCalibrationData[chipNumber]->fRead = false;
      fCalibrationData[chipNumber]->fHasOffsetCalibration = false;
      fBoard->SetCalibVoltage(0.0);
      fBoard->EnableAcal(1, 0.0);
      fBoard->SetDAC(fBoard->fDAC_CLKOFS, 0.0);
      return true;
   }

   return false;
}

/*------------------------------------------------------------------*/

bool ResponseCalibration::RecordCalibrationPointsV4(int chipNumber)
{
   int i, j, k, n;
   double voltage, s, s2, average;

   if (fCurrentPoint == 0) {
      fBoard->SetDominoMode(1);
      fBoard->EnableAcal(1, 0);
      fBoard->SoftTrigger();
      while (fBoard->IsBusy());
      fBoard->StartDomino();
      fCalibrationData[chipNumber]->fStartTemperature = static_cast < float >(fBoard->GetTemperature());
   }
   voltage = 1.0 * fCurrentPoint / (static_cast < double >(fNumberOfPoints) - 1) +0.1;
   fBoard->SetCalibVoltage(voltage);
   Sleep(10);
   fBoard->SetCalibVoltage(voltage);
   Sleep(10);

   // One dummy cycle for unknown reasons
   fBoard->SoftTrigger();
   while (fBoard->IsBusy());
   fBoard->StartDomino();
   Sleep(50);
   fBoard->TransferWaves();

   // Loop over number of samples for statistics
   for (i = 0; i < fNumberOfSamples; i++) {
      if (fBoard->Debug()) {
         printf("%02d:%02d\r", fNumberOfPoints - fCurrentPoint, fNumberOfSamples - i);
         fflush(stdout);
      }


      fBoard->SoftTrigger();
      while (fBoard->IsBusy());
      fBoard->StartDomino();
      Sleep(50);
      fBoard->TransferWaves();
      for (j = 0; j < kNumberOfCalibChannelsV4; j++) {
         fBoard->GetRawWave(chipNumber, j, fWaveFormMode3[j][i]);
      }
   }

   // Calculate averages
   for (i = 0; i < kNumberOfCalibChannelsV4; i++) {
      for (k = 0; k < kNumberOfBins; k++) {
         s = s2 = 0;

         for (j = 0; j < fNumberOfSamples; j++) {
            s += fWaveFormMode3[i][j][k];
            s2 += fWaveFormMode3[i][j][k] * fWaveFormMode3[i][j][k];
         }
         n = fNumberOfSamples;
         average = s / n;

         fResponseX[i][k][fCurrentPoint] = static_cast < float >(average);
      }
   }

   fCurrentPoint++;
   if (fCurrentPoint == fNumberOfPoints) {
      fCalibrationData[chipNumber]->fEndTemperature = static_cast < float >(fBoard->GetTemperature());
      fRecorded = true;
      return true;
   }

   return false;
}

/*------------------------------------------------------------------*/

bool ResponseCalibration::FitCalibrationPoints(int chipNumber)
{
   if (!fRecorded || fFitted)
      return true;
   if (fBoard->GetDRSType() == 3)
      return FitCalibrationPointsV4(chipNumber);
   else
      return FitCalibrationPointsV3(chipNumber);
}

/*------------------------------------------------------------------*/

bool ResponseCalibration::FitCalibrationPointsV3(int chipNumber)
{
   int i, j, k;
   float x1, x2, y1, y2;
   float uu;
   float yc, yr;
   float xminExt, xrangeExt;
   float xmin, xrange;
   float average, averageError, averageExt, averageErrorExt;
   unsigned short i0, i1;

   CalibrationData *data = fCalibrationData[chipNumber];
   CalibrationData::CalibrationDataChannel * chn = data->fChannel[fCurrentFitChannel];

   data->DeletePreCalculatedBSpline();

   if (fCurrentFitBin == 0 && fCurrentFitChannel == 0) {
      data->fNumberOfLimitGroups = 0;
      data->fMin = 100000;
      data->fMax = -100000;
      for (i = 0; i < kNumberOfCalibChannelsV3; i++) {
         for (j = 0; j < kNumberOfBins; j++) {
            if (data->fMin > fResponseX[i][j][fNumberOfPointsLowVolt + fNumberOfPoints - 1])
               data->fMin = fResponseX[i][j][fNumberOfPointsLowVolt + fNumberOfPoints - 1];
            if (data->fMax < fResponseX[i][j][fNumberOfPointsLowVolt])
               data->fMax = fResponseX[i][j][fNumberOfPointsLowVolt];
         }
      }
   }
   // Low Volt
   i0 = static_cast < unsigned short >(fResponseX[fCurrentFitChannel][fCurrentFitBin][0]);
   i1 = static_cast <
       unsigned short >(fResponseX[fCurrentFitChannel][fCurrentFitBin][fNumberOfPointsLowVolt]) + 1;
   chn->fLookUpOffset[fCurrentFitBin] = i0;
   delete chn->fLookUp[fCurrentFitBin];
   if (i0 - i1 + 1 < 2) {
      chn->fNumberOfLookUpPoints[fCurrentFitBin] = 2;
      chn->fLookUp[fCurrentFitBin] = new unsigned char[2];
      chn->fLookUp[fCurrentFitBin][0] = 0;
      chn->fLookUp[fCurrentFitBin][1] = 0;
   } else {
      chn->fNumberOfLookUpPoints[fCurrentFitBin] = i0 - i1 + 1;
      chn->fLookUp[fCurrentFitBin] = new unsigned char[i0 - i1 + 1];
      for (i = 0; i < i0 - i1 + 1; i++) {
         for (j = 0; j < fNumberOfPointsLowVolt; j++) {
            if (i0 - i >= fResponseX[fCurrentFitChannel][fCurrentFitBin][j + 1]) {
               x1 = fResponseX[fCurrentFitChannel][fCurrentFitBin][j];
               x2 = fResponseX[fCurrentFitChannel][fCurrentFitBin][j + 1];
               y1 = fResponseY[j];
               y2 = fResponseY[j + 1];
               chn->fLookUp[fCurrentFitBin][i] =
                   static_cast < unsigned char >(((y2 - y1) * (i0 - i - x1) / (x2 - x1) + y1) / fPrecision);
               break;
            }
         }
      }
   }

   // Copy Points
   for (i = 0; i < fNumberOfPoints; i++) {
      fPntX[0][i] = fResponseX[fCurrentFitChannel][fCurrentFitBin][fNumberOfPointsLowVolt + i];
      fPntY[0][i] = fResponseY[fNumberOfPointsLowVolt + i];
   }
   // Fit BSpline
   for (i = 0; i < fNumberOfPoints; i++) {
      fUValues[0][i] = static_cast < float >(1 - i / (fNumberOfPoints - 1.));
   }
   if (!Approx(fPntX[0], fUValues[0], fNumberOfPoints, fNumberOfGridPoints, fResX[fCurrentFitBin]))
      return true;
   if (!Approx(fPntY[0], fUValues[0], fNumberOfPoints, fNumberOfGridPoints, fRes[fCurrentFitBin]))
      return true;

   // X constant fit
   for (k = 0; k < fNumberOfXConstPoints - 2; k++) {
      fPntX[1][k + 1] =
          GetValue(fResX[fCurrentFitBin],
                   static_cast < float >(1 - k / static_cast < float >(fNumberOfXConstPoints - 3)),
                   fNumberOfGridPoints);
      fPntY[1][k + 1] =
          GetValue(fRes[fCurrentFitBin],
                   static_cast < float >(1 - k / static_cast < float >(fNumberOfXConstPoints - 3)),
                   fNumberOfGridPoints);
   }
   xmin = fPntX[1][fNumberOfXConstPoints - 2];
   xrange = fPntX[1][1] - xmin;

   for (i = 0; i < fNumberOfXConstPoints - 2; i++) {
      fUValues[1][i + 1] = (fPntX[1][i + 1] - xmin) / xrange;
   }

   if (!Approx
       (&fPntY[1][1], &fUValues[1][1], fNumberOfXConstPoints - 2, fNumberOfXConstGridPoints, chn->fTempData))
      return true;

   // error statistics
   if (fShowStatistics) {
      for (i = 0; i < fNumberOfPoints; i++) {
         uu = (fPntX[0][i] - xmin) / xrange;
         yc = GetValue(chn->fTempData, uu, fNumberOfXConstGridPoints);
         yr = fPntY[0][i];
         fStatisticsApprox[i][fCurrentFitBin + fCurrentFitChannel * kNumberOfBins] = yc - yr;
      }
   }
   // Add min and max point
   chn->fLimitGroup[fCurrentFitBin] = 0;
   while (xmin - kBSplineXMinOffset > data->fMin + kBSplineXMinOffset * chn->fLimitGroup[fCurrentFitBin]) {
      chn->fLimitGroup[fCurrentFitBin]++;
   }
   if (data->fNumberOfLimitGroups <= chn->fLimitGroup[fCurrentFitBin])
      data->fNumberOfLimitGroups = chn->fLimitGroup[fCurrentFitBin] + 1;
   xminExt = data->fMin + kBSplineXMinOffset * chn->fLimitGroup[fCurrentFitBin];
   xrangeExt = data->fMax - xminExt;

   fPntX[1][0] = data->fMax;
   uu = (fPntX[1][0] - xmin) / xrange;
   fPntY[1][0] = GetValue(chn->fTempData, uu, fNumberOfXConstGridPoints);

   fPntX[1][fNumberOfXConstPoints - 1] = xminExt;
   uu = (fPntX[1][fNumberOfXConstPoints - 1] - xmin) / xrange;
   fPntY[1][fNumberOfXConstPoints - 1] = GetValue(chn->fTempData, uu, fNumberOfXConstGridPoints);

   for (i = 0; i < fNumberOfXConstPoints; i++) {
      fUValues[1][i] = (fPntX[1][i] - xminExt) / xrangeExt;
   }

   if (!Approx(fPntY[1], fUValues[1], fNumberOfXConstPoints, fNumberOfXConstGridPoints, chn->fTempData))
      return true;

   // error statistics
   if (fShowStatistics) {
      for (i = 0; i < fNumberOfPoints; i++) {
         uu = (fPntX[0][i] - xminExt) / xrangeExt;
         yc = GetValue(chn->fTempData, uu, fNumberOfXConstGridPoints);
         yr = fPntY[0][i];
         fStatisticsApproxExt[i][fCurrentFitBin + fCurrentFitChannel * kNumberOfBins] = yc - yr;
      }
   }
   for (i = 0; i < fNumberOfXConstGridPoints; i++) {
      chn->fData[fCurrentFitBin][i] = static_cast < short >(chn->fTempData[i] / fPrecision);
   }

   // write end of file
   fCurrentFitBin++;
   if (fCurrentFitBin == kNumberOfBins) {
      fCurrentFitChannel++;
      fCurrentFitBin = 0;
   }
   if (fCurrentFitChannel == kNumberOfCalibChannelsV3) {
      if (fShowStatistics) {
         for (i = 0; i < fNumberOfPoints; i++) {
            average = 0;
            averageError = 0;
            averageExt = 0;
            averageErrorExt = 0;
            for (j = 0; j < kNumberOfCalibChannelsV3 * kNumberOfBins; j++) {
               average += fStatisticsApprox[i][j];
               averageError += fStatisticsApprox[i][j] * fStatisticsApprox[i][j];
               averageExt += fStatisticsApproxExt[i][j];
               averageErrorExt += fStatisticsApproxExt[i][j] * fStatisticsApproxExt[i][j];
            }
            average /= kNumberOfCalibChannelsV3 * kNumberOfBins;
            averageError =
                sqrt((averageError -
                      average * average / kNumberOfCalibChannelsV3 * kNumberOfBins) /
                     (kNumberOfCalibChannelsV3 * kNumberOfBins - 1));
            averageExt /= kNumberOfCalibChannelsV3 * kNumberOfBins;
            averageErrorExt =
                sqrt((averageErrorExt -
                      averageExt * averageExt / kNumberOfCalibChannelsV3 * kNumberOfBins) /
                     (kNumberOfCalibChannelsV3 * kNumberOfBins - 1));
            printf("Error at %3.1f V : % 2.3f +- % 2.3f ; % 2.3f +- % 2.3f\n", fPntY[0][i], average,
                   averageError, averageExt, averageErrorExt);
         }
      }
      fFitted = true;
      fOffset = false;
      fCalibrationData[chipNumber]->fRead = true;
      fCalibrationData[chipNumber]->fHasOffsetCalibration = false;
      data->PreCalculateBSpline();
      return true;
   }
   return false;
}

/*------------------------------------------------------------------*/

bool ResponseCalibration::FitCalibrationPointsV4(int chipNumber)
{
   if (!fRecorded || fFitted)
      return true;
   int i;
   double par[2];
   static int error;

   CalibrationData *data = fCalibrationData[chipNumber];
   CalibrationData::CalibrationDataChannel * chn = data->fChannel[fCurrentFitChannel];

   if (fCurrentFitBin == 0 && fCurrentFitChannel == 0) {
      error = 0;
      for (i = 0; i < fNumberOfPoints; i++)
         fWWFit[i] = 1;
   }

   for (i = 0; i < fNumberOfPoints; i++) {
      fXXFit[i] = 1.0 * i / (static_cast < double >(fNumberOfPoints) - 1) +0.1;
      fYYFit[i] = fResponseX[fCurrentFitChannel][fCurrentFitBin][i];
      if (fCurrentFitBin == 10 && fCurrentFitChannel == 1) {
         fXXSave[i] = fXXFit[i];
         fYYSave[i] = fYYFit[i];
      }
   }

   // DRSBoard::LinearRegression(fXXFit, fYYFit, fNumberOfPoints, &par[1], &par[0]);
   // exclude first two points (sometimes are on limit of FADC)
   DRSBoard::LinearRegression(fXXFit + 2, fYYFit + 2, fNumberOfPoints - 2, &par[1], &par[0]);

   chn->fOffset[fCurrentFitBin] = static_cast < unsigned short >(par[0] + 0.5);
   chn->fGain[fCurrentFitBin] = static_cast < unsigned short >(par[1] + 0.5);

   // Remember min/max of gain
   if (fCurrentFitBin == 0 && fCurrentFitChannel == 0)
      fGainMin = fGainMax = chn->fGain[0];
   if (chn->fGain[fCurrentFitBin] < fGainMin)
      fGainMin = chn->fGain[fCurrentFitBin];
   if (chn->fGain[fCurrentFitBin] > fGainMax)
      fGainMax = chn->fGain[fCurrentFitBin];

   // abort if outside normal region
   if (chn->fGain[fCurrentFitBin] / 4096.0 < 0.8 || chn->fGain[fCurrentFitBin] / 4096.0 > 1) {
      error++;

      if (error < 20)
         printf("Gain=%1.3lf for bin %d on channel %d on chip %d outside valid region\n",
                chn->fGain[fCurrentFitBin] / 4096.0, fCurrentFitBin, fCurrentFitChannel, chipNumber);
   }

   if (fCurrentFitChannel == 1 && fCurrentFitBin == 10) {
      for (i = 0; i < fNumberOfPoints; i++) {
         fXXSave[i] = fXXFit[i];
         fYYSave[i] = (fYYFit[i] - chn->fOffset[10]) / chn->fGain[10] - fXXFit[i];
      }
   }

   fCurrentFitBin++;
   if (fCurrentFitBin == kNumberOfBins) {
      fCurrentFitChannel++;
      fCurrentFitBin = 0;
   }
   if (fCurrentFitChannel == kNumberOfCalibChannelsV4) {

      if (fBoard->Debug()) {
         printf("Gain min=%1.3lf max=%1.3lf\n", fGainMin / 4096.0, fGainMax / 4096.0);
         fflush(stdout);
      }
      // allow up to three bad bins
      if (error > 3) {
         printf("Aborting calibration!\n");
         return true;
      }

      fFitted = true;
      fOffset = false;
      fCalibrationData[chipNumber]->fRead = true;
      fCalibrationData[chipNumber]->fHasOffsetCalibration = false;
      return true;
   }

   return false;
}

unsigned int millitime()
{
#ifdef _MSC_VER

   return (int) GetTickCount();

#else
   struct timeval tv;

   gettimeofday(&tv, NULL);

   return tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif
   return 0;
}

/*------------------------------------------------------------------*/

bool ResponseCalibration::OffsetCalibration(int chipNumber)
{
   if (!fFitted || fOffset)
      return true;
   if (fBoard->GetDRSType() == 3)
      return OffsetCalibrationV4(chipNumber);
   else
      return OffsetCalibrationV3(chipNumber);
}

/*------------------------------------------------------------------*/

bool ResponseCalibration::OffsetCalibrationV3(int chipNumber)
{
   int k, ii, j;
   int t1, t2;
   float mean, error;
   CalibrationData *data = fCalibrationData[chipNumber];
   CalibrationData::CalibrationDataChannel * chn;

   if (fCurrentSample == 0) {
      data->fHasOffsetCalibration = false;
      fBoard->SetCalibVoltage(0.0);
      fBoard->EnableAcal(0, 0.0);
   }
   // Loop Over Number Of Samples For Statistics
   t1 = millitime();
   fBoard->SoftTrigger();
   while (fBoard->IsBusy()) {
   }
   fBoard->TransferWaves();
   for (ii = 0; ii < kNumberOfCalibChannelsV3; ii++) {
      fBoard->GetRawWave(chipNumber, ii, fWaveFormOffsetADC[ii][fCurrentSample]);
      fBoard->CalibrateWaveform(chipNumber, ii, fWaveFormOffsetADC[ii][fCurrentSample],
                                fWaveFormOffset[ii][fCurrentSample], true, false, false, 0, true);
   }
   fBoard->StartDomino();
   fBoard->IsBusy();
   fBoard->IsBusy();
   fBoard->IsBusy();
   t2 = millitime();
   while (t2 - t1 < (1000 / fTriggerFrequency)) {
      t2 = millitime();
   }
   fCurrentSample++;

   if (fCurrentSample == fNumberOfSamples) {
      // Average Sample Points
      float *sample = new float[fNumberOfSamples];
      for (ii = 0; ii < kNumberOfCalibChannelsV3; ii++) {
         chn = data->fChannel[ii];
         for (k = 0; k < kNumberOfBins; k++) {
            for (j = 0; j < fNumberOfSamples; j++)
               sample[j] = static_cast < float >(fWaveFormOffset[ii][j][k]);
            Average(1, sample, fNumberOfSamples, mean, error, 2);
            chn->fOffset[k] = static_cast < short >(mean);
            for (j = 0; j < fNumberOfSamples; j++)
               sample[j] = fWaveFormOffsetADC[ii][j][k];
            Average(1, sample, fNumberOfSamples, mean, error, 2);
            chn->fOffsetADC[k] = static_cast < unsigned short >(mean);
         }
      }
      fOffset = true;
      fCalibrationData[chipNumber]->fHasOffsetCalibration = true;
      delete[] sample;
      return true;
   }

   return false;
}

/*------------------------------------------------------------------*/

bool ResponseCalibration::OffsetCalibrationV4(int chipNumber)
{
   int k, ii, j;
   float mean, error;
   CalibrationData *data = fCalibrationData[chipNumber];
   CalibrationData::CalibrationDataChannel * chn;

   /* switch DRS to input, hope that no real signal occurs */
   if (fCurrentSample == 0) {
      data->fHasOffsetCalibration = false;
      fBoard->SetCalibVoltage(0.0);
      fBoard->EnableAcal(0, 0.0);
      /* one dummy trigger for unknown reasons */
      fBoard->SoftTrigger();
      while (fBoard->IsBusy());
      fBoard->StartDomino();
      Sleep(50);
   }
   // Loop Over Number Of Samples For Statistics
   fBoard->SoftTrigger();
   while (fBoard->IsBusy());
   fBoard->TransferWaves();
   for (ii = 0; ii < kNumberOfCalibChannelsV4; ii++)
      fBoard->GetRawWave(chipNumber, ii, fWaveFormOffsetADC[ii][fCurrentSample]);

   fBoard->StartDomino();
   Sleep(50);
   fCurrentSample++;

   if (fBoard->Debug()) {
      printf("%02d\r", fNumberOfSamples - fCurrentSample);
      fflush(stdout);
   }

   if (fCurrentSample == fNumberOfSamples) {
      // Average Sample Points
      float *sample = new float[fNumberOfSamples];
      for (ii = 0; ii < kNumberOfCalibChannelsV3; ii++) {
         chn = data->fChannel[ii];
         for (k = 0; k < kNumberOfBins; k++) {
            for (j = 0; j < fNumberOfSamples; j++)
               sample[j] = static_cast < float >(fWaveFormOffsetADC[ii][j][k]);
            Average(1, sample, fNumberOfSamples, mean, error, 2);
            chn->fOffset[k] = static_cast < unsigned short >(mean);
         }
      }
      fOffset = true;
      fCalibrationData[chipNumber]->fHasOffsetCalibration = true;
      delete[] sample;
      return true;
   }

   return false;
}

/*------------------------------------------------------------------*/

void ResponseCalibration::InitFields(int numberOfPointsLowVolt, int numberOfPoints, int numberOfMode2Bins,
                                     int numberOfSamples, int numberOfGridPoints, int numberOfXConstPoints,
                                     int numberOfXConstGridPoints, double triggerFrequency,
                                     int showStatistics)
{
   int ii, j, i;
   fInitialized = true;
   fNumberOfPointsLowVolt = numberOfPointsLowVolt;
   fNumberOfPoints = numberOfPoints;
   fNumberOfMode2Bins = numberOfMode2Bins;
   fNumberOfSamples = numberOfSamples;
   fNumberOfGridPoints = numberOfGridPoints;
   fNumberOfXConstPoints = numberOfXConstPoints;
   fNumberOfXConstGridPoints = numberOfXConstGridPoints;
   fTriggerFrequency = triggerFrequency;
   fShowStatistics = showStatistics;
   fCurrentPoint = 0;
   fCurrentSample = 0;
   fCurrentFitChannel = 0;
   fCurrentFitBin = 0;
   for (ii = 0; ii < kNumberOfCalibChannelsV3; ii++) {
      for (j = 0; j < kNumberOfBins; j++) {
         fResponseX[ii][j] = new float[fNumberOfPoints + fNumberOfPointsLowVolt];
      }
   }
   fResponseY = new float[fNumberOfPoints + fNumberOfPointsLowVolt];
   for (ii = 0; ii < kNumberOfCalibChannelsV3; ii++) {
      fWaveFormMode3[ii] = new unsigned short *[fNumberOfSamples];
      fWaveFormMode2[ii] = new unsigned short *[fNumberOfSamples];
      fWaveFormOffset[ii] = new short *[fNumberOfSamples];
      fWaveFormOffsetADC[ii] = new unsigned short *[fNumberOfSamples];
      for (i = 0; i < fNumberOfSamples; i++) {
         fWaveFormMode3[ii][i] = new unsigned short[kNumberOfBins];
         fWaveFormMode2[ii][i] = new unsigned short[kNumberOfBins];
         fWaveFormOffset[ii][i] = new short[kNumberOfBins];
         fWaveFormOffsetADC[ii][i] = new unsigned short[kNumberOfBins];
      }
   }
   fSamples = new unsigned short[fNumberOfSamples];
   fSampleUsed = new int[fNumberOfSamples];

   for (j = 0; j < kNumberOfBins; j++) {
      fRes[j] = new float[fNumberOfGridPoints];
      fResX[j] = new float[fNumberOfGridPoints];
   }
   for (i = 0; i < 2; i++) {
      fPntX[i] = new float[fNumberOfPoints * (1 - i) + fNumberOfXConstPoints * i];
      fPntY[i] = new float[fNumberOfPoints * (1 - i) + fNumberOfXConstPoints * i];
      fUValues[i] = new float[fNumberOfPoints * (1 - i) + fNumberOfXConstPoints * i];
   }
   fXXFit = new double[fNumberOfPoints];
   fYYFit = new double[fNumberOfPoints];
   fWWFit = new double[fNumberOfPoints];
   fYYFitRes = new double[fNumberOfPoints];
   fYYSave = new double[fNumberOfPoints];
   fXXSave = new double[fNumberOfPoints];

   fStatisticsApprox = new float *[fNumberOfPoints];
   fStatisticsApproxExt = new float *[fNumberOfPoints];
   for (i = 0; i < fNumberOfPoints; i++) {
      fStatisticsApprox[i] = new float[kNumberOfCalibChannelsV3 * kNumberOfBins];
      fStatisticsApproxExt[i] = new float[kNumberOfCalibChannelsV3 * kNumberOfBins];
   }
   for (i = 0; i < kNumberOfChipsMax; i++) {
      fCalibrationData[i] = new CalibrationData(numberOfXConstGridPoints);
   }
}

/*------------------------------------------------------------------*/

void ResponseCalibration::DeleteFields()
{
   if (!fInitialized)
      return;
   fInitialized = false;
   int ii, j, i;
   for (ii = 0; ii < kNumberOfCalibChannelsV3; ii++) {
      for (j = 0; j < kNumberOfBins; j++) {
         delete fResponseX[ii][j];
      }
   }
   delete fResponseY;
   for (ii = 0; ii < kNumberOfCalibChannelsV3; ii++) {
      for (i = 0; i < fNumberOfSamples; i++) {
         if (fWaveFormMode3[ii] != NULL)
            delete fWaveFormMode3[ii][i];
         if (fWaveFormMode2[ii] != NULL)
            delete fWaveFormMode2[ii][i];
         if (fWaveFormOffset[ii] != NULL)
            delete fWaveFormOffset[ii][i];
         if (fWaveFormOffsetADC[ii] != NULL)
            delete fWaveFormOffsetADC[ii][i];
      }
      delete fWaveFormMode3[ii];
      delete fWaveFormMode2[ii];
      delete fWaveFormOffset[ii];
      delete fWaveFormOffsetADC[ii];
   }
   delete fSamples;
   delete fSampleUsed;

   for (j = 0; j < kNumberOfBins; j++) {
      delete fRes[j];
      delete fResX[j];
   }
   for (i = 0; i < 2; i++) {
      delete fPntX[i];
      delete fPntY[i];
      delete fUValues[i];
   }
   delete fXXFit;
   delete fYYFit;
   delete fWWFit;
   delete fYYFitRes;
   delete fYYSave;
   delete fXXSave;

   for (i = 0; i < fNumberOfPoints; i++) {
      delete fStatisticsApprox[i];
      delete fStatisticsApproxExt[i];
   }
   delete fStatisticsApprox;
   delete fStatisticsApproxExt;
   for (i = 0; i < kNumberOfChipsMax; i++)
      delete fCalibrationData[i];
}

/*------------------------------------------------------------------*/

double ResponseCalibration::GetTemperature(unsigned int chipIndex)
{
   if (fCalibrationData[chipIndex] == NULL)
      return 0;
   if (!fCalibrationData[chipIndex]->fRead)
      return 0;
   return (fCalibrationData[chipIndex]->fStartTemperature + fCalibrationData[chipIndex]->fEndTemperature) / 2;
}

/*------------------------------------------------------------------*/

bool ResponseCalibration::Calibrate(unsigned int chipIndex, unsigned int channel, unsigned short *adcWaveform,
                                    short *uWaveform, int triggerCell, float threshold, bool offsetCalib)
{
   int i;
   unsigned int NumberOfCalibChannels;
   int hasOffset;
   bool aboveThreshold;
   float wave, v;
   int j, irot;

   CalibrationData *data = fCalibrationData[chipIndex];
   CalibrationData::CalibrationDataChannel * chn;

   if (fBoard->GetDRSType() == 3)
      NumberOfCalibChannels = kNumberOfCalibChannelsV4;
   else
      NumberOfCalibChannels = kNumberOfCalibChannelsV3;

   if (channel >= NumberOfCalibChannels || data == NULL) {
      for (i = 0; i < kNumberOfBins; i++) {
         irot = i;
         if (triggerCell > -1)
            irot = (triggerCell + i) % kNumberOfBins;

         uWaveform[i] = adcWaveform[irot];
      }
      return true;
   }
   if (!data->fRead) {
      for (i = 0; i < kNumberOfBins; i++) {
         uWaveform[i] = adcWaveform[i];
      }
      return true;
   }

   chn = data->fChannel[channel];

   hasOffset = data->fHasOffsetCalibration;
   aboveThreshold = (threshold == 0);   // if threshold equal zero, always return true

   short offset;

   // Calibrate
   for (i = 0; i < kNumberOfBins; i++) {
      if (fBoard->GetDRSType() != 3) {
         irot = i;
         if (triggerCell > -1)
            irot = (triggerCell + i) % kNumberOfBins;
         offset = offsetCalib ? chn->fOffset[irot] : 0;
         if (adcWaveform[irot] > chn->fLookUpOffset[irot]) {
            uWaveform[i] =
                ((chn->fLookUp[irot][0] - chn->fLookUp[irot][1]) * (adcWaveform[irot] -
                                                                    chn->fLookUpOffset[irot]) +
                 chn->fLookUp[irot][0]);
         } else if (adcWaveform[irot] <= chn->fLookUpOffset[irot]
                    && adcWaveform[irot] > chn->fLookUpOffset[irot] - chn->fNumberOfLookUpPoints[irot]) {
            uWaveform[i] = chn->fLookUp[irot][chn->fLookUpOffset[irot] - adcWaveform[irot]];
         } else {
            wave = 0;
            for (j = 0; j < kBSplineOrder; j++) {
               wave +=
                   chn->fData[irot][data->fBSplineOffsetLookUp[adcWaveform[irot]][chn->fLimitGroup[irot]] + j]
                   * data->fBSplineLookUp[adcWaveform[irot]][chn->fLimitGroup[irot]][j];
            }
            uWaveform[i] = static_cast < short >(wave);
         }
         // Offset Calibration
         if (hasOffset)
            uWaveform[i] -= offset;
      } else {
         irot = i;
         if (triggerCell > -1)
            irot = (triggerCell + i) % kNumberOfBins;
#if 0                           /* not enabled yet for DRS3 */
         offset = offsetCalib ? chn->fOffset[irot] : 0;
#else
         offset = chn->fOffset[irot];
#endif
         v = static_cast < float >(adcWaveform[irot] - offset) / chn->fGain[irot];
         uWaveform[i] = static_cast < short >(v * 1000 / GetPrecision() + 0.5);
      }

      // Check for Threshold
      if (!aboveThreshold) {
         if (uWaveform[i] >= threshold)
            aboveThreshold = true;
      }
   }
   return aboveThreshold;
}

/*------------------------------------------------------------------*/

bool ResponseCalibration::SubtractADCOffset(unsigned int chipIndex, unsigned int channel,
                                            unsigned short *adcWaveform,
                                            unsigned short *adcCalibratedWaveform,
                                            unsigned short newBaseLevel)
{
   int i;
   unsigned int NumberOfCalibChannels;
   CalibrationData *data = fCalibrationData[chipIndex];
   CalibrationData::CalibrationDataChannel * chn;

   if (fBoard->GetDRSType() == 3)
      NumberOfCalibChannels = kNumberOfCalibChannelsV4;
   else
      NumberOfCalibChannels = kNumberOfCalibChannelsV3;

   if (channel >= NumberOfCalibChannels || data == NULL)
      return false;
   if (!data->fRead || !data->fHasOffsetCalibration)
      return false;

   chn = data->fChannel[channel];
   for (i = 0; i < kNumberOfBins; i++)
      adcCalibratedWaveform[i] = adcWaveform[i] - chn->fOffsetADC[i] + newBaseLevel;
   return true;
}


/*------------------------------------------------------------------*/

bool ResponseCalibration::ReadCalibration(unsigned int chipIndex)
{
   if (fBoard->GetDRSType() == 3)
      return ReadCalibrationV4(chipIndex);
   else
      return ReadCalibrationV3(chipIndex);
}

/*------------------------------------------------------------------*/

bool ResponseCalibration::ReadCalibrationV3(unsigned int chipIndex)
{
   int k, l, m, num;
   unsigned char ng;
   short tempShort;
   char fileName[2000];
   FILE *fileHandle;
   char calibDir[1000];

   // Read Response Calibration
   delete fCalibrationData[chipIndex];
   fCalibrationData[chipIndex] = NULL;

   fBoard->GetCalibrationDirectory(calibDir);
   sprintf(fileName, "%s/board%d/ResponseCalib_board%d_chip%d_%dMHz.bin", calibDir,
           fBoard->GetBoardSerialNumber(), fBoard->GetBoardSerialNumber(), chipIndex,
           static_cast < int >(fBoard->GetNominalFrequency() * 1000));

   fileHandle = fopen(fileName, "rb");
   if (fileHandle == NULL) {
      printf("Board %d --> Could not find response calibration file:\n", fBoard->GetBoardSerialNumber());
      printf("%s\n", fileName);
      return false;
   }
   // Number Of Grid Points
   num = fread(&ng, 1, 1, fileHandle);
   if (num != 1) {
      printf("Error while reading response calibration file '%s'\n", fileName);
      printf("   at 'NumberOfGridPoints'.\n");
      return false;
   }

   fCalibrationData[chipIndex] = new CalibrationData(ng);
   CalibrationData *data = fCalibrationData[chipIndex];
   CalibrationData::CalibrationDataChannel * chn;
   data->fRead = true;
   data->fHasOffsetCalibration = 1;
   data->DeletePreCalculatedBSpline();
   fCalibrationValid[chipIndex] = true;

   // Start Temperature
   num = fread(&tempShort, 2, 1, fileHandle);
   if (num != 1) {
      printf("Error while reading response calibration file '%s'\n", fileName);
      printf("   at 'StartTemperature'.\n");
      return false;
   }
   data->fStartTemperature = static_cast < float >(tempShort) / 10;
   // End Temperature
   num = fread(&tempShort, 2, 1, fileHandle);
   if (num != 1) {
      printf("Error while reading response calibration file '%s'\n", fileName);
      printf("   at 'EndTemperature'.\n");
      return false;
   }
   data->fEndTemperature = static_cast < float >(tempShort) / 10;
   if (fBoard->GetDRSType() != 3) {
      // Min
      num = fread(&data->fMin, 4, 1, fileHandle);
      if (num != 1) {
         printf("Error while reading response calibration file '%s'\n", fileName);
         printf("   at 'Min'.\n");
         return false;
      }
      // Max
      num = fread(&data->fMax, 4, 1, fileHandle);
      if (num != 1) {
         printf("Error while reading response calibration file '%s'\n", fileName);
         printf("   at 'Max'.\n");
         return false;
      }
      // Number Of Limit Groups
      num = fread(&data->fNumberOfLimitGroups, 1, 1, fileHandle);
      if (num != 1) {
         printf("Error while reading response calibration file '%s'\n", fileName);
         printf("   at 'NumberOfLimitGroups'.\n");
         return false;
      }
   }
   // read channel
   for (k = 0; k < kNumberOfCalibChannelsV3; k++) {
      chn = data->fChannel[k];
      for (l = 0; l < kNumberOfBins; l++) {
         if (fBoard->GetDRSType() != 3) {
            // Range Group
            num = fread(&chn->fLimitGroup[l], 1, 1, fileHandle);
            if (num != 1) {
               printf("Error while reading response calibration file '%s'\n", fileName);
               printf("   at 'RangeGroup' of channel %d bin %d.\n", k, l);
               return false;
            }
            // Look Up Offset
            num = fread(&chn->fLookUpOffset[l], 2, 1, fileHandle);
            if (num != 1) {
               printf("Error while reading response calibration file '%s'\n", fileName);
               printf("   at 'LookUpOffset' of channel %d bin %d.\n", k, l);
               return false;
            }
            // Number Of Look Up Points
            num = fread(&chn->fNumberOfLookUpPoints[l], 1, 1, fileHandle);
            if (num != 1) {
               printf("Error while reading response calibration file '%s'\n", fileName);
               printf("   at 'NumberOfLookUpPoints' of channel %d bin %d.\n", k, l);
               return false;
            }
            // Look Up Points
            delete chn->fLookUp[l];
            chn->fLookUp[l] = new unsigned char[chn->fNumberOfLookUpPoints[l]];
            for (m = 0; m < chn->fNumberOfLookUpPoints[l]; m++) {
               num = fread(&chn->fLookUp[l][m], 1, 1, fileHandle);
               if (num != 1) {
                  printf("Error while reading response calibration file '%s'\n", fileName);
                  printf("   at 'LookUp %d' of channel %d bin %d.\n", m, k, l);
                  return false;
               }
            }
            // Points
            for (m = 0; m < data->fNumberOfGridPoints; m++) {
               num = fread(&chn->fData[l][m], 2, 1, fileHandle);
               if (num != 1) {
                  printf("Error while reading response calibration file '%s'\n", fileName);
                  printf("   at 'Point %d' of channel %d bin %d.\n", m, k, l);
                  return false;
               }
            }
            // ADC Offset
            num = fread(&chn->fOffsetADC[l], 2, 1, fileHandle);
            if (num != 1) {
               printf("Error while reading response calibration file '%s'\n", fileName);
               printf("   at 'ADC Offset' of channel %d bin %d.\n", k, l);
               return false;
            }
         }
         // Offset
         num = fread(&chn->fOffset[l], 2, 1, fileHandle);
         if (num != 1) {
            printf("Error while reading response calibration file '%s'\n", fileName);
            printf("   at 'Offset' of channel %d bin %d.\n", k, l);
            return false;
         }
         if (fBoard->GetDRSType() == 3) {
            // Gain
            num = fread(&chn->fGain[l], 2, 1, fileHandle);
            if (num != 1) {
               printf("Error while reading response calibration file '%s'\n", fileName);
               printf("   at 'Gain' of channel %d bin %d.\n", k, l);
               return false;
            }
         }
      }
   }
   fclose(fileHandle);

   if (fBoard->GetDRSType() != 3) {
      data->PreCalculateBSpline();
   }

   return true;
}

/*------------------------------------------------------------------*/

bool ResponseCalibration::ReadCalibrationV4(unsigned int chipIndex)
{
   int k, l, num;
   char fileName[2000];
   FILE *fileHandle;
   char calibDir[1000];

   // Read Response Calibration

   fBoard->GetCalibrationDirectory(calibDir);
   sprintf(fileName, "%s/board%d/ResponseCalib_board%d_chip%d_%dMHz.bin", calibDir,
           fBoard->GetBoardSerialNumber(), fBoard->GetBoardSerialNumber(), chipIndex,
           static_cast < int >(fBoard->GetNominalFrequency() * 1000));

   fileHandle = fopen(fileName, "rb");
   if (fileHandle == NULL) {
      printf("Board %d --> Could not find response calibration file:\n", fBoard->GetBoardSerialNumber());
      printf("%s\n", fileName);
      return false;
   }

   if (fInitialized)
      delete fCalibrationData[chipIndex];
   fCalibrationData[chipIndex] = new CalibrationData(1);
   CalibrationData *data = fCalibrationData[chipIndex];
   CalibrationData::CalibrationDataChannel * chn;
   data->fRead = true;
   data->fHasOffsetCalibration = 1;
   fCalibrationValid[chipIndex] = true;
   data->fStartTemperature = 0;
   data->fEndTemperature = 0;

   // read channel
   for (k = 0; k < kNumberOfCalibChannelsV4; k++) {
      chn = data->fChannel[k];
      for (l = 0; l < kNumberOfBins; l++) {
         // Offset
         num = fread(&chn->fOffset[l], 2, 1, fileHandle);
         if (num != 1) {
            printf("Error while reading response calibration file '%s'\n", fileName);
            printf("   at 'Offset' of channel %d bin %d.\n", k, l);
            return false;
         }
         if (fBoard->GetDRSType() == 3) {
            // Gain
            num = fread(&chn->fGain[l], 2, 1, fileHandle);
            if (num != 1) {
               printf("Error while reading response calibration file '%s'\n", fileName);
               printf("   at 'Gain' of channel %d bin %d.\n", k, l);
               return false;
            }
         }
      }
   }

   fclose(fileHandle);
   return true;
}

/*------------------------------------------------------------------*/

float ResponseCalibration::GetValue(float *coefficients, float u, int n)
{
   int j, ii;
   float bsplines[4];
   ii = CalibrationData::CalculateBSpline(n, u, bsplines);

   float s = 0;
   for (j = 0; j < kBSplineOrder; j++) {
      s += coefficients[ii + j] * bsplines[j];
   }
   return s;
}

/*------------------------------------------------------------------*/

int ResponseCalibration::Approx(float *p, float *uu, int np, int nu, float *coef)
{
   int i, iu, j;

   const int mbloc = 50;
   int ip = 0;
   int ir = 0;
   int mt = 0;
   int ileft, irow;
   float bu[kBSplineOrder];
   float *matrix[kBSplineOrder + 2];
   for (i = 0; i < kBSplineOrder + 2; i++)
      matrix[i] = new float[mbloc + nu + 1];
   for (iu = kBSplineOrder - 1; iu < nu; iu++) {
      for (i = 0; i < np; i++) {
         if (1 <= uu[i])
            ileft = nu - 1;
         else if (uu[i] < 0)
            ileft = kBSplineOrder - 2;
         else
            ileft = kBSplineOrder - 1 + static_cast < int >(uu[i] * (nu - kBSplineOrder + 1));
         if (ileft != iu)
            continue;
         irow = ir + mt;
         mt++;
         CalibrationData::CalculateBSpline(nu, uu[i], bu);
         for (j = 0; j < kBSplineOrder; j++) {
            matrix[j][irow] = bu[j];
         }
         matrix[kBSplineOrder][irow] = p[i];
         if (mt < mbloc)
            continue;
         LeastSquaresAccumulation(matrix, kBSplineOrder, &ip, &ir, mt, iu - kBSplineOrder + 1);
         mt = 0;
      }
      if (mt == 0)
         continue;
      LeastSquaresAccumulation(matrix, kBSplineOrder, &ip, &ir, mt, iu - kBSplineOrder + 1);
      mt = 0;
   }
   if (!LeastSquaresSolving(matrix, kBSplineOrder, ip, ir, coef, nu)) {
      for (i = 0; i < kBSplineOrder + 2; i++)
         delete matrix[i];
      return 0;
   }

   for (i = 0; i < kBSplineOrder + 2; i++)
      delete matrix[i];
   return 1;
}

/*------------------------------------------------------------------*/

void ResponseCalibration::LeastSquaresAccumulation(float **matrix, int nb, int *ip, int *ir, int mt, int jt)
{
   int i, j, l, mu, k, kh;
   float rho;

   if (mt <= 0)
      return;
   if (jt != *ip) {
      if (jt > (*ir)) {
         for (i = 0; i < mt; i++) {
            for (j = 0; j < nb + 1; j++) {
               matrix[j][jt + mt - i] = matrix[j][(*ir) + mt - i];
            }
         }
         for (i = 0; i < jt - (*ir); i++) {
            for (j = 0; j < nb + 1; j++) {
               matrix[j][(*ir) + i] = 0;
            }
         }
         *ir = jt;
      }
      mu = min(nb - 1, (*ir) - (*ip) - 1);
      if (mu != 0) {
         for (l = 0; l < mu; l++) {
            k = min(l + 1, jt - (*ip));
            for (i = l + 1; i < nb; i++) {
               matrix[i - k][(*ip) + l + 1] = matrix[i][(*ip) + l + 1];
            }
            for (i = 0; i < k; i++) {
               matrix[nb - i - 1][(*ip) + l + 1] = 0;
            }
         }
      }
      *ip = jt;
   }
   kh = min(nb + 1, (*ir) + mt - (*ip));

   for (i = 0; i < kh; i++) {
      Housholder(i, max(i + 1, (*ir) - (*ip)), (*ir) + mt - (*ip), matrix, i, (*ip), &rho, matrix, i + 1,
                 (*ip), 1, nb - i);
   }

   *ir = (*ip) + kh;
   if (kh < nb + 1)
      return;
   for (i = 0; i < nb; i++) {
      matrix[i][(*ir) - 1] = 0;
   }
}

/*------------------------------------------------------------------*/

int ResponseCalibration::LeastSquaresSolving(float **matrix, int nb, int ip, int ir, float *x, int n)
{
   int i, j, l, ii;
   float s, rsq;
   for (j = 0; j < n; j++) {
      x[j] = matrix[nb][j];
   }
   rsq = 0;
   if (n <= ir - 1) {
      for (j = n; j < ir; j++) {
         rsq += pow(matrix[nb][j], 2);
      }
   }

   for (ii = 0; ii < n; ii++) {
      i = n - ii - 1;
      s = 0;
      l = max(0, i - ip);
      if (i != n - 1) {
         for (j = 1; j < min(n - i, nb); j++) {
            s += matrix[j + l][i] * x[i + j];
         }
      }
      if (matrix[l][i] == 0) {
         printf("Error in LeastSquaresSolving.\n");
         return 0;
      }
      x[i] = (x[i] - s) / matrix[l][i];
   }
   return 1;
}

/*------------------------------------------------------------------*/

void ResponseCalibration::Housholder(int lpivot, int l1, int m, float **u, int iU1, int iU2, float *up,
                                     float **c, int iC1, int iC2, int ice, int ncv)
{
   int i, j, incr;
   float tol = static_cast < float >(1e-20);
   float tolb = static_cast < float >(1e-24);
   float cl, clinv, sm, b;

   if (lpivot < 0 || lpivot >= l1 || l1 > m - 1)
      return;
   cl = fabs(u[iU1][iU2 + lpivot]);

   // Construct the transformation
   for (j = l1 - 1; j < m; j++)
      cl = max(fabsf(u[iU1][iU2 + j]), cl);
   if (cl < tol)
      return;
   clinv = 1 / cl;
   sm = pow(u[iU1][iU2 + lpivot] * clinv, 2);
   for (j = l1; j < m; j++) {
      sm = sm + pow(u[iU1][iU2 + j] * clinv, 2);
   }
   cl *= sqrt(sm);
   if (u[iU1][iU2 + lpivot] > 0)
      cl = -cl;
   *up = u[iU1][iU2 + lpivot] - cl;
   u[iU1][iU2 + lpivot] = cl;

   if (ncv <= 0)
      return;
   b = (*up) * u[iU1][iU2 + lpivot];
   if (fabs(b) < tolb)
      return;
   if (b >= 0)
      return;
   b = 1 / b;
   incr = ice * (l1 - lpivot);
   for (j = 0; j < ncv; j++) {
      sm = c[iC1 + j][iC2 + lpivot] * (*up);
      for (i = l1; i < m; i++) {
         sm = sm + c[iC1 + j][iC2 + lpivot + incr + (i - l1) * ice] * u[iU1][iU2 + i];
      }
      if (sm == 0)
         continue;
      sm *= b;
      c[iC1 + j][iC2 + lpivot] = c[iC1 + j][iC2 + lpivot] + sm * (*up);
      for (i = l1; i < m; i++) {
         c[iC1 + j][iC2 + lpivot + incr + (i - l1) * ice] =
             c[iC1 + j][iC2 + lpivot + incr + (i - l1) * ice] + sm * u[iU1][iU2 + i];
      }
   }
}

/*------------------------------------------------------------------*/

int ResponseCalibration::MakeDir(const char *path)
{
   struct stat buf;
   if (stat(path, &buf)) {
#ifdef _MSC_VER
      return mkdir(path);
#else
      return mkdir(path, 0711);
#endif                          // R__UNIX
   }
   return 0;
}

/*------------------------------------------------------------------*/

ResponseCalibration::ResponseCalibration(DRSBoard * board)
:  fBoard(board)
    , fPrecision(0.1)           // mV
    , fInitialized(false)
    , fRecorded(false)
    , fFitted(false)
    , fOffset(false)
    , fNumberOfPointsLowVolt(0)
    , fNumberOfPoints(0)
    , fNumberOfMode2Bins(0)
    , fNumberOfSamples(0)
    , fNumberOfGridPoints(0)
    , fNumberOfXConstPoints(0)
    , fNumberOfXConstGridPoints(0)
    , fTriggerFrequency(0)
    , fShowStatistics(0)
    , fCalibFile(0)
    , fCurrentLowVoltPoint(0)
    , fCurrentPoint(0)
    , fCurrentSample(0)
    , fCurrentFitChannel(0)
    , fCurrentFitBin(0)
    , fResponseY(0)
    , fSamples(0)
    , fSampleUsed(0)
    , fXXFit(0)
    , fYYFit(0)
    , fWWFit(0)
    , fYYFitRes(0)
    , fYYSave(0)
    , fXXSave(0)
    , fStatisticsApprox(0)
    , fStatisticsApproxExt(0)
{
   int i;
   // Initializing the Calibration Class
   CalibrationData::fIntRevers[0] = 0;
   for (i = 1; i < 2 * kBSplineOrder - 2; i++) {
      CalibrationData::fIntRevers[i] = static_cast < float >(1.) / i;
   }
   for (i = 0; i < kNumberOfChipsMax; i++) {
      fCalibrationData[i] = NULL;
   }
   // Initializing the Calibration Creation
   fCalibrationValid[0] = false;
   fCalibrationValid[1] = false;
}

/*------------------------------------------------------------------*/

ResponseCalibration::~ResponseCalibration()
{
   // Delete the Calibration
   for (int i=0 ; i<kNumberOfChipsMax ; i++)
      delete fCalibrationData[i];

   // Deleting the Calibration Creation
   DeleteFields();
}

/*------------------------------------------------------------------*/

float ResponseCalibration::CalibrationData::fIntRevers[2 * kBSplineOrder - 2];
ResponseCalibration::CalibrationData::CalibrationData(int numberOfGridPoints)
:fRead(false)
, fNumberOfGridPoints(numberOfGridPoints)
, fHasOffsetCalibration(0)
, fStartTemperature(0)
, fEndTemperature(0)
, fMin(0)
, fMax(0)
, fNumberOfLimitGroups(0)
{
   int i;
   for (i = 0; i < kNumberOfCalibChannelsV3; i++) {
      fChannel[i] = new CalibrationDataChannel(numberOfGridPoints);
   }
   for (i = 0; i < kNumberOfADCBins; i++) {
      fBSplineOffsetLookUp[i] = NULL;
      fBSplineLookUp[i] = NULL;
   }
};

/*------------------------------------------------------------------*/

void ResponseCalibration::CalibrationData::PreCalculateBSpline()
{
   int i, j;
   float uu;
   float xmin, xrange;
   int nk = fNumberOfGridPoints - kBSplineOrder + 1;
   for (i = 0; i < kNumberOfADCBins; i++) {
      fBSplineLookUp[i] = new float *[fNumberOfLimitGroups];
      fBSplineOffsetLookUp[i] = new int[fNumberOfLimitGroups];
      for (j = 0; j < fNumberOfLimitGroups; j++) {
         fBSplineLookUp[i][j] = new float[kBSplineOrder];
         xmin = fMin + j * kBSplineXMinOffset;
         xrange = fMax - xmin;
         uu = (i - xmin) / xrange;
         if (i < xmin) {
            uu = 0;
         }
         if (i - xmin > xrange) {
            uu = 1;
         }
         fBSplineOffsetLookUp[i][j] = static_cast < int >(uu * nk);
         CalculateBSpline(fNumberOfGridPoints, uu, fBSplineLookUp[i][j]);
      }
   }
}

/*------------------------------------------------------------------*/

void ResponseCalibration::CalibrationData::DeletePreCalculatedBSpline()
{
   int i, j;
   for (i = 0; i < kNumberOfADCBins; i++) {
      if (fBSplineLookUp[i] != NULL) {
         for (j = 0; j < fNumberOfLimitGroups; j++)
            delete fBSplineLookUp[i][j];
      }
      delete fBSplineLookUp[i];
      delete fBSplineOffsetLookUp[i];
   }
}

/*------------------------------------------------------------------*/

ResponseCalibration::CalibrationData::~CalibrationData()
{
   int i, j;
   for (i = 0; i < kNumberOfCalibChannelsV3; i++) {
      delete fChannel[i];
   }
   for (i = 0; i < kNumberOfADCBins; i++) {
      if (fBSplineLookUp[i] != NULL) {
         for (j = 0; j < fNumberOfLimitGroups; j++) {
            delete fBSplineLookUp[i][j];
         }
      }
      delete fBSplineLookUp[i];
      delete fBSplineOffsetLookUp[i];
   }
};

/*------------------------------------------------------------------*/

int ResponseCalibration::CalibrationData::CalculateBSpline(int nGrid, float value, float *bsplines)
{
   int minimum;
   int maximum;
   float xl;

   int nk = nGrid - kBSplineOrder + 1;
   float vl = value * nk;
   int ivl = static_cast < int >(vl);

   if (1 <= value) {
      xl = vl - nk + 1;
      minimum = 1 - nk;
   } else if (value < 0) {
      xl = vl;
      minimum = 0;
   } else {
      xl = vl - ivl;
      minimum = -ivl;
   }
   maximum = nk + minimum;

//   printf("xl = %f\n",xl);
   float vm, vmprev;
   int jl, ju;
   int nb = 0;

   bsplines[0] = 1;
   for (int i = 0; i < kBSplineOrder - 1; i++) {
      vmprev = 0;
      for (int j = 0; j < nb + 1; j++) {
         jl = max(minimum, j - nb);
         ju = min(maximum, j + 1);
         vm = bsplines[j] * fIntRevers[ju - jl];
         bsplines[j] = vm * (ju - xl) + vmprev;
         vmprev = vm * (xl - jl);
      }
      nb++;
      bsplines[nb] = vmprev;
   }
   return -minimum;
}

/*------------------------------------------------------------------*/

void ResponseCalibration::Average(int method, float *points, int numberOfPoints, float &mean, float &error,
                                  float sigmaBoundary)
{
   // Methods :
   // 0 : Average
   // 1 : Average inside sigmaBoundary*sigma
   int i;
   float sum = 0;
   float sumSquare = 0;

   if (method == 0 || method == 1) {
      for (i = 0; i < numberOfPoints; i++) {
         sum += points[i];
         sumSquare += points[i] * points[i];
      }

      mean = sum / numberOfPoints;
      error = sqrt((sumSquare - sum * sum / numberOfPoints) / (numberOfPoints - 1));
   }
   if (method == 1) {
      int numberOfGoodPoints = numberOfPoints;
      bool found = true;
      bool *goodSample = new bool[numberOfGoodPoints];
      for (i = 0; i < numberOfGoodPoints; i++)
         goodSample[i] = true;

      while (found) {
         found = false;
         for (i = 0; i < numberOfPoints; i++) {
            if (goodSample[i] && fabs(points[i] - mean) > sigmaBoundary * error) {
               found = true;
               goodSample[i] = false;
               numberOfGoodPoints--;
               sum -= points[i];
               sumSquare -= points[i] * points[i];
               mean = sum / numberOfGoodPoints;
               error = sqrt((sumSquare - sum * sum / numberOfGoodPoints) / (numberOfGoodPoints - 1));
            }
         }
      }
      delete[] goodSample;
   }
}
