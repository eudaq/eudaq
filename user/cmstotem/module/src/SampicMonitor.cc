#include "eudaq/Monitor.hh"
#include "eudaq/TTreeEventConverter.hh"

#include "TFile.h"
#include "TTree.h"

#include "MonitorWindow.hh"
#include "SampicEvent.hh"

#include "TApplication.h"
#include "TTimer.h"

#include <iostream>
#include <fstream>
#include <ratio>
#include <chrono>
#include <thread>
#include <random>

class SampicMonitor : public eudaq::Monitor, public TApplication {
public:
  SampicMonitor(const std::string & name, const std::string & runcontrol);
  ~SampicMonitor() override;
  void DoInitialise() override;
  void DoConfigure() override;
  void DoStartRun() override;
  void DoStopRun() override;
  void DoTerminate() override {}
  void DoReset() override {}
  void DoReceive(eudaq::EventSP ev) override;

  static const uint32_t m_id_factory = eudaq::cstr2hash("SampicMonitor");

private:
  std::unique_ptr<TFile> m_file;
  eudaq::TTreeEventSP m_stdev;
  bool m_en_print;
  bool m_en_std_converter;
  bool m_en_std_print;

  std::unique_ptr<MonitorWindow> m_main;
  std::future<bool> m_daemon;
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

  TApplication::SetReturnFromRun(true);
  if (!m_daemon.valid())
    m_daemon = std::async(std::launch::async, &SampicMonitor::Run, this);

  //m_main->Connect("CloseWindow()", "MainFrame", this, "~SampicMonitor()");
}

SampicMonitor::~SampicMonitor(){
  TApplication::Terminate();
}

void SampicMonitor::DoInitialise(){
  auto ini = GetInitConfiguration();
  ini->Print(std::cout);
}

void SampicMonitor::DoConfigure(){
  auto conf = GetConfiguration();
  //conf->Print(std::cout);
  m_en_print = conf->Get("SAMPIC_ENABLE_PRINT", 1);
  m_en_std_converter = conf->Get("SAMPIC_ENABLE_STD_CONVERTER", 0);
  m_en_std_print = conf->Get("SAMPIC_ENABLE_STD_PRINT", 0);
}

void SampicMonitor::DoStartRun(){
  m_main->SetRunNumber(GetRunNumber());
  m_file.reset(TFile::Open(Form("run%d.root", GetRunNumber()), "recreate"));
  m_stdev.reset(new TTree);
}

void SampicMonitor::DoReceive(eudaq::EventSP ev){
  if(m_en_print)
    ev->Print(std::cout);
  if(m_en_std_converter){
    eudaq::TTreeEventConverter::Convert(ev, m_stdev, nullptr); //no conf
    if (m_en_std_print)
      m_stdev->Print();//(std::cout);
  }
}

bool SampicMonitor::Run(){
  TApplication::Run(false);
  return true;
}

void SampicMonitor::DoStopRun(){
  m_stdev->Write("monitor");
  m_file->Close();
}
