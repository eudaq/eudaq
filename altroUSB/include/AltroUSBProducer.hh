#include "eudaq/Producer.hh"
#include <pthread.h>
#include <stdint.h>
#include "ilcdaq.h"

class AltroUSBProducer : public eudaq::Producer
{
  public:
    AltroUSBProducer(const std::string & name, const std::string & runcontrol);
    virtual ~AltroUSBProducer();

    enum Commands{ NONE = 0, START_RUN = 1 , STOP_RUN =2 , STATUS =3 , TERMINATE =4 , RESET=5 };

// Just send the plain chars
//    /** Send an recorded event from the physmem.
//     *  The length is the number of unsigned long words.
//     *  This function ensures little endian raw data. Use SendEvent to skip 
//     *  the conversion, which is faster (memory does not have to be allocated and
//     *  released).
//     */
//    void Event(std::uint16_t *u2fdata, unsigned int length);

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

    /** Threadsave version to get (a copy of) the m_runactive variable
     */
    bool GetRunActive();

    /** Threadsave version to set the m_runactive variable
     */
    void SetRunActive(bool activestatus);

    /** Threadsave version to get (a copy of) the m_event variable.
     *  After creating the copy m_event is increased before releasing the 
     *  mutex. The non-increased version is returned (like in the post-increment operator).
     */
    unsigned int GetIncreaseEventNumber();

    /** The main loop. This starts the Producer.
     *
     */
    void Exec();

    /** The protected member functions live in the communication thread.
     *  Make sure they do not perfom computing intensive tasks and return as
     *  soon as possible.
     */
protected:

    /** Configure the hardware. The configuration files are read in using Ulf's routines.
     *  Only the names of the config files are read from the eudaq config.
     */
    virtual void OnConfigure(const eudaq::Configuration & param);

    virtual void OnStartRun(unsigned param);
    virtual void OnStopRun();
    virtual void OnTerminate();
    virtual void OnReset();
    virtual void OnStatus();
    virtual void OnUnrecognised(const std::string & cmd, const std::string & param);

    // all data members have to be protected by mutex since they can be accessed by multiple 
    // threads
    /// status whether a data run is active							
    bool m_runactive; pthread_mutex_t m_runactive_mutex;
    /// status whether the confguration has been run
    bool  m_configured;// no need for a mutex, only accessed by the communication thread
    /// The run number
    unsigned m_run;  pthread_mutex_t m_run_mutex;
    /// The event number
    unsigned m_ev;   pthread_mutex_t m_ev_mutex;
    
    // Variables needed by the readout C library
    // Always lock the ilcdaq mutex before accessing them!

    DAQ* m_daq_config;     ///< the config of the daq
    unsigned int m_block_size; ///< size of the u2fdata memory block (number of bytes)
    unsigned char * m_data_block; ///< the memory block where the u2f dumps the data it reads

    /// A mutex to protect the non thread safe C routines and memory blocks
    //  Call this before accessing the library from Ulf
    pthread_mutex_t m_ilcdaq_mutex;

    /// The thread safe way to push a command to the command queue
    void CommandPush(Commands c);

    /** The thread safe way to pop a command fom the command queue. If the queue is emty
     *  it returns Commands::NONE.
     */
    Commands CommandPop();

/** As the command receiver runs in a separate thread we need a queue to ensure all 
      *  commands are executed. Do not access directly, only use the thread safe CommandPush() and
      *  CommandPop()!
      */
    std::queue<Commands> m_commandQueue;

    /// A mutex to protect the CommandQueue
    pthread_mutex_t m_commandqueue_mutex;
};
