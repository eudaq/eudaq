//
// Producer for the ALICE pALPIDEfs chip
// Author: Jan.Fiete.Grosse-Oetringhaus@cern.ch
//

#include "eudaq/Producer.hh"

#include "eudaq/Mutex.hh"
#include "eudaq/EudaqThread.hh"
#include <queue>

#include <tinyxml.h>

// pALPIDEfs driver
#include "TTestsetup.h"

struct SingleEvent {
  SingleEvent(unsigned int length, uint64_t trigger_id, uint64_t timestamp) : m_buffer(0), m_length(length), m_trigger_id(trigger_id), m_timestamp(timestamp) {
    m_buffer = new unsigned char[length];
  }
  ~SingleEvent() {
    delete m_buffer;
    m_buffer = 0;
  }
  unsigned char* m_buffer;
  unsigned int m_length;
  uint64_t m_trigger_id;
  uint64_t m_timestamp;
};

class SimpleLock {
  public:
    SimpleLock(eudaq::Mutex& mutex) : m_mutex(mutex) {
      mutex.Lock();
    }
    ~SimpleLock() {
      m_mutex.UnLock();
    }
  protected:
    eudaq::Mutex& m_mutex;
};

class DeviceReader {
  public:
    DeviceReader(int id, int debuglevel, TDAQBoard* daq_board, TpAlpidefs* dut);
    ~DeviceReader() { }
    
    void SetMaxQueueSize(unsigned long size) { m_max_queue_size = size; }
    void SetQueueFullDelay(int delay) { m_queuefull_delay = delay; }
    
    void Stop();
    void SetRunning(bool running);
    SingleEvent* NextEvent();
    void DeleteNextEvent();
    SingleEvent* PopNextEvent();
    void PrintQueueStatus();
    static void* LoopWrapper(void* arg);
    
    TDAQBoard* GetDAQBoard() { return m_daq_board; }
    
    float GetTemperature();
  
    void ParseXML(TiXmlNode* node, int base, int rgn, bool readwrite);

  protected:
    void Loop();
    void Print(const char* text);
    bool Disconnect();
    bool IsStopping() {  SimpleLock lock(m_mutex); return m_stop; }
    bool IsRunning() {   SimpleLock lock(m_mutex); return m_running; }
    bool IsFlushing() {   SimpleLock lock(m_mutex); return m_flushing; }
    void SetStopping() { SimpleLock lock(m_mutex); m_stop = true; }
    
    void Push(SingleEvent* ev);
    bool QueueFull();
    
    std::queue<SingleEvent*> m_queue;
    unsigned long m_queue_size;
    eudaq::eudaqThread m_thread;
    eudaq::Mutex m_mutex;
    bool m_stop;
    bool m_running;
    bool m_flushing;
    int m_id;
    int m_debuglevel;
    uint64_t m_last_trigger_id;

    TDAQBoard* m_daq_board;
    TpAlpidefs* m_dut;
    
    // config
    int m_queuefull_delay; // milliseconds
    unsigned long m_max_queue_size; // queue size in B
};

class PALPIDEFSProducer : public eudaq::Producer {
  public:
    PALPIDEFSProducer(const std::string & name, const std::string & runcontrol, int debuglevel = 0)
      : eudaq::Producer(name, runcontrol), m_run(0), m_ev(0), m_done(false), m_running(false), m_flush(false), m_configured(false), m_reader(0), m_next_event(0), m_debuglevel(debuglevel), m_testsetup(0), m_mutex(), m_nDevices(0), m_status_interval(-1), m_full_config() {}

    virtual void OnConfigure(const eudaq::Configuration & param);
    virtual void OnStartRun(unsigned param);
    virtual void OnStopRun();
    virtual void OnTerminate();
    virtual void OnReset();
    virtual void OnStatus() { }
    virtual void OnUnrecognised(const std::string & cmd, const std::string & param);
    
    void Loop();
    
  protected:
    int BuildEvent();
    void SendEOR();
    void SendStatusEvent();
    void PrintQueueStatus();
    
    bool IsRunning() {   SimpleLock lock(m_mutex); return m_running; }
    bool IsFlushing() {  SimpleLock lock(m_mutex); return m_flush; }
    bool IsDone() {  SimpleLock lock(m_mutex); return m_done; }

    unsigned m_run, m_ev;
    bool m_done;
    bool m_running;
    bool m_flush;
    bool m_configured;
    DeviceReader** m_reader;
    SingleEvent** m_next_event;
    int m_debuglevel;
    
    eudaq::Mutex m_mutex;
    TTestSetup* m_testsetup;
    
    // config
    int m_nDevices;
    int m_status_interval;
    std::string m_full_config;
};
