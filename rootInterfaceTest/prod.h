#ifndef root_producer_h__
#define root_producer_h__


#include "RQ_OBJECT.h"
#include "TSystem.h"
#include "TMutex.h"
#ifndef __CINT__
class SCTProducer;
#endif



class prod{
	RQ_OBJECT("prod")
public:
	prod();
	~prod();
	void Connect2run(const char *name);
	void readoutloop();
	void onConfigure();
	void sendEvent();
	void onStopping();
#ifndef __CINT__
	SCTProducer *a;//=new SCTProducer("TLU","192.168.2.109:44000");
#endif
	TMutex mut;
	
	bool isRunning;
};

#ifdef __CINT__
#pragma link C++ class prod;
#endif
#endif // root_producer_h__