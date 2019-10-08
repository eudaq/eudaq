#include "eudaq/ROOTMonitor.hh"
#include "eudaq/FileDeserializer.hh"
#include "eudaq/FileReader.hh"
#include "eudaq/DataConverter.hh"

#include "TSysEvtHandler.h"

#include <ratio>
#include <chrono>
#include <thread>

namespace eudaq {
  ROOTMonitor::ROOTMonitor(const std::string & name, const std::string & title, const std::string & runcontrol)
    :Monitor(name, runcontrol),
     m_app(new TApplication(name.c_str(), nullptr, nullptr)),
     m_monitor(new ROOTMonitorWindow(m_app.get(), title)){
    if (!m_monitor)
      EUDAQ_THROW("Error allocating main window");

    m_monitor->SetStatus(ROOTMonitorWindow::Status::idle);
    m_monitor->Connect("FillFromRAWFile(const char*)", NAME, this, "LoadRAWFileAsync(const char*)");
    m_monitor->Connect("WindowClosed()", NAME, this, "DoTerminate()");

    // launch the run loop
    m_app->SetReturnFromRun(true);
    if (!m_daemon.valid())
      m_daemon = std::async(std::launch::async, &TApplication::Run, m_app.get(), 0);
  }

  void ROOTMonitor::DoInitialise(){
    m_monitor->ResetCounters();
    AtInitialisation();
  }

  void ROOTMonitor::DoConfigure(){
    m_monitor->SetStatus(ROOTMonitorWindow::Status::configured);
    m_monitor->ResetCounters();
    AtConfiguration();
  }

  void ROOTMonitor::DoStartRun(){
    m_monitor->ResetCounters();
    m_monitor->SetStatus(ROOTMonitorWindow::Status::running);
    m_monitor->SetRunNumber(GetRunNumber());
    AtRunStart();
  }

  void ROOTMonitor::DoReceive(eudaq::EventSP ev){
    // update the counters
    m_monitor->SetCounters(ev->GetEventN(), ++m_num_evt_mon);
    AtEventReception(ev);
  }

  void ROOTMonitor::DoStopRun(){
    m_monitor->SetStatus(ROOTMonitorWindow::Status::configured);
    AtRunStop();
  }

  void ROOTMonitor::DoReset(){
    m_monitor->SetStatus(ROOTMonitorWindow::Status::idle);
    m_monitor->ResetCounters();
    AtReset();
  }

  void ROOTMonitor::DoTerminate(){
    if (!m_daemon_load.empty())
      m_daemon_load.clear();
    m_app->Terminate(1);
  }

  void ROOTMonitor::LoadRAWFileAsync(const char* path){
    if (!m_daemon_load.empty()) {
      EUDAQ_INFO(GetName()+" clearing the processing queue...");
      m_daemon_load.clear();
    }
    m_daemon_load.emplace_back(std::async(std::launch::async, &ROOTMonitor::LoadRAWFile, this, std::string(path)));
  }

  void ROOTMonitor::LoadRAWFile(const std::string& path){
    uint32_t id;
    unsigned long long num_evts = 0;
    bool first_evt = true;
    DoInitialise();
    DoConfigure();
    eudaq::EventUP ev;
    eudaq::FileDeserializer reader(path);
    do {
      reader.PreRead(id);
      ev = eudaq::Factory<eudaq::Event>::Create<eudaq::Deserializer&>(id, reader);
      eudaq::EventSP evt(std::move(ev));
      if (first_evt) {
        DoStartRun();
        m_monitor->SetRunNumber(evt->GetRunN());
        first_evt = false;
      }
      DoReceive(evt);
      if (num_evts > 0 && num_evts % 100000 == 0)
        EUDAQ_INFO(GetName()+" processed "+std::to_string(num_evts)+" events");
      num_evts++;
    } while (reader.HasData());
    EUDAQ_INFO(GetName()+" processed "+std::to_string(num_evts)+" events");
    m_monitor->Update();
    DoStopRun();
  }
}
