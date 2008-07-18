#include "eudaq/Producer.hh"
#include "eudaq/Utils.hh"
//#include "eudaq/DEPFETEvent.hh"
#include "depfet/rc_depfet.hh"
#include "depfet/TCPClient.h"

using eudaq::to_string;
//using eudaq::DEPFETEvent;

namespace {
  static const int BUFSIZE = 128000;
  static const int REQUEST = 0x1013;
  static const unsigned BORE_TRIGGERID = 0x55555555;
  static const unsigned EORE_TRIGGERID = 0xAAAAAAAA;
}

class DEPFETProducerTCP : public eudaq::Producer {
public:
  DEPFETProducerTCP(const std::string & name, const std::string & runcontrol)
    : eudaq::Producer(name, runcontrol),
      done(false)
    {
      //
    }
  virtual void OnConfigure(const eudaq::Configuration & param) {
    data_host = param.Get("DataHost", "");
    cmd_host = param.Get("CmdHost", "");
    cmd_port = param.Get("CmdPort", 32767);
    set_host(&cmd_host[0], cmd_port);
    SetStatus(eudaq::Status::LVL_OK, "Configured (" + param.Name() + ")");
  }
  virtual void OnStartRun(unsigned param) {
    m_run = param;
    m_evt = 0;
    cmd_send(1);
    SetStatus(eudaq::Status::LVL_OK, "Started");
  }
  virtual void OnStopRun() {
    eudaq::mSleep(1000);
    cmd_send(3);
    eudaq::mSleep(1000);
    SetStatus(eudaq::Status::LVL_OK, "Stopped");
  }
  virtual void OnTerminate() {
    std::cout << "Terminating..." << std::endl;
    done = true;
  }
  virtual void OnReset() {
    SetStatus(eudaq::Status::LVL_OK, "Reset");
  }
  void Process() {
    int lenevent;
    int Nmod, Kmod;
    unsigned int itrg, itrg_old == -1;
    do {   //--- modules of one event loop
      lenevent = BUFSIZE;
      Nmod = REQUEST;
      int rc = tcp_event_get(&data_host[0], buffer, &lenevent, &Nmod, &Kmod, &itrg);
      if (rc < 0) EUDAQ_WARN("tcp_event_get ERROR");
      int evt_type = (BUFFER[0] >> 22) & 0x3;
      if (itrg == BORE_TRIGGERID || itrg == EORE_TRIGGERID || itrg < itrg_old || evt_type != 2) {
        int evtModID = (buffer[0] >> 24) & 0xf;
        int len2 = buffer[0] & 0xfffff;
        int dev_type = (BUFFER[0] >> 28) & 0xf;
        std::cout << "Received: Mod " << (Kmod+1) << " of " << Nmod << ", id=" << evtModID
                  << ", EvType=" << evt_type << ", DevType=" << dev_type
                  << ", NData=" << LEVEVENT << " (" << len2 << ") "
                  << ", TrigID=" << itrg << " (" << buffer[1] << ")" << std::endl;
      }
    }  while (Kmod!=(Nmod-1));
    //--- here you have complete DEPFET event

    if (itrg == BORE_TRIGGERID) {
      SendEvent(DEPFETEvent::BORE(m_run, ++m_evt));
    } else if (itrg == EORE_TRIGGERID) {
      SendEvent(DEPFETEvent::EORE(m_run, ++m_evt));
    }

    if (evt_type != 0x2 || dev_type != 0x2) return;

    eudaq::DEPFETEvent ev(m_run, ++m_evt);
    ev.AddBoard(ModuleID, buffer, sizeof buffer);
    printf("Sending event \n");
    //SendEvent(ev);
    printf("OK \n");
  }
  bool done;
private:
  unsigned m_run, m_evt, cmd_port;
  std::string data_host, cmd_host;

  unsigned int buffer[BUFSIZE];
};
