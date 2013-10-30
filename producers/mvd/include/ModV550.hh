#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <csignal>
#include <unistd.h>
#include <cctype>

#include "VMEInterface.hh"

#define MODV550_CHANNELS 2

/*=====================================================================*/
/*          class Mod_v550    Readout ADC                              */
/*=====================================================================*/
class ModV550{  /* Readout ADC */

  int debug;  /* debug level */
  unsigned int VMEaddr;          /* base VME address of module            */
  unsigned int Len;              /* length of module in address space     */
  unsigned int   Base;             /* Base address of module in memory      */
  unsigned int   PT1;              /* Pedestal/Threshold channel 1, r/w D32 */
  unsigned int   PT0;              /* Pedestal/Threshold channel 0, r/w D32 */
  unsigned int   Vers;             /* Version and Series            r/o     */
  unsigned int   Type;             /* Manufacturer and module type  r/o     */
  unsigned int   Code;             /* Fixed code                    r/o     */
  unsigned int   Test1;            /* Test pattern channel 1        w/o     */
  unsigned int   Test0;            /* Test pattern channel 0        w/o     */
  unsigned int   WC1;              /* Word count channel 1          r/o     */
  unsigned int   WC0;              /* Word count channel 0          r/o     */
  unsigned int   Fifo_1;           /* FIFO channel 1                r/o D32 */
  unsigned int   Fifo_0;           /* FIFO channel 0                r/o D32 */
  unsigned int   MClear;           /* Module Clear                  w/o     */
  unsigned int   Nchan;            /* Number of channels            r/w     */
  unsigned int   Status;           /* Status Register               r/w     */
  unsigned int   IRQ;              /* Interrupt register            w/o     */

public:

  VMEptr vme;
  VMEptr vme32;

  unsigned short int data;

  ModV550(unsigned int);
  ~ModV550();

  void ModuleInfo();
  void SetTestPattern(unsigned short int, unsigned short int);
  void GetWordCounter(unsigned int&, unsigned int&);
  unsigned int GetWordCounter(unsigned int n);
  void GetFifo0(unsigned int, unsigned int, int*, int*);
  void GetFifo1(unsigned int, unsigned int, int*, int*);
  void Clear();
  void SetNumberOfChannels(unsigned short int, unsigned short int);
  void GetNumberOfChannels(unsigned int&, unsigned int&);
  void GetStatus();
  unsigned int GetTestMode();
  unsigned int GetMemoryOwner();
  unsigned int GetDRDY0();
  unsigned int GetDRDY1();
  unsigned int GetFifoEmpty0();
  unsigned int GetFifoEmpty1();
  unsigned int GetFifoHF0();
  unsigned int GetFifoHF1();
  unsigned int GetFifoFull0();
  unsigned int GetFifoFull1();
  void SetTestMode(unsigned int);
  void SetMemoryOwner(unsigned int);
  void SetInterrupt(unsigned short int, unsigned short int);
  void SetPedestalThreshold(int, int, int, int);
  void SetPedestalThresholdFull(int, int, int );
  void GetPedestalThreshold(int, int, int*, int*);
  void GetPedestalThresholdFull(int, int, int* );
  void SetPedThrZero();
  void Init(unsigned int);
  unsigned int GetEvent(unsigned int);

};
