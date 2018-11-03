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
  virtual void OnConfigure(const eudaq::Configuration & param) override final {
    m_idoffset = param.Get("IDOffset", 6);
    data_host = param.Get("DataHost", "silab22a");
    if (host_is_set) {
      if (cmd_host != param.Get("CmdHost", "silab22a") ||
          cmd_port != param.Get("CmdPort", 32767)) {
          std::cout<<"Old host >>"<<cmd_host<<"<<, new host >>"<<param.Get("CmdHost", "silab22a")<<"<< ";
          std::cout<<"Old port "<<cmd_port<<" new port"<<param.Get("CmdPort", 32767)<<std::endl;
        SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Command Host changed");
        EUDAQ_ERROR("Changing DEPFET command host is not supported");
        return;
      }
    } else {
      cmd_host = param.Get("CmdHost", "silab22a");
      cmd_port = param.Get("CmdPort", 32767);
      set_host(&cmd_host[0], cmd_port);
      host_is_set = true;
    }
    cmd_send("CMD STOP\n");
    eudaq::mSleep(1000);
    cmd_send("CMD STATUS\n");
    eudaq::mSleep(1000);
    SetConnectionState(eudaq::ConnectionState::STATE_CONF, "Configured (" + param.Name() + ")");
  }
  virtual void OnStartRun(unsigned param) override final {
    cmd_send("CMD STOP\n");
    eudaq::mSleep(1000);
    m_run = param;
    m_evt = 0;
    cmd_send("CMD EVB RUNNUM " + to_string(m_run) + "\n");
    eudaq::mSleep(1000);  
    SendEvent(RawDataEvent::BORE(datatypename, m_run));
    cmd_send("CMD START\n");
    firstevent = true;
    running = true;
    SetConnectionState(eudaq::ConnectionState::STATE_RUNNING, "Started");
  }
  virtual void OnStopRun() override final {
    eudaq::mSleep(1000);
    cmd_send("CMD STOP\n");
    eudaq::mSleep(1000);
    running = false;
    SetConnectionState(eudaq::ConnectionState::STATE_CONF, "Stopped");
  }
  virtual void OnTerminate() override final {
    std::cout << "Terminating..." << std::endl;
    done = true;
  }
  virtual void OnReset() override final {
    SetConnectionState(eudaq::ConnectionState::STATE_UNCONF, "Reset");
  }
  void Process() {
    eudaq::Timer timer;
    if (!running || data_host == "") {
      usleep(10);
      return;
    }
    int lenevent;
    int Nmod, Kmod;
    unsigned int itrg;
    //eudaq::DEPFETEvent ev(m_run, m_evt+1);
    std::shared_ptr<eudaq::RawDataEvent> ev;
    unsigned id = m_idoffset;
    do {   //--- modules of one event loop
      lenevent = BUFSIZE;
      Nmod = REQUEST;
      eudaq::Timer timer2;
      int rc = tcp_event_get(&data_host[0], buffer, &lenevent, &Nmod, &Kmod, &itrg);
      if (itrg%400 == 0) std::cout << "##DEBUG## tcp_event_get " << timer2.mSeconds() << "ms" << std::endl;
      if (rc < 0) {
          EUDAQ_WARN("tcp_event_get ERROR");
          SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Could not read from host!");
          done=true;
      }
      int evtModID = (buffer[0] >> 24) & 0xf;
      int len2 = buffer[0] & 0xfffff;
      int evt_type = (buffer[0] >> 22) & 0x3;
      int dev_type = (buffer[0] >> 28) & 0xf;
      if (itrg == BORE_TRIGGERID || itrg == EORE_TRIGGERID || itrg < last_trigger || evt_type != 2) {
        std::cout << "Received: Mod " << (Kmod+1) << " of " << Nmod << ", id=" << evtModID
                  << ", EvType=" << evt_type << ", DevType=" << dev_type
                  << ", NData=" << lenevent << " (" << len2 << ") "
                  << ", TrigID=" << itrg << " (" << buffer[1] << ") last trigger "<<last_trigger << std::endl;
      }
      last_trigger=itrg;

      if (itrg == BORE_TRIGGERID) {
        //if (dev_type == 2) {
        //  printf("Sending BORE \n");
         // SendEvent(RawDataEvent::BORE(datatypename, m_run));
        //}
        //printf("DEBUG: BORE\n");
        return;
      } else if (itrg == EORE_TRIGGERID) {
        printf("Sending EORE \n");
        SendEvent(RawDataEvent::EORE(datatypename, m_run, ++m_evt));
        return;
      }

      if (evt_type != 2 /*|| dev_type != 2*/) {
        //printf("DEBUG: ignoring evt_type %d\n", evt_type);
        continue;
      }
      if (!ev) {
        //ev = new eudaq::RawDataEvent("DEPFET", m_run, itrg); 
        ev = make_shared<eudaq::RawDataEvent>(datatypename, m_run, m_evt); // -- fsv:: send local counter instead TLU number
      }
      ev->AddBlock(id++, buffer, lenevent*4);

    }  while (Kmod!=(Nmod-1));
    if (itrg%100 == 0) std::cout << "##DEBUG## Reading took " << timer.mSeconds() << "ms" << std::endl;
    timer.Restart();
//    if (firstevent && itrg != 0) {
//      printf("Ignoring bad event (%d)\n", itrg);
//      firstevent = false;
//      return;
//    }
    ++m_evt;
    SendEvent(*ev);
    if (itrg%400 == 0) std::cout << "##DEBUG## Sending took " << timer.mSeconds() << "ms" << std::endl;
  }
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
