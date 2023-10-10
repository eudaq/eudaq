#include "eudaq/ROOTMonitorWindow.hh"

#include <TApplication.h>
#include <TCanvas.h>
#include <TGFileDialog.h>
#include <TObjString.h>

#include <iostream>

namespace eudaq {
  ROOTMonitorWindow::ROOTMonitorWindow(TApplication *par,
                                       const std::string &name)
      : TGMainFrame(gClient->GetRoot(), 800, 600, kVerticalFrame),
        ROOTMonitorBaseWindow(par, name) {
    SetName(name.c_str());
    SetWindowName(name.c_str());

    m_top_win = new TGHorizontalFrame(this);
    m_left_bar = new TGVerticalFrame(m_top_win);
    m_left_canv = new TGCanvas(m_left_bar, 200, 600);

    auto vp = m_left_canv->GetViewPort();

    m_tree_list = new TGListTree(m_left_canv, kHorizontalFrame);
    m_tree_list->Connect("DoubleClicked(TGListTreeItem*, Int_t)", NAME, this,
                         "DrawElement(TGListTreeItem*, Int_t)");
    m_tree_list->Connect("Clicked(TGListTreeItem*, Int_t, Int_t, Int_t)", NAME,
                         this,
                         "DrawMenu(TGListTreeItem*, Int_t, Int_t, Int_t)");
    vp->AddFrame(m_tree_list, new TGLayoutHints(kLHintsExpandY | kLHintsExpandY,
                                                5, 5, 5, 5));
    m_top_win->AddFrame(m_left_bar,
                        new TGLayoutHints(kLHintsExpandY, 2, 2, 2, 2));
    m_left_bar->AddFrame(
        m_left_canv,
        new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 5, 5, 5, 5));

    auto right_frame = new TGVerticalFrame(m_top_win);

    // toolbar
    m_toolbar = new TGToolBar(right_frame, 180, 80);
    right_frame->AddFrame(m_toolbar,
                          new TGLayoutHints(kLHintsExpandX, 2, 2, 2, 2));

    m_button_open = new TGPictureButton(m_toolbar, m_icon_open);
    m_button_open->SetToolTipText(
        "Load monitoring information from a RAW/ROOT file");
    m_button_open->SetEnabled(true);
    m_button_open->Connect("Clicked()", NAME, this, "OpenFileDialog()");
    m_toolbar->AddFrame(m_button_open,
                        new TGLayoutHints(kLHintsLeft, 2, 1, 0, 0));

    m_button_save = new TGPictureButton(m_toolbar, m_icon_save);
    m_button_save->SetToolTipText("Save all monitors");
    m_button_save->SetEnabled(false);
    m_button_save->Connect("Clicked()", NAME, this, "SaveFileDialog()");
    m_toolbar->AddFrame(m_button_save,
                        new TGLayoutHints(kLHintsLeft, 2, 1, 0, 0));

    m_button_clean = new TGPictureButton(m_toolbar, m_icon_del);
    m_button_clean->SetToolTipText("Clean all monitors");
    m_button_clean->SetEnabled(false);
    m_button_clean->Connect("Clicked()", NAME, this, "ClearMonitors()");
    m_toolbar->AddFrame(m_button_clean,
                        new TGLayoutHints(kLHintsLeft, 1, 2, 0, 0));

    m_update_toggle = new TGCheckButton(m_toolbar, "&Update", 1);
    m_toolbar->AddFrame(m_update_toggle,
                        new TGLayoutHints(kLHintsLeft, 2, 2, 2, 2));
    m_update_toggle->SetToolTipText(
        "Switch on/off the auto refresh of all monitors");
    m_update_toggle->Connect("Toggled(Bool_t)", NAME, this,
                             "SwitchUpdate(Bool_t)");

    m_refresh_toggle = new TGCheckButton(m_toolbar, "&Clear between runs", 1);
    m_toolbar->AddFrame(m_refresh_toggle,
                        new TGLayoutHints(kLHintsLeft, 2, 2, 2, 2));
    m_refresh_toggle->SetToolTipText("Clear all monitors between two runs");
    m_refresh_toggle->Connect("Toggled(Bool_t)", NAME, this,
                              "SwitchClearRuns(Bool_t)");

