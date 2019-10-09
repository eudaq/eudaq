#include "eudaq/ROOTMonitor.hh"
#include "eudaq/FileDeserializer.hh"
#include "eudaq/FileReader.hh"
#include "eudaq/DataConverter.hh"

#include "TH1.h"
#include "TGraph.h"

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

    m_monitor->SetStatus(eudaq::Status::STATE_UNINIT);
    m_monitor->Connect("FillFromRAWFile(const char*)", NAME, this, "LoadRAWFileAsync(const char*)");
    m_monitor->Connect("Quit()", NAME, this, "DoTerminate()");

    // launch the run loop
    m_app->SetReturnFromRun(true);
    if (!m_daemon.valid())
      m_daemon = std::async(std::launch::async, &TApplication::Run, m_app.get(), 0);
  }

  void ROOTMonitor::DoInitialise(){
    m_monitor->ResetCounters();
    AtInitialisation();
    m_glob_evt_reco_time = m_monitor->Book<TH1D>("Global/event_reco_time", "Event reconstruction time",
                                                 "event_reco_time", ";Event reconstruction time (ms)", 100, 0., 2.5);
    m_glob_evt_num_subevt = m_monitor->Book<TH1D>("Global/num_subevts", "Number of sub-events",
                                                  "num_subevts", ";Number of sub-events", 10, 0., 10.);
    m_monitor->SetDrawOptions(m_glob_evt_num_subevt, "hist text0");
    m_glob_evt_vs_ts = m_monitor->Book<TGraph>("Global/evt_vs_ts", "Event timestamp");
    m_glob_evt_vs_ts->SetTitle(";Time (s);Event ID");
    m_glob_rate_vs_ts = m_monitor->Book<TGraph>("Global/rate_vs_ts", "Rate evolution");
    m_glob_rate_vs_ts->SetTitle(";Time (s);Event rate (Hz)");
  }

  void ROOTMonitor::DoConfigure(){
    m_monitor->SetStatus(eudaq::Status::STATE_CONF);
    m_monitor->ResetCounters();
    AtConfiguration();
  }

  void ROOTMonitor::DoStartRun(){
    m_monitor->ResetCounters();
    m_monitor->SetStatus(eudaq::Status::STATE_RUNNING);
    m_monitor->SetRunNumber(GetRunNumber());
    AtRunStart();
  }

  void ROOTMonitor::DoReceive(eudaq::EventSP ev){
    auto start = std::chrono::system_clock::now();
    // update the counters
    m_monitor->SetCounters(ev->GetEventN(), ++m_num_evt_mon);
    // user-specific filling part
    AtEventReception(ev);
    // global event information
    std::chrono::duration<double> elapsed_sec = std::chrono::system_clock::now()-start;
    m_glob_evt_reco_time->Fill(elapsed_sec.count()*1.e3);
    m_glob_evt_num_subevt->Fill(ev->GetNumSubEvent());
    if (ev->GetTimestampBegin() != 0) {
      m_glob_evt_vs_ts->SetPoint(m_glob_evt_vs_ts->GetN(), ev->GetTimestampBegin(), ev->GetEventID());
      const double rate = (ev->GetTimestampBegin() != m_glob_last_evt_ts)
        ? 1./(ev->GetTimestampBegin()-m_glob_last_evt_ts) : 0.;
      m_glob_rate_vs_ts->SetPoint(m_glob_rate_vs_ts->GetN(), ev->GetTimestampBegin(), rate);
      m_glob_last_evt_ts = ev->GetTimestampBegin();
    }
  }

  void ROOTMonitor::DoStopRun(){
    m_monitor->SetStatus(eudaq::Status::STATE_STOPPED);
    AtRunStop();
  }

  void ROOTMonitor::DoReset(){
    m_monitor->SetStatus(eudaq::Status::STATE_UNINIT);
    m_monitor->ResetCounters();
    AtReset();
  }

  void ROOTMonitor::DoTerminate(){
    m_interrupt = true;
    if (!m_daemon_load.empty())
      m_daemon_load.clear();
    m_app->Terminate(1);
  }

  void ROOTMonitor::LoadRAWFileAsync(const char* path){
    EUDAQ_INFO(GetName()+" will load \""+std::string(path)+"\".");
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
      if (m_interrupt)
        return;
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
