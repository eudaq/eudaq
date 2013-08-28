#ifndef EudaqThread_h__
#define EudaqThread_h__
#include "Utils.hh"


namespace eudaq{

class DLLEXPORT eudaqThread{
public:
	eudaqThread();
	eudaqThread(void *(*start) (void *),void *arg);
	void start(void *(*start) (void *),void *arg);
	~eudaqThread();
	bool joinable() const;
	void join();
	void detach();

	
private:
	class Impl;
	Impl * m_impl;

};


}




#endif // EudaqThread_h__
