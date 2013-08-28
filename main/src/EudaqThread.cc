
#include "eudaq/EudaqThread.hh"
#ifdef CPP11
#include <thread>
namespace eudaq {
	class eudaqThread::Impl{
	public:
		Impl(void *(*start1) (void *),void *arg1):t1(start1,arg1){
		}
		std::thread t1;
	};



}

eudaq::eudaqThread::eudaqThread():m_impl(nullptr)
{
	
}

eudaq::eudaqThread::eudaqThread(void *(*start1) (void *),void *arg1)
{
	start(start1 ,arg1);
}



eudaq::eudaqThread::~eudaqThread()
{
	if (m_impl!=nullptr)
	{
		m_impl->t1.join();
		delete m_impl;
		m_impl=nullptr;
	}

}

void eudaq::eudaqThread::start( void *(*start) (void *),void *arg )
{
	m_impl=new eudaqThread::Impl(start,arg);
}

void eudaq::eudaqThread::detach()
{
	m_impl->t1.detach();
}

void eudaq::eudaqThread::join()
{
	if (m_impl!=nullptr)
	{
		m_impl->t1.join();
	}
}

bool eudaq::eudaqThread::joinable() const
{
	if (m_impl!=nullptr)
	{
		return m_impl->t1.joinable();
	}
	return false;
}


#else
#include "pthread.h"
namespace eudaq {
	class eudaqThread::Impl{
	public:
		Impl(){
		}
		pthread_t m_thread;
		pthread_attr_t m_threadattr;
	};

	eudaqThread::eudaqThread():m_impl(new eudaqThread::Impl())
	{

	}

	eudaq::eudaqThread::eudaqThread(void *(*start1) (void *),void *arg1):m_impl(new eudaqThread::Impl())
	{
		start(start1 ,arg1);
	}



	eudaq::eudaqThread::~eudaqThread()
	{
		join();

	}

	void eudaq::eudaqThread::start( void *(*start1) (void *),void *arg1 )
	{
		pthread_attr_init(&m_impl->m_threadattr);
		pthread_create(&m_impl->m_thread, &m_impl->m_threadattr, start1, arg1);
		
	}

	void eudaq::eudaqThread::detach()
	{
		// do nothing
	}

	void eudaq::eudaqThread::join()
	{
			pthread_join(m_impl->m_thread, 0);
	}

	bool eudaq::eudaqThread::joinable() const
	{
		// not implemented
		return true;
	}

}

#endif