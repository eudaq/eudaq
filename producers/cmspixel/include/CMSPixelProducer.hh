#ifndef CMSPIXELPRODUCER_HH
#define CMSPIXELPRODUCER_HH

// EUDAQ includes:
#include "eudaq/Producer.hh"
#include "eudaq/Timer.hh"

// pxarCore includes:
#include "api.h"

// system includes:
#include <iostream>
#include <ostream>
#include <vector>

// CMSPixelProducer
class CMSPixelProducer : public eudaq::Producer {

public:
  CMSPixelProducer(const std::string & name, const std::string & runcontrol, const std::string & verbosity);
  virtual void OnConfigure(const eudaq::Configuration & config);
  virtual void OnStartRun(unsigned param);
  virtual void OnStopRun();
  virtual void OnTerminate();
  void ReadoutLoop();

private:
  void ReadInSingleEventWriteBinary();
  void ReadInSingleEventWriteASCII();
  void ReadInFullBufferWriteBinary();
  void ReadInFullBufferWriteASCII();

  std::vector<std::pair<std::string,uint8_t> > GetConfDACs();
  std::vector<pxar::pixelConfig> GetConfTrimming(std::string filename);

  unsigned m_run, m_ev;
  std::string m_verbosity, m_foutName, m_roctype, m_usbId, m_producerName, m_detector, m_event_type;
  bool stopping, done, started, triggering;
  bool m_dacsFromConf, m_trimmingFromConf;
  eudaq::Configuration m_config;
  pxar::pxarCore *m_api;
  int m_pattern_delay;
  uint8_t m_perFull;
  std::ofstream m_fout;
  eudaq::Timer* m_t;
};
#endif /*CMSPIXELPRODUCER_HH*/
