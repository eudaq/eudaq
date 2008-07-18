#include "eudaq/Producer.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Logger.hh"
#include "eudaq/DEPFETEvent.hh"
#include "depfet/rc_depfet.hh"
#include "depfet/TCPClient.h"

using eudaq::to_string;
using eudaq::DEPFETEvent;

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
      done(false),
      host_is_set(false),
      running(false)
    {
      //
    }
  virtual void OnConfigure(const eudaq::Configuration & param) {
    data_host = param.Get("DataHost", "silab22a");
    if (host_is_set) {
      if (cmd_host != param.Get("CmdHost", "silab22a") ||
          cmd_port != param.Get("CmdPort", 32767)) {
        SetStatus(eudaq::Status::LVL_ERROR, "Command Host changed");
        EUDAQ_THROW("Changing DEPFET command host is not supported");
      }
    } else {
      cmd_host = param.Get("CmdHost", "silab22a");
      cmd_port = param.Get("CmdPort", 32767);
      set_host(&cmd_host[0], cmd_port);
      host_is_set = true;
    }
    SetStatus(eudaq::Status::LVL_OK, "Configured (" + param.Name() + ")");
  }
  virtual void OnStartRun(unsigned param) {
    m_run = param;
    m_evt = 0;
    cmd_send(1);
    running = true;
    SetStatus(eudaq::Status::LVL_OK, "Started");
  }
  virtual void OnStopRun() {
    eudaq::mSleep(1000);
    cmd_send(3);
    eudaq::mSleep(1000);
    running = false;
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
    if (!running || data_host == "") {
      usleep(10);
      return;
    }
    int lenevent;
    int Nmod, Kmod;
    unsigned int itrg, itrg_old = -1;
    eudaq::DEPFETEvent ev(m_run, m_evt+1);
    do {   //--- modules of one event loop
      lenevent = BUFSIZE;
      Nmod = REQUEST;
      int rc = tcp_event_get(&data_host[0], buffer, &lenevent, &Nmod, &Kmod, &itrg);
      if (rc < 0) EUDAQ_WARN("tcp_event_get ERROR");
      int evtModID = (buffer[0] >> 24) & 0xf;
      int len2 = buffer[0] & 0xfffff;
      int evt_type = (buffer[0] >> 22) & 0x3;
      int dev_type = (buffer[0] >> 28) & 0xf;
      if (itrg == BORE_TRIGGERID || itrg == EORE_TRIGGERID || itrg < itrg_old || evt_type != 2) {
        std::cout << "Received: Mod " << (Kmod+1) << " of " << Nmod << ", id=" << evtModID
                  << ", EvType=" << evt_type << ", DevType=" << dev_type
                  << ", NData=" << lenevent << " (" << len2 << ") "
                  << ", TrigID=" << itrg << " (" << buffer[1] << ")" << std::endl;
      }

      if (itrg == BORE_TRIGGERID) {
        printf("Sending BORE \n");
        SendEvent(DEPFETEvent::BORE(m_run));
        return;
      } else if (itrg == EORE_TRIGGERID) {
        printf("Sending EORE \n");
        SendEvent(DEPFETEvent::EORE(m_run, ++m_evt));
        return;
      }

      if (evt_type != 2 /*|| dev_type != 2*/) continue;

      ev.AddBoard(evtModID, buffer, lenevent*4);

    }  while (Kmod!=(Nmod-1));

    printf("Sending event \n");
    ++m_evt;
    SendEvent(ev);
    printf("OK \n");
  }
  bool done;
private:
  bool host_is_set, running;
  unsigned m_run, m_evt;
  int cmd_port;
  std::string data_host, cmd_host;

  unsigned int buffer[BUFSIZE];
};
