#ifndef ITSROOT_PRODUCER_H__
#define ITSROOT_PRODUCER_H__

#include "RQ_OBJECT.h"
#include "Rtypes.h"

#include <string>

#ifndef __CINT__
#include "eudaq/RawEvent.hh"
#include <mutex>
class ItsRootProducer;
#endif

class ROOTProducer {
  RQ_OBJECT("ROOTProducer")
public:
  ROOTProducer();
  ~ROOTProducer();
  //signal
  void send_OnInilialise();
  void send_OnConfigure();
  void send_OnStartRun(unsigned);
  void send_OnStopRun();
  /* void send_OnReset(); */
  void send_OnTerminate();
  //
  
  void Connect2RunControl(const char *name, const char *runcontrol);
  bool isNetConnected();
  void checkStatus();
  int getConfiguration(const char *tag, int defaultValue);
  void createNewEvent(unsigned nev);
  void setTag(const char *tagNameTagValue); //"tag=value"
  void addData2Event(unsigned dataid, UChar_t * data, size_t size);
  void sendEvent();
  void sendLog(const char *msg);
  
  void setStatusToStop(){};//do nothing  
private:
#ifndef __CINT__
  std::unique_ptr<ItsRootProducer> m_prod;
  eudaq::EventUP ev;
  std::mutex m_mtx_ev;
#endif
  ClassDef(ROOTProducer, 0)
};

#ifdef __CINT__
#pragma link C++ class ROOTProducer;
#endif

#endif // ROOT_PRODUCER_H__