    // main canvas
    m_main_canvas = new TRootEmbeddedCanvas("Canvas", right_frame);
    right_frame->AddFrame(
        m_main_canvas,
        new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 5, 5, 5, 5));
    m_top_win->AddFrame(
        right_frame,
        new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 2, 2, 2, 2));
    this->AddFrame(m_top_win, new TGLayoutHints(
                                  kLHintsExpandY | kLHintsExpandX | kLHintsLeft,
                                  0, 0, 0, 0));

    // status bar
    int status_parts[(int)StatusBarPos::num_parts] = {20, 10, 35, 35};
    m_status_bar = new TGStatusBar(this, 510, 10, kHorizontalFrame);
    m_status_bar->SetParts(status_parts, (int)StatusBarPos::num_parts);
    ResetCounters();

    this->AddFrame(
        m_status_bar,
        new TGLayoutHints(kLHintsBottom | kLHintsExpandX, 0, 0, 2, 0));

    m_update_toggle->SetOn();
    m_refresh_toggle->SetOn();
    SwitchUpdate(true);

    this->MapSubwindows();
    this->Layout();
    this->MapSubwindows();
    this->MapWindow();
    m_context_menu = new TContextMenu("", "");
  }

  //--- counters/status bookeeping

  void ROOTMonitorWindow::ResetCounters() {
    if (m_status_bar) {
      m_status_bar->SetText("Run: N/A", (int)StatusBarPos::run_number);
      m_status_bar->SetText("Curr. event: N/A", (int)StatusBarPos::tot_events);
      m_status_bar->SetText("Analysed events: N/A",
                            (int)StatusBarPos::an_events);
    }
    m_button_save->SetEnabled(false);
    m_button_clean->SetEnabled(false);
    ROOTMonitorBaseWindow::ResetCounters();
  }

  void ROOTMonitorWindow::SetRunNumber(int run) {
    m_run_number = run;
    if (m_status_bar)
      m_status_bar->SetText(Form("Run: %u", m_run_number),
                            (int)StatusBarPos::run_number);
    m_button_save->SetEnabled(true);
    m_button_clean->SetEnabled(true);
  }

  void ROOTMonitorWindow::SetLastEventNum(int num) {
    ROOTMonitorBaseWindow::SetLastEventNum(num);
    if (m_status_bar && (num < 0 || m_status == Status::STATE_RUNNING))
      m_status_bar->SetText(Form("Curr. event: %llu", m_last_event),
                            (int)StatusBarPos::tot_events);
  }

  void ROOTMonitorWindow::SetMonitoredEventsNum(int num) {
    ROOTMonitorBaseWindow::SetMonitoredEventsNum(num);
    if (m_status_bar && (num < 0 || m_status == Status::STATE_RUNNING))
      m_status_bar->SetText(Form("Analysed events: %llu", m_last_event_mon),
                            (int)StatusBarPos::an_events);
  }

  void ROOTMonitorWindow::SetStatus(Status::State st) {
    m_status = st;
    if (m_status_bar)
      m_status_bar->SetText(Status::State2String(st).c_str(),
                            (int)StatusBarPos::status);
  }

  //--- file I/O

  void ROOTMonitorWindow::OpenFileDialog() {
    static TString dir(".");
    // allows for both RAW and post-processed ROOT files (TBrowser-like)
    const char *filetypes[] = {"RAW files", "*.raw", "ROOT files",
                               "*.root",    0,       0};
    TGFileInfo fi;
    fi.fFileTypes = filetypes;
    fi.fIniDir = StrDup(dir);
    new TGFileDialog(gClient->GetRoot(), this, kFDOpen, &fi); // handled by GC
    if (fi.fMultipleSelection && fi.fFileNamesList) // only grab first file
      LoadFile(dynamic_cast<TObjString *>(fi.fFileNamesList->First())
                   ->GetString()
                   .Data());
    else if (fi.fFilename != nullptr)
      LoadFile(fi.fFilename);
  }

  void ROOTMonitorWindow::SaveFileDialog() {
    TGFileInfo fi;
    // first define the output file
    static TString dir(".");
    const char *filetypes[] = {"ROOT files", "*.root", 0, 0};
    fi.fFileTypes = filetypes;
    fi.fIniDir = StrDup(dir);
    new TGFileDialog(gClient->GetRoot(), this, kFDSave, &fi);
    if (fi.fFilename != nullptr)
      SaveFile(fi.fFilename);
  }

  void ROOTMonitorWindow::LoadFile(const char *filename) {
    // disable control over update
    m_update_toggle->SetEnabled(false);
    m_refresh_toggle->SetEnabled(false);
    ROOTMonitorBaseWindow::LoadFile(filename);
    // filter by extension
    TString s_ext(filename);
    s_ext.Remove(0, s_ext.Last('.') + 1).ToLower();
    if (s_ext == "root")
      m_button_save->SetEnabled(
          false); // nothing has changed, export feature disabled
    else if (s_ext == "raw")
      m_button_save->SetEnabled(true); // allows for export
    else
      m_button_clean->SetEnabled(false);
  }

  //--- graphical part

  void ROOTMonitorWindow::Update() {
    ROOTMonitorBaseWindow::Update();
    // check if we have something to draw
    if (m_drawable.empty())
      return;
    TCanvas *canv = m_main_canvas->GetCanvas();
    if (!canv) { // failed to retrieve the plotting region
      std::cerr << "[WARNING] Failed to retrieve the main plotting canvas!"
                << std::endl;
      return;
    }
    Draw(canv);
    PostDraw(canv);
  }

  void ROOTMonitorWindow::MapCanvas() {
    if (!m_left_canv)
      return;
    m_left_canv->MapSubwindows();
    m_left_canv->MapWindow();
  }

  void ROOTMonitorWindow::Draw(TCanvas *canv) {
    canv->cd();
    if (m_canv_needs_refresh) { // book the pads and monitors placeholders
      canv->Clear();
      if (m_drawable.size() > 1) {
        int ncol = ceil(sqrt(m_drawable.size()));
        int nrow = ceil(m_drawable.size() * 1. / ncol);
        canv->Divide(ncol, nrow);
      }
      for (size_t i = 0; i < m_drawable.size(); ++i) {
        auto &dr = m_drawable.at(i);
        auto pad = canv->cd(i + 1);
        dr->object->Draw(dr->draw_opt);
        pad->SetTicks();
        pad->SetGrid();
      }
      canv->SetTicks();
      canv->SetGrid();
    }
    ROOTMonitorBaseWindow::Draw();
  }

  void ROOTMonitorWindow::PostDraw(TCanvas *canv) {
    // canvas(es) flushing procedure
    canv->Modified();
    canv->Update();
    ROOTMonitorBaseWindow::PostDraw();
    for (size_t i = 0; i < m_drawable.size(); ++i)
      if (auto pad = canv->GetPad(i + 1)) {
        pad->Update();
        pad->Modified();
      }
  }

  //--- monitoring elements helpers

  void ROOTMonitorWindow::DrawMenu(TGListTreeItem *it, int but, int x, int y) {
    if (but == 3)
      m_context_menu->Popup(x, y, this);
  }
} // namespace eudaq
