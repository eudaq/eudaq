#ifndef H_HGCALCONTROLLER
#define H_HGCALCONTROLLER

#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <deque>
#include <sstream>
#include <vector>
#include <iomanip>

#include <TFile.h>
#include <TTree.h>

#include "IpbusHwController.h"

#define MAX_NUMBER_OF_ORM 16

enum HGCalControllerState{ 
  INIT,
  CONFED,
  RUNNING,
};

struct HGCalConfig
{
  std::string connectionFile;
  std::string rootFilePath;
  uint16_t rdoutMask;
  uint32_t blockSize;
  int logLevel;
  int uhalLogLevel;
  int timeToWaitAtEndOfRun;//in seconds
  bool saveRawData;
  bool checkCRC;
  bool throwFirstTrigger;
};

class HGCalDataBlocks
{
  /*
    HGCalDataBlock contains data from all ORM boards (1 vector for each ORM) controlled by this HGCalController instance. The vector size is fixed when the init is called. This size must be at least equal to the size of the Ipbus FIFO. If needed one can can allocate extra space to add trailler by using the method setValueInBlock.
   */
 public:
  HGCalDataBlocks(){;}
  ~HGCalDataBlocks(){;}
  void initBlocks(uint16_t _mask, uint32_t _size)
  {
    for(int iorm=0; iorm<MAX_NUMBER_OF_ORM; iorm++){
      if( (_mask>>iorm)&1 ){
	std::vector<uint32_t> vec;
	vec.resize(_size);
	m_data.insert( std::pair< int,std::vector<uint32_t> >(iorm,vec) );
      }
    }
  }
  inline void copyDataInBlock(int blkID, std::vector<uint32_t> data){ std::copy(data.begin(),data.end(),m_data[blkID].begin()); }
  inline void setValueInBlock(int blkID, int pos, uint32_t val){ m_data[blkID][pos]=val; }
  inline std::map< int,std::vector<uint32_t> > &getData(){return m_data;}
 private:
  std::map< int,std::vector<uint32_t> > m_data;
};

class HGCalController
{
 public:
  HGCalController(){;}
  ~HGCalController(){;}
  void init();
  void configure(HGCalConfig config);
  void startrun(int runId);
  void stoprun();
  void terminate();
  inline std::deque<HGCalDataBlocks>& getDequeData(){return m_deqData;}
  
 private:
  void run();
  static void readFIFOThread(ipbus::IpbusHwController* orm);
  void readAndThrowFirstTrigger();
  //
  int m_runId;
  int m_evtId;
  int m_triggerId;
  boost::mutex m_mutex;
  HGCalConfig m_config;
  HGCalControllerState m_state;
  bool m_gotostop;
  std::ofstream m_rawFile;
  std::map< int,ipbus::IpbusHwController* > m_rdout_orms;
  std::deque<HGCalDataBlocks> m_deqData;
  
  // root stuff to store timing information
  TFile *m_rootfile;
  TTree *m_roottree;
  float m_daqrate;
  float m_eventtime;
  float m_rdoutreadytime;
  float m_readouttime;
  float m_datestamptime;
  float m_rdoutdonetime;

};

#endif
