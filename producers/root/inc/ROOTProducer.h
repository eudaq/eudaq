#ifndef ROOT_PRODUCER_H__
#define ROOT_PRODUCER_H__

#include "RQ_OBJECT.h"
#include "Rtypes.h"

#include <string>

class ROOTProducer {
  RQ_OBJECT("ROOTProducer")
public:
  ROOTProducer();
  ~ROOTProducer();

  void Connect2RunControl(const char *name, const char *runcontrol);
  bool isNetConnected();

  const char *getProducerName();

  int getConfiguration(const char *tag, int defaultValue);
  int getConfiguration(const char *tag, const char *defaultValue,
                       char *returnBuffer, Int_t sizeOfReturnBuffer);
  std::string getConfiguration( const char* tag, const std::string &defaultValue);

  
  void createNewEvent(unsigned nev);
  void createEOREvent();
  void addData2Event(unsigned dataid,const std::vector<unsigned char>& data);
  void addData2Event(unsigned dataid, UChar_t * data, size_t size);
  void appendData2Event(unsigned dataid,const std::vector<unsigned char>& data);
  void appendData2Event(unsigned dataid,const unsigned char* data, size_t size);

  void sendEvent();

  void setTimeStamp(ULong64_t TimeStamp);
  void setTimeStamp2Now();
  
  void setTag(const char *tagNameTagValue); //"tag=value"
  void setFileTag(const char *tagNameTagValue); //"tag=value"
  void setTimerTag(const char *tagNameTagValue, size_t freq);

  void sendLog(const char *msg);
  //signal
  void send_OnStartRun(unsigned);
  void send_OnConfigure();
  void send_OnStopRun();
  void send_OnTerminate();

  void checkStatus();
  void setStatusToStop();
      
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
