//
// Producer for the ALICE pALPIDEfs chip
// Author: Jan.Fiete.Grosse-Oetringhaus@cern.ch
//

#include "eudaq/Producer.hh"

#include <mutex>
#include <thread>
#include <queue>

#include <tinyxml.h>

// pALPIDEfs driver
#include "TTestsetup.h"

struct SingleEvent {
  SingleEvent(unsigned int length, uint64_t trigger_id, uint64_t timestamp, uint64_t timestamp_reference)
      : m_buffer(0), m_length(length), m_trigger_id(trigger_id),
        m_timestamp(timestamp), m_timestamp_corrected(timestamp), m_timestamp_reference(timestamp_reference) {
    m_buffer = new unsigned char[length];
  }
  ~SingleEvent() {
    delete[] m_buffer;
    m_buffer = 0;
  }
  unsigned char* m_buffer;
  unsigned int m_length;
  uint64_t m_trigger_id;
  uint64_t m_timestamp;
  uint64_t m_timestamp_corrected;
  uint64_t m_timestamp_reference;
};

class SimpleLock {
public:
  SimpleLock(std::mutex &mutex) : m_mutex(mutex) { mutex.lock(); }
  ~SimpleLock() { m_mutex.unlock(); }

protected:
  std::mutex &m_mutex;
};

class DeviceReader {
public:
  DeviceReader(int id, int debuglevel, TTestSetup* test_setup, int boardid,
               TDAQBoard* daq_board, TpAlpidefs* dut);
  ~DeviceReader() {}

  void SetMaxQueueSize(unsigned long size) { m_max_queue_size = size; }
  void SetQueueFullDelay(int delay) { m_queuefull_delay = delay; }

  void Stop();
  void SetRunning(bool running);
  void StartDAQ();
  void StopDAQ();
  SingleEvent* NextEvent();
  void DeleteNextEvent();
  SingleEvent* PopNextEvent();
  void PrintQueueStatus();
  int GetQueueLength() {
    SimpleLock lock(m_mutex);
    return m_queue.size();
  }

  static void* LoopWrapper(void* arg);

  TDAQBoard* GetDAQBoard() { return m_daq_board; }
  TpAlpidefs* GetDUT() { return m_dut; }

  float GetTemperature();

  void ParseXML(TiXmlNode* node, int base, int rgn, bool readwrite);

  void RequestThresholdScan() {
    SimpleLock lock(m_mutex);
    m_threshold_scan_result = 0;
    m_threshold_scan_rqst = true;
  }
  int GetThresholdScanState() {
    SimpleLock lock(m_mutex);
    return m_threshold_scan_result;
  }
  void SetupThresholdScan(int NMaskStage, int NEvts, int ChStart, int ChStop,
                          int ChStep, unsigned char*** Data,
                          unsigned char* Points);

  bool IsWaitingForEOR() {
    SimpleLock lock(m_mutex);
    return m_waiting_for_eor;
  }

  bool IsReading() {
    SimpleLock lock(m_mutex);
    return m_reading;
  }

protected:
  void Loop();
  void Print(int level, const char* text, uint64_t value1 = -1,
             uint64_t value2 = -1, uint64_t value3 = -1, uint64_t value4 = -1);
  bool IsStopping() {
    SimpleLock lock(m_mutex);
    return m_stop;
  }
  bool IsRunning() {
    SimpleLock lock(m_mutex);
    return m_running;
  }
  void SetReading(bool reading) {
    SimpleLock lock(m_mutex);
    m_reading = reading;
  }
  bool IsFlushing() {
    SimpleLock lock(m_mutex);
    return m_flushing;
  }
  void SetStopping() {
    SimpleLock lock(m_mutex);
    m_stop = true;
  }
  bool IsThresholdScanRqsted() {
    SimpleLock lock(m_mutex);
    return m_threshold_scan_rqst;
  }

  void Push(SingleEvent* ev);
  bool QueueFull();

  bool ThresholdScan();

  void PrepareMaskStage(TAlpidePulseType APulseType, int AMaskStage, int steps);

  std::queue<SingleEvent* > m_queue;
  unsigned long m_queue_size;
  std::thread m_thread;
  std::mutex m_mutex;
  bool m_stop;
  bool m_running;
  bool m_flushing;
  bool m_reading;
  bool m_waiting_for_eor;
  bool m_threshold_scan_rqst;
  int m_threshold_scan_result; // 0 = not running, 1 = running, 2 = error, 3 =
                               // success
  int m_id;
  int m_boardid; // id of the DAQ board as used by TTestSetup::GetDAQBoard()...
  int m_debuglevel;
  uint64_t m_last_trigger_id;

  TTestSetup* m_test_setup;
  TDAQBoard* m_daq_board;
  TpAlpidefs* m_dut;
  int m_daq_board_header_length;
  int m_daq_board_trailer_length;

