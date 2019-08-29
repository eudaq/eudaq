#include "MonitorWindow.hh"

#include "TApplication.h"
#include "TTimer.h"
#include "TRootEmbeddedCanvas.h"

#include <iostream>
#include <fstream>

MonitorWindow::MonitorWindow(TApplication* par, const std::string& name)
  :TGMainFrame(gClient->GetRoot(), 800, 600, kVerticalFrame), m_parent(par){
  SetWindowName(name.c_str());

  m_top_win = new TGHorizontalFrame(this);
  m_left_bar = new TGVerticalFrame(m_top_win);
  m_left_canv = new TGCanvas(m_left_bar, 200, 600);

  auto vp = m_left_canv->GetViewPort();

  auto left_tree = new TGListTree(m_left_canv, kHorizontalFrame);
  //left_tree->Connect("Clicked(TGListTreeItem*, Int_t)", NAME, this,
  //                   "actor(TGListTreeItem*, Int_t)");
  //left_tree->Connect("Clicked(TGListTreeItem*, Int_t, Int_t, Int_t)",
  //                   NAME, this,
  //                   "actorMenu(TGListTreeItem*, Int_t, Int_t, Int_t)");
  vp->AddFrame(left_tree, new TGLayoutHints(kLHintsExpandY | kLHintsExpandY, 5, 5, 5, 5));
  m_top_win->AddFrame(m_left_bar, new TGLayoutHints(kLHintsExpandY, 2, 2, 2, 2));
  m_left_bar->AddFrame(m_left_canv, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 5, 5, 5, 5));
 // this->AddFrame(m_top_win, new TGLayoutHints(kLHintsExpandY | kLHintsExpandX | kLHintsLeft, 0, 0, 0, 0));


  auto right_frame = new TGVerticalFrame(m_top_win);
  auto right_canv = new TRootEmbeddedCanvas("Canvas", right_frame);
  right_frame->AddFrame(right_canv, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 5, 5, 5, 5));
  //m_top_win->AddFrame(right_frame, new TGLayoutHints(kLHintsExpandY, 2, 2, 2, 2));
  m_top_win->AddFrame(right_frame, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 2, 2, 2, 2));
  this->AddFrame(m_top_win, new TGLayoutHints(kLHintsExpandY | kLHintsExpandX | kLHintsLeft, 0, 0, 0, 0));

  int status_parts[4] = {20, 20, 30, 30};
  m_status_bar = new TGStatusBar(this, 510, 10, kHorizontalFrame);
  m_status_bar->SetParts(status_parts, 4);
  m_status_bar->SetText("IDLE", 0);
  m_status_bar->SetText("Run: N/A", 1);
  m_status_bar->SetText("Curr. event: ", 2);
  m_status_bar->SetText("Analysed events: ", 3);

  this->AddFrame(m_status_bar, new TGLayoutHints(kLHintsBottom | kLHintsExpandX, 0, 0, 2, 0));
  //left_bar->MapSubwindows();

  auto timer = new TTimer();
  timer->Connect("Timeout()", NAME, this, "Update()");
  timer->Start(1000, kFALSE); // update automatically every second

  std::cout << __PRETTY_FUNCTION__ << std::endl;
  this->MapSubwindows();
  this->MapWindow();
  //this->Connect("CloseWindow()", NAME, this, "Quit()");
}

MonitorWindow::~MonitorWindow(){
  m_parent->Terminate();
}

void MonitorWindow::SetRunNumber(int run){
  m_status_bar->SetText(Form("Run: %u", run), 1);
}

void MonitorWindow::Update(){
  std::cout << "Update..." << std::endl;
}

/*void MonitorWindow::Quit(){
  std::cout <<__PRETTY_FUNCTION__<<std::endl;
  //m_parent->Terminate();
}*/
