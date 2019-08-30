#include "eudaq/Monitor.hh"
#include "eudaq/TTreeEventConverter.hh"

#include "TFile.h"
#include "TTree.h"
#include "TH1.h"
#include "TGraph.h"

#include "MonitorWindow.hh"
#include "SampicEvent.hh"

#include "TApplication.h"

#include <iostream>
#include <fstream>
#include <ratio>
#include <chrono>
#include <thread>
#include <random>

class SampicMonitor : public eudaq::Monitor, public TApplication {
public:
  SampicMonitor(const std::string & name, const std::string & runcontrol);
  void DoInitialise() override;
  void DoConfigure() override;
  void DoStartRun() override;
  void DoStopRun() override;
  void DoTerminate() override;
  void DoReset() override;
  void DoReceive(eudaq::EventSP ev) override;

  static const uint32_t m_id_factory = eudaq::cstr2hash("SampicMonitor");

private:
  std::unique_ptr<TFile> m_file;
  eudaq::TTreeEventSP m_stdev;

  bool m_en_print;
  bool m_en_std_converter;
  bool m_en_std_print;

  std::unique_ptr<MonitorWindow> m_main;
  std::future<void> m_daemon;
  std::future<void> m_test_daemon;

  TGraph* m_g_trg_time;
  void test(TH1D* h){h->Fill(rand()/RAND_MAX*10.);usleep(1000);}
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::Monitor>::
    Register<SampicMonitor, const std::string&, const std::string&>(SampicMonitor::m_id_factory);
}

SampicMonitor::SampicMonitor(const std::string & name, const std::string & runcontrol)
  :eudaq::Monitor(name, runcontrol),
   TApplication(name.c_str(), nullptr, nullptr),
   m_main(new MonitorWindow(this, "Sampic monitor")){
  if (!m_main)
    EUDAQ_THROW("Error Allocationg main window");
  std::cout << __PRETTY_FUNCTION__ << std::endl;

  m_main->SetStatus(MonitorWindow::Status::idle);

  TApplication::SetReturnFromRun(true);
  if (!m_daemon.valid())
    m_daemon = std::async(std::launch::async, &TApplication::Run, this, 0);

  //m_main->Book<TH1D,eudaq::Event>([](TH1D* h, eudaq::Event* ev)->void{h->Fill(ev->GetTriggerN());}, "/test", "test_hist", "testing;x;y", 100, 0., 10.);
  auto hist = m_main->Book<TH1D>("/test", "test_hist", "testing;x;y", 100, 0., 10.);
  m_g_trg_time = m_main->Book<TGraph>("/trig_vs_time");
  if (!m_test_daemon.valid())
    m_test_daemon = std::async(std::launch::async, &SampicMonitor::test, this, hist);
}

void SampicMonitor::DoInitialise(){
  auto ini = GetInitConfiguration();
  ini->Print(std::cout);
  m_main->ResetCounters();
}

void SampicMonitor::DoConfigure(){
  auto conf = GetConfiguration();
  //conf->Print(std::cout);
  m_en_print = conf->Get("SAMPIC_ENABLE_PRINT", 1);
  m_en_std_converter = conf->Get("SAMPIC_ENABLE_STD_CONVERTER", 0);
  m_en_std_print = conf->Get("SAMPIC_ENABLE_STD_PRINT", 0);
  m_main->SetStatus(MonitorWindow::Status::configured);
  m_main->ResetCounters();
}

void SampicMonitor::DoStartRun(){
  m_main->SetRunNumber(GetRunNumber());
  m_file.reset(TFile::Open(Form("run%d.root", GetRunNumber()), "recreate"));
  m_stdev.reset(new TTree);
  m_main->SetStatus(MonitorWindow::Status::running);
  m_main->SetTree(m_stdev.get());
}

void SampicMonitor::DoReceive(eudaq::EventSP ev){
  if(m_en_print)
    ev->Print(std::cout);

  auto event = std::make_shared<eudaq::SampicEvent>(*ev);

  m_main->Get<TH1D>("/test")->Fill(ev->GetEventN());
  m_g_trg_time->SetPoint(m_g_trg_time->GetN(), event->header().sf2Timestamp(), ev->GetTriggerN());
  if(m_en_std_converter){
    eudaq::TTreeEventConverter::Convert(ev, m_stdev, nullptr); //no conf
    if (m_en_std_print)
      m_stdev->Print();//(std::cout);
  }
}

void SampicMonitor::DoStopRun(){
  m_stdev->Write("monitor");
  m_file->Close();
  m_main->SetStatus(MonitorWindow::Status::configured);
}

void SampicMonitor::DoReset(){
  m_main->SetStatus(MonitorWindow::Status::idle);
  m_main->ResetCounters();
}

void SampicMonitor::DoTerminate(){
  TApplication::Terminate(1);
}
