#include "eudaq/Producer.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Logger.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Configuration.hh"
#include "eudaq/Timer.hh"
#include "depfet/rc_depfet.hh"
#include "depfet/TCPclient.h"


#include <memory>


using eudaq::to_string;
using eudaq::RawDataEvent;

namespace {
  static const int BUFSIZE = 1128000;
  static const int REQUEST = 0x1013;
  static const unsigned BORE_TRIGGERID = 0x55555555;
  static const unsigned EORE_TRIGGERID = 0xAAAAAAAA;
}

class DEPFETProducerTCP : public eudaq::Producer {
public:
  DEPFETProducerTCP(const std::string & name, const std::string & runcontrol, const string typenm)
    : eudaq::Producer(name, runcontrol),
      done(false),
      datatypename(typenm),
      host_is_set(false),
      running(false),
      firstevent(false),
      last_trigger(0)
    {
      SetConnectionState(eudaq::ConnectionState::STATE_UNINIT, "Uninitialized");//
    }
  virtual void OnConfigure(const eudaq::Configuration & param) override final;
  virtual void OnStartRun(unsigned param) override final;
  virtual void OnStopRun() override final;
  virtual void OnTerminate() override final;
  virtual void OnReset() override final;
  void Process();
  bool done;
private:
  const string datatypename;
  bool host_is_set, running, firstevent;
  unsigned m_run, m_evt, m_idoffset;
  int cmd_port;
  unsigned last_trigger;
  std::string data_host, cmd_host;

  unsigned int buffer[BUFSIZE];
};
