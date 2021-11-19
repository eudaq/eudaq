#ifndef CMSPIXELPRODUCER_HH
#define CMSPIXELPRODUCER_HH

// EUDAQ includes:
#include "eudaq/Producer.hh"
#include "eudaq/Configuration.hh"

// pxarCore includes:
#include "api.h"

// system includes:
#include <chrono>
#include <iostream>
#include <ostream>
#include <vector>
#include <mutex>

// CMSPixelProducer
class CMSPixelProducer : public eudaq::Producer {

public:
  CMSPixelProducer(const std::string name, const std::string &runcontrol);
  void DoInitialise() override;
  void DoConfigure() override;
  void DoStartRun() override;
  void DoStopRun() override;
  void DoReset() override;
  void DoTerminate() override;
  void RunLoop() override;

  static const uint32_t m_id_factory = eudaq::cstr2hash("CMSPixelProducer");
private:
  void ReadInSingleEventWriteBinary();
  void ReadInSingleEventWriteASCII();
  void ReadInFullBufferWriteBinary();
  void ReadInFullBufferWriteASCII();

  // Helper function to read DACs from file which is provided via eudaq config:
  std::vector<std::pair<std::string, uint8_t>> GetConfDACs(eudaq::ConfigurationSPC config, int16_t i2c = -1,
                                                           bool tbm = false);
  std::vector<int32_t> &split(const std::string &s, char delim,
                              std::vector<int32_t> &elems);
  std::vector<int32_t> split(const std::string &s, char delim);

  std::vector<pxar::pixelConfig> GetConfMaskBits(eudaq::ConfigurationSPC config);
  std::vector<pxar::pixelConfig>
  GetConfTrimming(eudaq::ConfigurationSPC config, std::vector<pxar::pixelConfig> maskbits, int16_t i2c = -1);

  std::string prepareFilename(std::string filename, std::string n);

  unsigned m_run;
  unsigned m_tlu_waiting_time;
  unsigned m_roc_resetperiod;
  unsigned m_nplanes, m_channels;
  std::string m_verbosity, m_roctype, m_tbmtype, m_pcbtype,
      m_producerName, m_detector, m_event_type, m_alldacs;
  bool m_running, triggering;
  bool m_trimmingFromConf, m_trigger_is_pg;

  int shift_trigger_id{0};

  // Flag to send BORE:
  std::once_flag bore_flag_;

  // Add one mutex to protect calls to pxarCore:
  std::mutex m_mutex;
  pxar::pxarCore *m_api;

  int m_pattern_delay;

  std::chrono::steady_clock::time_point m_reset_timer;
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::Producer>::
    Register<CMSPixelProducer, const std::string&, const std::string&>(CMSPixelProducer::m_id_factory);
}

#endif /*CMSPIXELPRODUCER_HH*/
