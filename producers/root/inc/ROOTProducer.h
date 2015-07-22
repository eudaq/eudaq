#ifndef SCT_PRODUCER_H__
#define SCT_PRODUCER_H__

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

class DLLEXPORT ROOTProducer {
  RQ_OBJECT("ROOTProducer")
public:
  ROOTProducer(const char *name, const char *runcontrol);
  ROOTProducer();
  ~ROOTProducer();

  void Connect2RunControl(const char *name, const char *runcontrol);
  bool getConnectionStatus();

  const char *getProducerName();

  bool ConfigurationSatus();
  int getConfiguration(const char *tag, int DefaultValue);
  int getConfiguration(const char *tag, const char *defaultValue,
                       char *returnBuffer, Int_t sizeOfReturnBuffer);
  void getConfiguration(const char *tag);
  void emitConfiguration(const char *answer);

  void createNewEvent();
  void createNewEvent(int);
  void createEOREvent();
  void setTimeStamp(ULong64_t TimeStamp);
  void setTimeStamp2Now();
  void setTag(const char *tag, const char *Value);
  void setTag(const char *tagNameTagValue); // to use the signal slot mechanism
                                            // use this function.
                                            // example:
                                            // setTag("tagName=tagValue");
                                            // the equal symbol is mandatory.
  void AddPlane2Event(unsigned Block_id,
                      const std::vector<unsigned char> &inputVector);
  void AddPlane2Event(unsigned Block_id, const bool *inputVector,
                      size_t Elements);
  void AddPlane2Event(unsigned MODULE_NR, int ST_STRIPS_PER_LINK,
                      bool *evtr_strm0, bool *evtr_strm1);

  void addDataPointer_bool(unsigned Block_id, const bool *inputVector,
                           size_t Elements);
  void addDataPointer_UChar_t(unsigned Block_id,
                              const unsigned char *inputVector,
                              size_t Elements);
  void addDataPointer_Uint_t(unsigned Block_id, const unsigned int *inputVector,
                             size_t Elements);
  void addDataPointer_ULong64_t(unsigned Block_id,
                                const unsigned long long *inputVector,
                                size_t Elements);

  void resetDataPointer();

  void sendEvent();
  void sendEvent(int);

  // signals

  void send_onStart(int); // sync 	void send_onStart(int RunNumber);
  void send_onConfigure(); // sync
  void send_onStop();      // sync
  void send_OnTerminate(); // sync
  void send_statusChanged(); // sync

  // status flags
  void checkStatus();

  void setTimeOut(int);

  bool getOnStart();
  void setOnStart(bool newStat);

  bool getOnConfigure();
  void setOnconfigure(bool newStat);

  bool getOnStop();
  void setOnStop(bool newStat);
  void setStatusToStopped();

  bool getOnTerminate();
  void setOnTerminate(bool newStat);

private:
#ifndef __CINT__
  class Producer_PImpl;
  Producer_PImpl *m_prod;
#endif
  ClassDef(ROOTProducer, 1)
};

#ifdef __CINT__
#pragma link C++ class ROOTProducer;
#endif

#endif // SCT_PRODUCER_H__
