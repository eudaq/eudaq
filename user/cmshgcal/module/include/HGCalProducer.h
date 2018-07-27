#ifndef HGCALPRODUCER_HH
#define HGCALPRODUCER_HH

#include "eudaq/Producer.hh"

#include <iostream>
#include <ostream>
#include <vector>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>

#include <boost/thread/thread.hpp>

#include "IpbusHwController.h"
#include "HGCalController.h"

namespace eudaq {
  
  class HGCalProducer: public eudaq::Producer
  {
  public:
    HGCalProducer(const std::string & name, const std::string & runcontrol);
    void DoInitialise() override final;
    void DoConfigure() override final;
    void DoStartRun() override final;
    void DoStopRun() override final;
    void DoReset() override final{;}
    void DoTerminate() override final;
    void RunLoop() override final;

    static const uint32_t m_id_factory = eudaq::cstr2hash("HGCalProducer");
  private:
    
    unsigned int m_run, m_eventN, m_triggerN, m_ev;
    bool m_doCompression;
    int m_compressionLevel;
    HGCalController *m_hgcController;
    boost::thread m_hgcCtrlThread;
    enum DAQState {
      STATE_ERROR,
      STATE_UNCONF,
      STATE_GOTOCONF,
      STATE_CONFED,
      STATE_GOTORUN,
      STATE_RUNNING,
      STATE_GOTOSTOP,
      STATE_GOTOTERM
    } m_state;
  };
}

#endif // HGCALPRODUCER_HH

