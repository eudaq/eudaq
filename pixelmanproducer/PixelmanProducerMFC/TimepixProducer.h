#pragma once
#include "eudaq/Producer.hh"
#include <pthread.h>
#include "PixelmanProducerMFCDlg.h"
//#include "targetver.h"

#include <queue>


class TimepixProducer : public eudaq::Producer
{
  public:
    TimepixProducer(const std::string & name, const std::string & runcontrol,
					CPixelmanProducerMFCDlg* pixelmanCtrl);
    virtual ~TimepixProducer();

	enum timepix_producer_status_t  {RUN_STOPPED, RUN_ACTIVE};
	enum timepix_producer_command_t {NONE, START_RUN, STOP_RUN, TERMINATE, CONFIGURE, RESET};

    void Event(i16 *timepixdata, u32 size);
    void SimpleEvent();
    void BlobEvent();

    /** Threadsave version to set the m_done variable
     */
    void SetDone(bool done);

    /** Threadsave version to get (a copy of) the m_done variable
     */

    /** Threadsave version to get (a copy of) the m_done variable
     */

	bool GetDone();

    /** Threadsave version to set the m_run variable
     */

	bool GetStopRun();
	/** Threadsave version to set the m_stopRun variable
     */

    void SetRunNumber(unsigned int runnumber);

    /** Threadsave version to get (a copy of) the m_run variable
     */
    unsigned int GetRunNumber();

    /** Threadsave version to set the m_event variable
     */
    void SetEventNumber(unsigned int eventnumber);

    /** Threadsave version to get (a copy of) the m_event variable
     */
    unsigned int GetEventNumber();

    /** Threadsave version to get (a copy of) the m_event variable.
     *  After creating the copy m_event is increased before releasing the 
     *  mutex. The non-increased version is returned (like in the post-increment operator).
     */
    unsigned int GetIncreaseEventNumber();

	/** Threadsave way to get the status.
	 */
	timepix_producer_status_t GetRunStatusFlag();

	/** Threadsave way to set the status.
	 */
	void SetRunStatusFlag(timepix_producer_status_t status);

	/** Threadsave way to retreive the next commnad.
	 */
	timepix_producer_command_t PopCommand();

	/** Threadsave way to push the next commnad into the queue.
	 */
	void PushCommand(timepix_producer_command_t command);

	/** The protected member functions live in the communication thread.
     *  Make sure they do not perfom computing intensive tasks and return as
     *  soon as possible.
     */
protected:
    virtual void OnConfigure(const eudaq::Configuration & param);
    virtual void OnStartRun(unsigned param);
    virtual void OnStopRun();
    virtual void OnTerminate();
    virtual void OnReset();
    virtual void OnStatus();
    virtual void OnUnrecognised(const std::string & cmd, const std::string & param);

    // all data members have to be protected by mutex since they can be accessed by multiple 
    // threads
    bool m_done;		pthread_mutex_t m_done_mutex;
    unsigned m_run;		pthread_mutex_t m_run_mutex;
    unsigned m_ev;		pthread_mutex_t m_ev_mutex;
	pthread_mutexattr_t m_mutexattr;

	
private:
	CPixelmanProducerMFCDlg* pixelmanCtrl;
	timepix_producer_status_t m_status;
	pthread_mutex_t m_status_mutex;

	std::queue<timepix_producer_command_t> m_commandQueue;
	pthread_mutex_t m_commandQueue_mutex;
};
