#ifndef ROOT_PRODUCER_H__
#define ROOT_PRODUCER_H__

#include "RQ_OBJECT.h"
#include "Rtypes.h"

#ifdef WIN32
#ifndef __CINT__
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif
#else
#define DLLEXPORT
#endif


/* #if ((defined WIN32) && (defined __CINT__)) */
/* typedef unsigned long long uint64_t */
/* typedef long long int64_t */
/* typedef unsigned int uint32_t */
/* typedef int int32_t */
/* #else */
/* #include <cstdint> */
/* #endif */

/* #include <cstdint> */


class DLLEXPORT ROOTProducer {
  RQ_OBJECT("ROOTProducer")
public:
  ROOTProducer(const char *name, const char *runcontrol);
  ROOTProducer();
  ~ROOTProducer();

  void Connect2RunControl(const char *name, const char *runcontrol);
  bool isNetConnected();

  const char *getProducerName();

  int getConfiguration(const char *tag, int DefaultValue);
  int getConfiguration(const char *tag, const char *defaultValue,
                       char *returnBuffer, Int_t sizeOfReturnBuffer);
  /* int getConfiguration(const char *tag); */

  void createNewEvent(unsigned nev);
  void createEOREvent();
  void addData2Event(unsigned dataid,const std::vector<unsigned char>& data);
  void addData2Event(unsigned dataid,const unsigned char* data, size_t size);
  void sendEvent();

  void setTimeStamp(ULong64_t TimeStamp);
  void setTimeStamp2Now();
  
  void setTag(const char *tag, const char *Value);
  void setTag(const char *tagNameTagValue); //"tag=value"


  //signal
  void send_OnStartRun(unsigned);
  void send_OnConfigure();
  void send_OnStopRun();
  void send_OnTerminate();

  
  void checkStatus();
  void setStatusToStopped();

  //TODO:: remove them, only for test
  void createNewEvent(int nev){createNewEvent(unsigned(nev));};
  void addData2Event(int nev, int mid, int len, ULong64_t* ptr){
    addData2Event((unsigned)mid, reinterpret_cast<unsigned char*>(ptr), 
		  sizeof(ULong64_t)*(unsigned)len);}
  void sendEvent(int){sendEvent();};
  
  void addDataPointer_bool(unsigned bid, bool* data, size_t size);
  void addDataPointer_uchar(unsigned bid, unsigned char* data, size_t size);
  void addDataPointer_ULong64_t(unsigned bid, ULong64_t* data, size_t size);
  
private:
  std::vector<bool*> m_vpoint_bool;
  std::vector<size_t> m_vsize_bool;
  std::vector<unsigned> m_vblockid_bool;

  std::vector<unsigned char*> m_vpoint_uchar;
  std::vector<size_t> m_vsize_uchar;
  std::vector<unsigned> m_vblockid_uchar;

  
#ifndef __CINT__
  class Producer_PImpl;
  Producer_PImpl *m_prod;
#endif
  ClassDef(ROOTProducer, 0)
};



#ifdef __CINT__
#pragma link C++ class ROOTProducer;
#endif

#endif // ROOT_PRODUCER_H__
