#ifndef CMSPIXELPRODUCER_HH
#define CMSPIXELPRODUCER_HH

// EUDAQ includes:
#include "eudaq/Producer.hh"
#include "eudaq/Timer.hh"
#include "eudaq/Configuration.hh"

// pxarCore includes:
#include "api.h"

// system includes:
#include <iostream>
#include <ostream>
#include <vector>
#include <mutex>

// CMSPixelProducer
class CMSPixelProducer : public eudaq::Producer {

public:
  CMSPixelProducer(const std::string & name, const std::string & runcontrol, const std::string & verbosity);
  virtual void OnConfigure(const eudaq::Configuration & config);
  virtual void OnStartRun(unsigned runnumber);
  virtual void OnStopRun();
  virtual void OnTerminate();
  void ReadoutLoop();

private:
  void ReadInSingleEventWriteBinary();
  void ReadInSingleEventWriteASCII();
  void ReadInFullBufferWriteBinary();
  void ReadInFullBufferWriteASCII();

  // Helper function to read DACs from the eudaq config object:
  std::vector<std::pair<std::string,uint8_t> > GetConfDACs();
  // Helper function to read DACs from file which is provided via eudaq config:
  std::vector<std::pair<std::string,uint8_t> > GetConfDACs(std::string filename);

  std::vector<pxar::pixelConfig> GetConfTrimming(std::string filename);

  unsigned m_run, m_ev, m_ev_filled, m_ev_runningavg_filled;
  unsigned m_tlu_waiting_time;
  std::string m_verbosity, m_foutName, m_roctype, m_pcbtype, m_usbId, m_producerName, m_detector, m_event_type, m_alldacs;
  bool m_terminated, m_running, triggering;
  bool m_dacsFromConf, m_trimmingFromConf, m_trigger_is_pg;
  eudaq::Configuration m_config;

  // Add one mutex to protect calls to pxarCore:
  std::mutex m_mutex;
  pxar::pxarCore *m_api;

  int m_pattern_delay;
  uint8_t m_perFull;
  std::ofstream m_fout;
  eudaq::Timer* m_t;
};
#endif /*CMSPIXELPRODUCER_HH*/
