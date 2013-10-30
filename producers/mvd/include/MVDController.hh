#include "ModV550.hh"
#include "ModV551.hh"
#include "eudaq/Configuration.hh"
#include <math.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include<sys/time.h>
#include <vector>
using namespace std;

#define NumOfCha 1280
#define NumOfSi 3
#define BuffSize NumOfCha*NumOfSi

#define EventLength 1280*3 + 20 + 256 + 10

class MVDController {
public:
  MVDController();
  void Configure(const eudaq::Configuration & conf);
  void ReadCalFile();
  void WriteCalFile();
  bool DataReady();
  bool DataBusy();
  bool ActiveSequence();
  bool GetStatTrigger();
  int swap4(int w4);
  void TimeEvents();
  void CalibAutoTrig();
  bool Enabled(unsigned adc, unsigned chan);
  std::vector<unsigned int> Read(unsigned adc, unsigned chan);
  std::vector<unsigned int> Time(unsigned adc);
  void Clear();
  unsigned NumADCs() const { return NumOfADC; }
  virtual ~MVDController();
  const char* pather;
private:
  unsigned NumOfChan, NumOfSil, NumOfADC;

  struct timeval tv;
  unsigned int sec;
  unsigned int microsec;
  time_t    timer;
  struct tm *date;

  ModV551 * m_modv551;
  std::vector<ModV550 *> m_modv550;
  std::vector<bool> m_enabled;

  unsigned int OR;
  unsigned int V;
  unsigned int ChTmp;
  unsigned int ChNum;
  unsigned int ChData;
  unsigned int SilcPointer;

};
