#include "eudaq/Monitor.hh"
#include "eudaq/TTreeEventConverter.hh"

#include "TFile.h"
#include "TTree.h"

#include "SampicEvent.hh"

#include "TApplication.h"
#include "TTimer.h"
#include "TGFrame.h"
#include "TGStatusBar.h"
#include "TGListTree.h"

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

  std::unique_ptr<TGMainFrame> m_main;
  TGStatusBar* m_status_bar;
  std::future<bool> m_daemon;
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::Monitor>::
    Register<SampicMonitor, const std::string&, const std::string&>(SampicMonitor::m_id_factory);
}

SampicMonitor::SampicMonitor(const std::string & name, const std::string & runcontrol)
  :eudaq::Monitor(name, runcontrol){
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

void SampicMonitor::DoStopRun(){
  m_stdev->Write("monitor");
  m_file->Close();
}
