#ifndef CHECK_EOF_HH
#define CHECK_EOF_HH

#include <TTimer.h>

#include <vector>
#include <iostream>
#include <string>


//#include "OnlineHistograms.hh"
#include "BaseCollection.hh"
class BaseCollection;


class CheckEOF {
	//RQ_OBJECT("CheckEOF")
protected:
	bool _isEOF;
	TTimer *timer;
	std::vector <BaseCollection*> _colls;
	std::string _rootfilename;
public:
	CheckEOF();
	
	void TimerLoop();
	
	void EventReceived() {
		//std::cout << "Called event received!" << std::endl;
		_isEOF = false;
	}
	
	void setCollections(std::vector <BaseCollection*> colls);
	
	void startChecking(int mtime = 10000);
	
	void setRootFileName(std::string name) { _rootfilename = name; }
};

#ifdef __CINT__
#pragma link C++ class CheckEOF-;
#endif

#endif
