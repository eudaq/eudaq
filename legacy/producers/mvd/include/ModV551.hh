#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <csignal>
#include <unistd.h>
#include <cctype>

#include "VMEInterface.hh"

/*=====================================================================*/
/*          class Mod_v551    Sequencer ADC                            */
/*=====================================================================*/
class ModV551{       /* Sequencer */

  int debug;    /* debug level */
  unsigned long udelay;   /* VME R/W delay */
  int IRQ_Vector;
  unsigned int VMEaddr;
  unsigned int Len;
  unsigned int Base;      /* base address      */
  unsigned int Vers;      /* Version and Series            r/o     */
  unsigned int Type;      /* Manufacturer and module type  r/o     */
  unsigned int Code;      /* Fixed code                    r/o     */
  unsigned int DAC;       /* internal DAC w/o  */
  unsigned int T5;        /* T5 register r/w   */
  unsigned int T4;        /* T4 register r/w   */
  unsigned int T3;        /* T3 register r/w   */
  unsigned int T2;        /* T2 register r/w   */
  unsigned int T1;        /* T1 register r/w   */
  unsigned int Nchan;     /* Number of channels r/w   */
  unsigned int Test;      /* Test Register      r/w   */
  unsigned int Status;    /* Status Register    r/w   */
  unsigned int STrigger;  /* Software Trigger   r/w   */
  unsigned int SClear;    /* Software Clear     r/w   */
  unsigned int IRQlevel;  /* Interrupt Level    w/o   */
  unsigned int IRQvector; /* Interrupt Vector   w/o   */

public:

  VMEptr vme;

  unsigned short int data;

  unsigned int time1, time2, time3, time4, time5, time6;

  ModV551(unsigned int);
  ~ModV551();

  unsigned int ReadVME16(unsigned int);
  void WriteVME16(unsigned int, unsigned int);
  void ModuleInfo();
  void SetIRQStatusID(unsigned int);
  void SetIRQLevel(unsigned int);
  void Clear();
  void SoftwareTrigger();
  unsigned int GetTrigger();
  void SetInternalDelay(unsigned int);
  void SetVeto(unsigned int);
  void SetAutoTrigger(unsigned int);
  void GetStatus();
  unsigned int GetDataRedy();
  unsigned int GetBusy();
  unsigned int GetActiveSequence();
  void SetTestMode(unsigned int);
  void SetClockLevel(unsigned int);
  void SetShiftLevel(unsigned int);
  void SetTestPulsLevel(unsigned int);
  void GetTest();
  unsigned int GetTestMode();
  unsigned int GetClockLevel();
  unsigned int GetShiftLevel();
  unsigned int GetTestPulsLevel();
  void SetNChannels(unsigned int);
  void GetNChannels();
  void SetTime(int, int, int, int, int);
  void GetTime(int*, int*, int*, int*, int*);
  void SetVCAL(unsigned int);
  //void CalibAutoTrig();
  void Init(unsigned int);

};
