#ifndef SCT_PRODUCER_H__
#define SCT_PRODUCER_H__



#include "RQ_OBJECT.h"
#include "RTypes.h"


#ifndef  __CINT__
#define  DLLEXPORT  __declspec( dllexport ) 
#else
#define  DLLEXPORT
#endif







class DLLEXPORT SCTProducer
{
	RQ_OBJECT("SCTProducer")
public:

	// The constructor must call the eudaq::Producer constructor with the name
	// and the runcontrol connection string, and initialize any member variables.
	SCTProducer(const char* name,const char* runcontrol);
	SCTProducer();
	~SCTProducer();
	// This is just an example, adapt it to your hardware

	void Connect2RunControl(const char* name,const char* runcontrol);
	bool getConnectionStatus();

	



	bool ConfigurationSatus();
	int getConfiguration(const char* tag, int DefaultValue);
	int getConfiguration(const char* tag, const char* defaultValue,char* returnBuffer,Int_t sizeOfReturnBuffer);



	void createNewEvent();
	void setTimeStamp(unsigned long long TimeStamp);
	void setTimeStamp2Now();
	void setTag(const char* tag,const char* Value);
	void AddPlane2Event(unsigned plane,const std::vector<unsigned char>& inputVector);
	void AddPlane2Event(unsigned plane,const bool* inputVector,size_t Elements);
	void sendEvent();

	// signals
	void send_start_run();    //async
	void send_onStart();      //sync 
	void send_onConfigure();  // sync
	void send_onStop();       // sync
	void send_OnTerminate();  //sync 
	



	//status flags
	void checkStatus();

	bool getOnStart();
	void setOnStart(bool newStat);

	bool getOnConfigure();
	void setOnconfigure(bool newStat);

	bool getOnStop();
	void setOnStop(bool newStat);

	bool getOnTerminate();
	void setOnTerminate(bool newStat);

private:
	class Producer_PImpl;
	Producer_PImpl* m_prod;

	ClassDef(SCTProducer,1)
};

#ifdef __CINT__
#pragma link C++ class SCTProducer;
#endif


#endif // SCT_PRODUCER_H__
