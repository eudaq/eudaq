#include "eudaq/Monitor.hh"
#include "eudaq/Logger.hh"

#define EUDAQ_MAX_EVENTS_PER_IDLE 32

namespace eudaq {

  Monitor::Monitor(const std::string & name, const std::string & runcontrol, const std::string & datafile) :
    CommandReceiver("Monitor", name, runcontrol, false),
    m_run(0),
    m_datafile(datafile),
    m_callstart(false)
  {
    if (datafile != "") {
      // set offline
      if (datafile.find_first_not_of("0123456789") == std::string::npos) {
        m_run = from_string(datafile, 0);
      }
      m_callstart = true;
      //OnStartRun(m_run);
    }
    StartThread();
  }

  bool Monitor::ProcessEvent() {
    //std::cout << "processevent" << std::endl;
    if (!m_ser.get()) return false;
    if (!m_ser->HasData()) return false;
    try {
      Event * event = EventFactory::Create(*m_ser);
      counted_ptr<DetectorEvent> dev(dynamic_cast<DetectorEvent *>(event));
      if (dev.get()) {
        if (dev->IsBORE()) m_lastbore = dev;
        OnEvent(dev);
      } else {
        OnBadEvent(counted_ptr<Event>(event));
      }
    } catch (const InterruptedException &) {
      return false;
    }
    return true;
  }

  void Monitor::OnIdle() {
    if (m_callstart) {
      m_callstart = false;
      OnStartRun(m_run);
    }
    bool processed = false;
    for (int i = 0; i < EUDAQ_MAX_EVENTS_PER_IDLE; ++i) {
      if (ProcessEvent()) {
        processed = true;
      } else {
        //if (offline) OnTerminate();
        break;
      }
    }
    if (!processed) mSleep(1);
  }

  void Monitor::OnStartRun(unsigned param) {
    m_run = param;
    if (m_run > 0) m_datafile = "data/run" + to_string(m_run, 6) + ".raw";
    m_ser = counted_ptr<FileDeserializer>(new FileDeserializer(m_datafile));
    EUDAQ_INFO("Starting run " + to_string(m_run) + ", file=" + m_datafile);
  }

  void Monitor::OnStopRun() {
    m_ser->Interrupt();
  }

}
