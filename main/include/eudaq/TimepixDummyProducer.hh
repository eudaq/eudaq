#include "eudaq/Producer.hh"
#include <pthread.h>

class TimepixDummyProducer : public eudaq::Producer
{
  public:
    TimepixDummyProducer(const std::string & name, const std::string & runcontrol);
    virtual ~TimepixDummyProducer();

    void Event(unsigned short *timepixdata);
    void SimpleEvent();
    void BlobEvent();

    /** Threadsave version to set the m_done variable
     */
    void SetDone(bool done);

    /** Threadsave version to get (a copy of) the m_done variable
     */
    bool GetDone();

    /** Threadsave version to set the m_run variable
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
    bool m_done;       pthread_mutex_t m_done_mutex;
    unsigned m_run;  pthread_mutex_t m_run_mutex;
    unsigned m_ev;   pthread_mutex_t m_ev_mutex;

    pthread_mutexattr_t m_mutexattr;
};
