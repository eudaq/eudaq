#include "eudaq/Monitor.hh"
#include "eudaq/Logger.hh"
#include "eudaq/PluginManager.hh"

#define EUDAQ_MAX_EVENTS_PER_IDLE 1000

namespace eudaq {

  Monitor::Monitor(const std::string & name, const std::string & runcontrol, const std::string & datafile) :
    CommandReceiver("Monitor", name, runcontrol, false),
    m_run(0),
    m_callstart(false),
    m_reader(0)
  {
    if (datafile != "") {
      // set offline
      m_reader = counted_ptr<FileReader>(new FileReader(datafile));
      PluginManager::Initialize(m_reader->GetDetectorEvent()); // process BORE
      //m_callstart = true;
      std::cout << "DEBUG: Reading file " << datafile << " -> " << m_reader->Filename() << std::endl;
      //OnStartRun(m_run);
    }
    StartThread();
  }

  bool Monitor::ProcessEvent() {
    //std::cout << "processevent" << std::endl;
    if (!m_reader.get()) return false;
    if (!m_reader->NextEvent()) return false;
    if (m_reader->GetDetectorEvent().GetEventNumber() % 1000 == 0) {
      std::cout << "ProcessEvent " << m_reader->GetDetectorEvent().GetEventNumber()
                << (m_reader->GetDetectorEvent().IsBORE() ? "B" : m_reader->GetDetectorEvent().IsEORE() ? "E" : "")
                << std::endl;
    }
    try {
      const DetectorEvent & dev = m_reader->GetDetectorEvent();
      if (dev.IsBORE()) m_lastbore = counted_ptr<DetectorEvent>(new DetectorEvent(dev));
      OnEvent(PluginManager::ConvertToStandard(dev));
    } catch (const InterruptedException &) {
      return false;
    }
    return true;
  }

  void Monitor::OnIdle() {
    //std::cout << "..." << std::endl;
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
    std::cout << "run " << param << std::endl;
    m_run = param;
    m_reader = counted_ptr<FileReader>(new FileReader(to_string(m_run)));
    PluginManager::Initialize(m_reader->GetDetectorEvent()); // process BORE
    EUDAQ_INFO("Starting run " + to_string(m_run));
  }

  void Monitor::OnStopRun() {
    m_reader->Interrupt();
  }

}
