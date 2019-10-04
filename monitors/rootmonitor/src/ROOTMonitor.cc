#include "eudaq/ROOTMonitor.hh"

#include <ratio>
#include <chrono>
#include <thread>

namespace eudaq {
  ROOTMonitor::ROOTMonitor(const std::string & name, const std::string & title, const std::string & runcontrol)
    :Monitor(name, runcontrol),
     TApplication(name.c_str(), nullptr, nullptr),
     m_monitor(new ROOTMonitorWindow(this, title)){
    if (!m_monitor)
      EUDAQ_THROW("Error Allocationg main window");

    m_monitor->SetStatus(ROOTMonitorWindow::Status::idle);

    // launch the run loop
    TApplication::SetReturnFromRun(true);
    if (!m_daemon.valid())
      m_daemon = std::async(std::launch::async, &TApplication::Run, this, 0);
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
    TApplication::Terminate(1);
  }
}