  // config
  int m_queuefull_delay;          // milliseconds
  unsigned long m_max_queue_size; // queue size in B

  // S-Curve scan
  int m_n_mask_stages;
  int m_n_events;
  int m_ch_start;
  int m_ch_stop;
  int m_ch_step;
  unsigned char*** m_data;
  unsigned char* m_points;
};

class PALPIDEFSProducer : public eudaq::Producer {
public:
  PALPIDEFSProducer(const std::string &name, const std::string &runcontrol,
                    int debuglevel = 0)
      : eudaq::Producer(name, runcontrol), m_run(0), m_ev(0),
        m_timestamp_reference(0x0), m_done(false),
        m_running(false), m_stopping(false), m_flush(false),
        m_configured(false), m_firstevent(false), m_reader(0), m_next_event(0),
        m_debuglevel(debuglevel), m_testsetup(0), m_mutex(), m_nDevices(0),
        m_status_interval(-1), m_full_config_v1(), m_full_config_v2(),
        m_full_config_v3(), m_ignore_trigger_ids(true),
        m_recover_outofsync(true), m_chip_type(0x0),
        m_strobe_length(0x0), m_strobeb_length(0x0), m_trigger_delay(0x0),
        m_readout_delay(0x0), m_chip_readoutmode(0x0),
        m_monitor_PSU(false), m_back_bias_voltage(-1),
        m_dut_pos(-1.), m_dut_angle1(-1.), m_dut_angle2(-1.),
        m_SCS_charge_start(-1), m_SCS_charge_stop(-1),
        m_SCS_charge_step(-1), m_SCS_n_events(-1), m_SCS_n_mask_stages(-1),
        m_SCS_n_steps(-1), m_do_SCS(0x0), m_SCS_data(0x0), m_SCS_points(0x0) {}
  ~PALPIDEFSProducer() { PowerOffTestSetup(); }

  virtual void OnConfigure(const eudaq::Configuration &param);
  virtual void OnStartRun(unsigned param);
  virtual void OnStopRun();
  virtual void OnTerminate();
  virtual void OnReset();
  virtual void OnStatus() {}
  virtual void OnUnrecognised(const std::string &cmd, const std::string &param);

  void Loop();

protected:
  bool InitialiseTestSetup(const eudaq::Configuration &param);
  bool PowerOffTestSetup();
  bool DoSCurveScan(const eudaq::Configuration &param);
  void SetBackBiasVoltage(const eudaq::Configuration &param);
  void ControlLinearStage(const eudaq::Configuration &param);
  void ControlRotaryStages(const eudaq::Configuration &param);
  bool ConfigChip(int id, TpAlpidefs *dut, std::string configFile);
  int BuildEvent();
  void SendEOR();
  void SendStatusEvent();
  void PrintQueueStatus();
  void PrepareMaskStage(TAlpidePulseType APulseType, int AMaskStage,
                        int nPixels, int*** Data);

  bool IsRunning() {
    SimpleLock lock(m_mutex);
    return m_running;
  }
  bool IsStopping() {
    SimpleLock lock(m_mutex);
    return m_stopping;
  }
  bool IsFlushing() {
    SimpleLock lock(m_mutex);
    return m_flush;
  }
  bool IsDone() {
    SimpleLock lock(m_mutex);
    return m_done;
  }
  bool IsConfiguring() {
    SimpleLock lock(m_mutex);
    return m_configuring;
  }

  unsigned m_run, m_ev;
  uint64_t *m_timestamp_reference;
  bool m_done;
  bool m_running;
  bool m_stopping;
  bool m_configuring;
  bool m_flush;
  bool m_configured;
  bool m_firstevent;
  DeviceReader** m_reader;
  SingleEvent** m_next_event;
  int m_debuglevel;


  std::mutex m_mutex;
  TTestSetup* m_testsetup;

  // config
  int m_nDevices;
  int m_status_interval;
  std::string m_full_config_v1;
  std::string m_full_config_v2;
  std::string m_full_config_v3;
  bool m_ignore_trigger_ids;
  bool m_recover_outofsync;
  int* m_chip_type;
  int* m_strobe_length;
  int* m_strobeb_length;
  int* m_trigger_delay;
  int* m_readout_delay;
  int* m_chip_readoutmode;
  bool m_monitor_PSU;
  float m_back_bias_voltage;
  float m_dut_pos;
  float m_dut_angle1;
  float m_dut_angle2;
  // S-Curve scan settings
  int m_SCS_charge_start;
  int m_SCS_charge_stop;
  int m_SCS_charge_step;
  int m_SCS_n_events;
  int m_SCS_n_mask_stages;
  int m_SCS_n_steps;
  bool* m_do_SCS;

  // S-Curve scan output data
  unsigned char**** m_SCS_data;
  unsigned char** m_SCS_points;
};
