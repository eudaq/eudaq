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
  SingleEvent(unsigned int length, uint64_t trigger_id, uint64_t timestamp) : m_buffer(0), m_length(length), m_trigger_id(trigger_id), m_timestamp(timestamp), m_timestamp_corrected(timestamp) {
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
  uint64_t m_timestamp_corrected;
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
    DeviceReader(int id, int debuglevel, TTestSetup* test_setup, TDAQBoard* daq_board, TpAlpidefs* dut);
    ~DeviceReader() { }

    void SetMaxQueueSize(unsigned long size) { m_max_queue_size = size; }
    void SetQueueFullDelay(int delay) { m_queuefull_delay = delay; }
    void SetHighRateMode(bool flag) { m_high_rate_mode = flag; }
    void SetReadoutMode(int mode) { m_readout_mode = mode; }

    void Stop();
    void SetRunning(bool running);
    SingleEvent* NextEvent();
    void DeleteNextEvent();
    SingleEvent* PopNextEvent();
    void PrintQueueStatus();
    static void* LoopWrapper(void* arg);

    TDAQBoard* GetDAQBoard() { return m_daq_board; }
    TpAlpidefs* GetDUT() { return m_dut; }

    float GetTemperature();

    void ParseXML(TiXmlNode* node, int base, int rgn, bool readwrite);

    bool ThresholdScan(int NMaskStage, int NEvts, int ChStart, int ChStop, int ChStep, int ***Data, int *Points);

  protected:
    void Loop();
    void Print(int level, const char* text, uint64_t value1 = -1, uint64_t value2 = -1, uint64_t value3 = -1, uint64_t value4 = -1);
    bool Disconnect();
    bool IsStopping() {  SimpleLock lock(m_mutex); return m_stop; }
    bool IsRunning() {   SimpleLock lock(m_mutex); return m_running; }
    bool IsFlushing() {   SimpleLock lock(m_mutex); return m_flushing; }
    void SetStopping() { SimpleLock lock(m_mutex); m_stop = true; }

    void Push(SingleEvent* ev);
    bool QueueFull();

    void PrepareMaskStage(TAlpidePulseType APulseType, int AMaskStage, int nPixels = 1);

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

    TTestSetup* m_test_setup;
    TDAQBoard* m_daq_board;
    TpAlpidefs* m_dut;

    // config
    int m_queuefull_delay; // milliseconds
    unsigned long m_max_queue_size; // queue size in B
    bool m_high_rate_mode; // decides if is is checked if data is available before requesting an event
    bool m_readout_mode;
};

class PALPIDEFSProducer : public eudaq::Producer {
  public:
    PALPIDEFSProducer(const std::string & name, const std::string & runcontrol, int debuglevel = 0)
      : eudaq::Producer(name, runcontrol), m_run(0), m_ev(0), m_done(false), m_running(false), m_flush(false), m_configured(false), m_firstevent(false), m_reader(0), m_next_event(0), m_debuglevel(debuglevel), m_testsetup(0), m_mutex(), m_nDevices(0), m_status_interval(-1), m_full_config(), m_ignore_trigger_ids(true), m_recover_outofsync(true), m_readout_mode(0), m_back_bias_voltage(-1), m_strobe_length(0x0), m_strobeb_length(0x0), m_trigger_delay(0x0), m_readout_delay(0x0), m_SCS_charge_start(-1), m_SCS_charge_stop(-1), m_SCS_charge_step(-1), m_SCS_n_events(-1), m_SCS_n_mask_stages(-1), m_do_SCS(0x0), m_SCS_hitmap(0x0), m_SCS_mask(0x0) {}

    virtual void OnConfigure(const eudaq::Configuration & param);
    virtual void OnStartRun(unsigned param);
    virtual void OnStopRun();
    virtual void OnTerminate();
    virtual void OnReset();
    virtual void OnStatus() { }
    virtual void OnUnrecognised(const std::string & cmd, const std::string & param);

    void Loop();

  protected:
    bool ConfigChip(int id, TDAQBoard* daq_board, std::string configFile);
    int BuildEvent();
    void SendEOR();
    void SendStatusEvent();
    void PrintQueueStatus();
    void PrepareMaskStage(TAlpidePulseType APulseType, int AMaskStage, int nPixels);

    bool IsRunning() {   SimpleLock lock(m_mutex); return m_running; }
    bool IsFlushing() {  SimpleLock lock(m_mutex); return m_flush; }
    bool IsDone() {  SimpleLock lock(m_mutex); return m_done; }

    unsigned m_run, m_ev;
    bool m_done;
    bool m_running;
    bool m_flush;
    bool m_configured;
    bool m_firstevent;
    DeviceReader** m_reader;
    SingleEvent** m_next_event;
    int m_debuglevel;

    eudaq::Mutex m_mutex;
    TTestSetup* m_testsetup;

    // config
    int m_nDevices;
    int m_status_interval;
    std::string m_full_config;
    bool m_ignore_trigger_ids;
    bool m_recover_outofsync;
    bool m_readout_mode;
    int* m_strobe_length;
    int* m_strobeb_length;
    int* m_trigger_delay;
    int* m_readout_delay;
    float m_back_bias_voltage;
    // S-Curve scan settings
    int m_SCS_charge_start;
    int m_SCS_charge_stop;
    int m_SCS_charge_step;
    int m_SCS_n_events;
    int m_SCS_n_mask_stages;
    bool* m_do_SCS;

    // S-Curve scan output data
    int *****m_SCS_hitmap;
    int *****m_SCS_mask;
};
