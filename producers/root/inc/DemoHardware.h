#ifndef DEMO_ROOT_H
#define DEMO_ROOT_H


#include "TQObject.h"

/////////////////////////////////////////

class DemoHardware:public TQObject{
public:
  DemoHardware():m_isStarted(0){};
  
  void createNewEvent(unsigned nev){Emit("createNewEvent(unsigned)", nev);};
  void addData2Event(unsigned dataid, unsigned char* data, size_t size){
    EmitVA("addData2Event(unsigned, unsigned char*, size_t)", 3, dataid, data, size);};
  void sendEvent(){Emit("sendEvent()");};
  void addTag2Event(char * v){Emit("setTag(char*)", v);}

  void hdConfigure();
  void hdStartRun(unsigned nrun);
  void hdStopRun();

  void dummySendEvent(int nev);

  
  bool isStarted(){return m_isStarted != 0;};
  int m_isStarted;

  static void run(const char * rpro_name = "DemoHardware", const char* runctrl_addr = "127.0.0.1");

  ClassDef(DemoHardware, 0)
};

#ifdef __CINT__
#pragma link C++ class DemoHardware;
#endif
//NOTE:: class name should be same to header name for recent CMAKE config


#endif
