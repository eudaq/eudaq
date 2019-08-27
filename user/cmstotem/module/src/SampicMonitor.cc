#include "eudaq/Monitor.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/StdEventConverter.hh"

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
  void DoStopRun() override {}
  void DoTerminate() override {}
  void DoReset() override {}
  void DoReceive(eudaq::EventSP ev) override;

  static const uint32_t m_id_factory = eudaq::cstr2hash("SampicMonitor");

private:
  bool Run();

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
  :eudaq::Monitor(name, runcontrol),
   TApplication(name.c_str(), nullptr, nullptr),
   m_main(new TGMainFrame(gClient->GetRoot(), 800, 600)){
  if (!m_main)
    EUDAQ_THROW("Error Allocationg main window");
  m_main->MapWindow();
  m_main->SetWindowName("Sampic monitor");
  std::cout << __PRETTY_FUNCTION__ << std::endl;
  TApplication::SetReturnFromRun(true);
  if (!m_daemon.valid())
    m_daemon = std::async(std::launch::async, &SampicMonitor::Run, this);

  auto top_win = new TGHorizontalFrame(m_main.get());
  auto left_bar = new TGVerticalFrame(top_win);
  auto left_canv = new TGCanvas(left_bar, 200, 600);
  auto vp = left_canv->GetViewPort();

  auto left_tree = new TGListTree(left_canv, kHorizontalFrame);
  //left_tree->Connect("Clicked(TGListTreeItem*, Int_t)", "OnlineMonWindow", this,
  //                   "actor(TGListTreeItem*, Int_t)");
  //left_tree->Connect("Clicked(TGListTreeItem*, Int_t, Int_t, Int_t)",
  //                   "OnlineMonWindow", this,
  //                   "actorMenu(TGListTreeItem*, Int_t, Int_t, Int_t)");
  vp->AddFrame(left_tree, new TGLayoutHints(kLHintsExpandY | kLHintsExpandY, 5, 5, 5, 5));


  m_status_bar = new TGStatusBar(m_main.get(), 510, 10, kHorizontalFrame);
  int parts[4] = {25, 25, 25, 25};
  m_status_bar->SetParts(parts, 4);
  m_status_bar->SetText("IDLE", 0);
  m_status_bar->SetText("Run: N/A", 1);
  m_status_bar->SetText("Curr. event: ", 2);
  m_status_bar->SetText("Analysed events: ", 3);
  m_main->AddFrame(m_status_bar, new TGLayoutHints(kLHintsBottom | kLHintsExpandX, 0, 0, 2, 0));
}

SampicMonitor::~SampicMonitor(){
  if (m_daemon.valid())
    m_daemon.get();
  TApplication::Terminate();
}

void SampicMonitor::DoStartRun(){
  m_status_bar->SetText(Form("Run: %u", GetRunNumber()), 1);
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

void SampicMonitor::DoReceive(eudaq::EventSP ev){
  if(m_en_print)
    ev->Print(std::cout);
  if(m_en_std_converter){
    auto stdev = std::dynamic_pointer_cast<eudaq::SampicEvent>(ev);
    if(!stdev){
      stdev = eudaq::SampicEvent::MakeShared();
      eudaq::StdEventConverter::Convert(ev, stdev, nullptr); //no conf
    }
    if(m_en_std_print)
      stdev->Print(std::cout);
  }
}

bool SampicMonitor::Run(){
  TApplication::Run();
  return true;
}
