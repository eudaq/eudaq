#include "DemoHardware.h"

#include<iostream>
#include<string>
#include<vector>
#include<map>
#include<thread>
#include<chrono>
#include<random>
#include<sstream>


#include"ROOTProducer.h"


void DemoHardware::hdConfigure(){
  //TODO::

  
}


void DemoHardware::hdStartRun(unsigned nrun){
    //TODO:: add tag to BORE
    int abc_nblocks =2;
    std::vector<int> abc_blockids;
    abc_blockids.push_back(0);
    abc_blockids.push_back(1);
    std::vector<std::string> abc_blocktypes;
    abc_blocktypes.push_back(std::string("ABC130"));
    abc_blocktypes.push_back(std::string("ABC130"));
    std::stringstream ss;
    ss <<"ABC_N_BLOCKS"<< '=' << abc_nblocks;
    addTag2Event( ss.str().c_str());
    for(int i =0; i<abc_nblocks; i++){
      ss.str("");
      ss <<"ABC_BLOCKID_"<<i<< '=' << abc_blockids[i];
      addTag2Event( ss.str().c_str());
      ss.str("");
      ss <<"ABC_BLOCKTYPE_"<<i<< '=' << abc_blocktypes[i];
      addTag2Event( ss.str().c_str());
    }

    ss.str("");
    ss << "ST_DET_FILE"<<'='<<"/opt/itsdaq-sw/config/st_system_config.dat";
    addTagFile2Event(ss.str().c_str());
    
    m_isStarted = 1;
}


void DemoHardware::hdStopRun(){m_isStarted = 0;}

void DemoHardware::dummySendEvent(int nev){
  static size_t size = 256;
  static std::default_random_engine generator0;
  static std::default_random_engine generator1;
  static std::uniform_int_distribution<int> distribution(0,size-1);
  static std::normal_distribution<double> normal_distribution(0.5,2);

  static size_t bsize = (size+7)/8;
  static std::vector<unsigned char> data(bsize, 0);

  int rdm;
  unsigned char tmp;
  createNewEvent(unsigned(nev));
  rdm = distribution(generator0);
  tmp = data[rdm/8]; 
  data[rdm/8]= data[rdm/8] | (1<<(rdm%8));
  addData2Event(0, data.data(), data.size());
  data[rdm/8]= tmp;
  rdm = int(normal_distribution(generator1)+rdm);
  if(rdm<0)
    rdm = 0;
  if(rdm>size-1)
    rdm = size-1;
  tmp = data[rdm/8]; 
  data[rdm/8]= data[rdm/8] | (1<<(rdm%8));
  addData2Event(1, data.data(), data.size());
  data[rdm/8]=tmp;
  sendEvent();

}


void DemoHardware::run(const char * rpro_name, const char* runctrl_addr){
  DemoHardware *hd = new DemoHardware;
  ROOTProducer *rpro = new ROOTProducer;
  
  rpro->Connect("send_OnStartRun(unsigned)",  "DemoHardware", hd, "hdStartRun(unsigned)");
  rpro->Connect("send_OnConfigure()", "DemoHardware", hd, "hdConfigure()");
  rpro->Connect("send_OnStopRun()",      "DemoHardware", hd, "hdStopRun()");  
  hd->Connect("createNewEvent(unsigned)", "ROOTProducer", rpro, "createNewEvent(unsigned)");
  hd->Connect("addData2Event(unsigned, UChar_t*, size_t)","ROOTProducer", rpro,"addData2Event(unsigned, unsigned char*, size_t)");
  hd->Connect("sendEvent()",   "ROOTProducer", rpro, "sendEvent()");
  hd->Connect("addTag2Event(char*)", "ROOTProducer", rpro, "setTag(char*)");
  hd->Connect("addTagFile2Event(char*)", "ROOTProducer", rpro, "setTagFile(char*)");


  size_t size = 256;
  std::default_random_engine generator0;
  std::default_random_engine generator1;
  std::uniform_int_distribution<int> distribution(0,size-1);
  std::normal_distribution<double> normal_distribution(0.5,2);
  size_t bsize = (size+7)/8;
  std::vector<unsigned char> data(bsize, 0);
  
  do {
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    rpro->Connect2RunControl(rpro_name, runctrl_addr);
  } while (!rpro->isNetConnected());
  while (1){
    unsigned nev = 0;    
    if(hd->isStarted()){ //your readout loop
      nev++;
      int rdm;
      unsigned char tmp;
      hd->createNewEvent(nev);
      rdm = distribution(generator0);
      tmp = data[rdm/8]; 
      data[rdm/8]= data[rdm/8] | (1<<(rdm%8));
      hd->addData2Event(0, data.data(), data.size());
      data[rdm/8]= tmp;
      rdm = int(normal_distribution(generator1)+rdm);
      if(rdm<0)
	rdm = 0;
      if(rdm>size-1)
	rdm = size-1;
      tmp = data[rdm/8]; 
      data[rdm/8]= data[rdm/8] | (1<<(rdm%8));
      hd->addData2Event(1, data.data(), data.size());
      data[rdm/8]=tmp;
      hd->sendEvent();
    }
    rpro->checkStatus();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }
}
