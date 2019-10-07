#include "eudaq/ROOTMonitor.hh"
#include "eudaq/FileDeserializer.hh"
#include "eudaq/FileReader.hh"
#include "eudaq/DataConverter.hh"

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
    m_monitor->Connect("FillFromRAWFile(const char*)", NAME, this, "LoadRAWFile(const char*)");

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
    m_app->Terminate(1);
  }

  void ROOTMonitor::LoadRAWFile(const char* path){
    DoInitialise();
    DoConfigure();
    eudaq::FileDeserializer reader(path);
    eudaq::EventUP ev;
    uint32_t id;
    unsigned long long num_evts = 0;
    while (reader.HasData()) {
      reader.PreRead(id);
      ev = eudaq::Factory<eudaq::Event>::Create<eudaq::Deserializer&>(id, reader);
      if (num_evts > 0 && num_evts % 10000 == 0)
        std::cout << "Processed " << num_evts << " events" << std::endl;
      DoReceive(std::make_shared<eudaq::Event>(*ev));
      num_evts++;
    }
  }
}
