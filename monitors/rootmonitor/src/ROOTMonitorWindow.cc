#include "eudaq/ROOTMonitorWindow.hh"

#include "TApplication.h"
#include "TTimer.h"
#include "TFile.h"
#include "TGFileDialog.h"
#include "TCanvas.h"
#include "TKey.h"
#include "TObjString.h"

// required for object-specific "clear"
#include "TGraph.h"
#include "TGraph2D.h"
#include "TH1.h"
#include "TH2.h"
#include "TMultiGraph.h"

#include <iostream>
#include <fstream>
#include <sstream>

namespace eudaq {
  ROOTMonitorWindow::ROOTMonitorWindow(TApplication* par, const std::string& name)
    :TGMainFrame(gClient->GetRoot(), 800, 600, kVerticalFrame), m_parent(par),
     m_icon_save(gClient->GetPicture("bld_save.xpm")),
     m_icon_del(gClient->GetPicture("bld_delete.xpm")),
     m_icon_open(gClient->GetPicture("bld_open.xpm")),
     m_icon_th1(gClient->GetPicture("h1_t.xpm")),
     m_icon_th2(gClient->GetPicture("h2_t.xpm")),
     m_icon_tprofile(gClient->GetPicture("profile_t.xpm")),
     m_icon_track(gClient->GetPicture("eve_track.xpm")),
     m_icon_summ(gClient->GetPicture("draw_t.xpm")),
     m_timer(new TTimer(1000, kTRUE)){
    SetName(name.c_str());
    SetWindowName(name.c_str());

    m_top_win = new TGHorizontalFrame(this);
    m_left_bar = new TGVerticalFrame(m_top_win);
    m_left_canv = new TGCanvas(m_left_bar, 200, 600);

    auto vp = m_left_canv->GetViewPort();

    m_tree_list = new TGListTree(m_left_canv, kHorizontalFrame);
    m_tree_list->Connect("DoubleClicked(TGListTreeItem*, Int_t)", NAME, this,
                         "DrawElement(TGListTreeItem*, Int_t)");
    m_tree_list->Connect("Clicked(TGListTreeItem*, Int_t, Int_t, Int_t)",
                         NAME, this,
                         "DrawMenu(TGListTreeItem*, Int_t, Int_t, Int_t)");
    vp->AddFrame(m_tree_list, new TGLayoutHints(kLHintsExpandY | kLHintsExpandY, 5, 5, 5, 5));
    m_top_win->AddFrame(m_left_bar, new TGLayoutHints(kLHintsExpandY, 2, 2, 2, 2));
    m_left_bar->AddFrame(m_left_canv, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 5, 5, 5, 5));

    auto right_frame = new TGVerticalFrame(m_top_win);

    // toolbar
    m_toolbar = new TGToolBar(right_frame, 180, 80);
    right_frame->AddFrame(m_toolbar, new TGLayoutHints(kLHintsExpandX, 2, 2, 2, 2));

    m_button_open = new TGPictureButton(m_toolbar, m_icon_open);
    m_button_open->SetToolTipText("Load monitoring information from a RAW/ROOT file");
    m_button_open->SetEnabled(true);
    m_button_open->Connect("Clicked()", NAME, this, "OpenFileDialog()");
    m_toolbar->AddFrame(m_button_open, new TGLayoutHints(kLHintsLeft, 2, 1, 0, 0));

    m_button_save = new TGPictureButton(m_toolbar, m_icon_save);
    m_button_save->SetToolTipText("Save all monitors");
    m_button_save->SetEnabled(false);
    m_button_save->Connect("Clicked()", NAME, this, "SaveFileDialog()");
    m_toolbar->AddFrame(m_button_save, new TGLayoutHints(kLHintsLeft, 2, 1, 0, 0));

    m_button_clean = new TGPictureButton(m_toolbar, m_icon_del);
    m_button_clean->SetToolTipText("Clean all monitors");
    m_button_clean->SetEnabled(false);
    m_button_clean->Connect("Clicked()", NAME, this, "ClearMonitors()");
    m_toolbar->AddFrame(m_button_clean, new TGLayoutHints(kLHintsLeft, 1, 2, 0, 0));

    m_update_toggle = new TGCheckButton(m_toolbar, "&Update", 1);
    m_toolbar->AddFrame(m_update_toggle, new TGLayoutHints(kLHintsLeft, 2, 2, 2, 2));
    m_update_toggle->SetToolTipText("Switch on/off the auto refresh of all monitors");
    m_update_toggle->Connect("Toggled(Bool_t)", NAME, this, "SwitchUpdate(Bool_t)");

    m_refresh_toggle = new TGCheckButton(m_toolbar, "&Clear between runs", 1);
    m_toolbar->AddFrame(m_refresh_toggle, new TGLayoutHints(kLHintsLeft, 2, 2, 2, 2));
    m_refresh_toggle->SetToolTipText("Clear all monitors between two runs");
    m_refresh_toggle->Connect("Toggled(Bool_t)", NAME, this, "SwitchClearRuns(Bool_t)");

    // main canvas
    m_main_canvas = new TRootEmbeddedCanvas("Canvas", right_frame);
    right_frame->AddFrame(m_main_canvas, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 5, 5, 5, 5));
    m_top_win->AddFrame(right_frame, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 2, 2, 2, 2));
    this->AddFrame(m_top_win, new TGLayoutHints(kLHintsExpandY | kLHintsExpandX | kLHintsLeft, 0, 0, 0, 0));

    // status bar
    int status_parts[(int)StatusBarPos::num_parts] = {20, 10, 35, 35};
    m_status_bar = new TGStatusBar(this, 510, 10, kHorizontalFrame);
    m_status_bar->SetParts(status_parts, (int)StatusBarPos::num_parts);
    ResetCounters();

    this->AddFrame(m_status_bar, new TGLayoutHints(kLHintsBottom | kLHintsExpandX, 0, 0, 2, 0));

    m_timer->Connect("Timeout()", NAME, this, "Update()");
    m_update_toggle->SetOn();
    m_refresh_toggle->SetOn();
    SwitchUpdate(true);

    this->MapSubwindows();
    this->Layout();
    this->MapSubwindows();
    this->MapWindow();
    m_context_menu = new TContextMenu("", "");
  }

  ROOTMonitorWindow::~ROOTMonitorWindow(){
    Quit();
    // unregister the icons
    gClient->FreePicture(m_icon_save);
    gClient->FreePicture(m_icon_del);
    gClient->FreePicture(m_icon_th1);
    gClient->FreePicture(m_icon_th2);
    gClient->FreePicture(m_icon_tprofile);
    gClient->FreePicture(m_icon_track);
    gClient->FreePicture(m_icon_summ);
    for (auto& obj : m_objects)
      delete obj.second.object;
  }

  //--- signal handling

  void ROOTMonitorWindow::Quit(){
    Emit("Quit()");
  }

  void ROOTMonitorWindow::FillFromRAWFile(const char* path){
    Emit("FillFromRAWFile(const char*)", path);
  }

  //--- counters/status bookeeping

  void ROOTMonitorWindow::SetCounters(unsigned long long evt_recv, unsigned long long evt_mon){
    m_last_event = evt_recv;
    m_last_event_mon = evt_mon;
  }

  void ROOTMonitorWindow::ResetCounters(){
    if (m_status_bar) {
      m_status_bar->SetText("Run: N/A", (int)StatusBarPos::run_number);
      m_status_bar->SetText("Curr. event: N/A", (int)StatusBarPos::tot_events);
      m_status_bar->SetText("Analysed events: N/A", (int)StatusBarPos::an_events);
    }
    m_button_save->SetEnabled(false);
    m_button_clean->SetEnabled(false);
    if (m_clear_between_runs)
      ClearMonitors();
  }

  void ROOTMonitorWindow::SetRunNumber(int run){
    m_run_number = run;
    if (m_status_bar)
      m_status_bar->SetText(Form("Run: %u", m_run_number), (int)StatusBarPos::run_number);
    m_button_save->SetEnabled(true);
    m_button_clean->SetEnabled(true);
  }

  void ROOTMonitorWindow::SetLastEventNum(int num){
    if (num >= 0)
      m_last_event = num;
    if (m_status_bar && (num < 0 || m_status == Status::STATE_RUNNING))
      m_status_bar->SetText(Form("Curr. event: %llu", m_last_event), (int)StatusBarPos::tot_events);
  }

  void ROOTMonitorWindow::SetMonitoredEventsNum(int num){
    if (num >= 0)
      m_last_event_mon = num;
    if (m_status_bar && (num < 0 || m_status == Status::STATE_RUNNING))
      m_status_bar->SetText(Form("Analysed events: %llu", m_last_event_mon), (int)StatusBarPos::an_events);
  }

  void ROOTMonitorWindow::SetStatus(Status::State st){
    m_status = st;
    if (m_status_bar)
      m_status_bar->SetText(Status::State2String(st).c_str(), (int)StatusBarPos::status);
  }

  //--- file I/O

  void ROOTMonitorWindow::OpenFileDialog(){
    static TString dir(".");
    // allows for both RAW and post-processed ROOT files (TBrowser-like)
    const char* filetypes[] = {"RAW files",  "*.raw",
                               "ROOT files", "*.root",
                               0,            0};
    TGFileInfo fi;
    fi.fFileTypes = filetypes;
    fi.fIniDir = StrDup(dir);
    new TGFileDialog(gClient->GetRoot(), this, kFDOpen, &fi); // handled by GC
    if (fi.fMultipleSelection && fi.fFileNamesList) // only grab first file
      LoadFile(dynamic_cast<TObjString*>(fi.fFileNamesList->First())->GetString().Data());
    else if (fi.fFilename != nullptr)
      LoadFile(fi.fFilename);
  }

  void ROOTMonitorWindow::LoadFile(const char* filename){
    // disable control over update
    m_update_toggle->SetEnabled(false);
    m_refresh_toggle->SetEnabled(false);
    // filter by extension
    TString s_ext(filename);
    s_ext.Remove(0, s_ext.Last('.')+1).ToLower();
    if (s_ext == "root") {
      FillFileObject("", TFile::Open(filename, "read"), ""); // handled by GC
      m_button_save->SetEnabled(false); // nothing has changed, export feature disabled
    }
    else if (s_ext == "raw") {
      FillFromRAWFile(filename);
      m_button_save->SetEnabled(true); // allows for export
    }
    Update();
    m_button_clean->SetEnabled(false);
  }

  void ROOTMonitorWindow::FillFileObject(const std::string& path, TObject* obj, const std::string& path_par){
    const std::string full_path = path_par.empty() ? path : path_par+"/"+path;
    if (obj->IsFolder())
      for (const auto&& key : *((TDirectory*)obj)->GetListOfKeys())
        FillFileObject(key->GetName(), ((TKey*)key)->ReadObj(), path);
    if (obj->InheritsFrom("TH2"))
      Book<TH2D>(full_path, path, *(TH2D*)obj);
    else if (obj->InheritsFrom("TH1"))
      Book<TH1D>(full_path, path, *(TH1D*)obj);
    else if (obj->InheritsFrom("TGraph"))
      Book<TGraph>(full_path, path, *(TGraph*)obj);
    else if (!path.empty())
      std::cerr << "Failed to load a specific object: " << path << std::endl;
  }

  void ROOTMonitorWindow::SaveFileDialog(){
    TGFileInfo fi;
    // first define the output file
    static TString dir(".");
    const char *filetypes[] = {"ROOT files", "*.root",
                               0,            0};
    fi.fFileTypes = filetypes;
    fi.fIniDir = StrDup(dir);
    new TGFileDialog(gClient->GetRoot(), this, kFDSave, &fi);
    if (fi.fFilename != nullptr)
      SaveFile(fi.fFilename);
  }

  void ROOTMonitorWindow::SaveFile(const char* filename){
    // save all collections
    auto file = std::unique_ptr<TFile>(TFile::Open(filename, "recreate"));
    for (auto& obj : m_objects) {
      TString s_file(obj.first);
      auto pos = s_file.Last('/');
      TString s_path;
      if (pos != kNPOS) { // substructure found
        s_path = s_file;
        s_path.Remove(pos);
        s_file.Remove(0, pos+1);
        if (!file->GetDirectory(s_path))
          file->mkdir(s_path);
      }
      file->cd(s_path);
      if (obj.second.object->Write(s_file) == 0)
        std::cerr << "[WARNING] Failed to write \"" << obj.first << "\" into the output file!" << std::endl;
    }
    file->Close();
  }

  //--- live monitoring switches

  void ROOTMonitorWindow::SwitchUpdate(bool up){
    if (!up)
      m_timer->Stop();
    else if (up)
      m_timer->Start(-1, kFALSE); // update automatically
    m_canv_needs_refresh = true;
  }

  void ROOTMonitorWindow::SwitchClearRuns(bool clear){
    m_clear_between_runs = clear;
  }

  //--- graphical part

  void ROOTMonitorWindow::Update(){
    SetLastEventNum(m_last_event);
    SetMonitoredEventsNum(m_last_event_mon);

    // check if we have something to draw
    if (m_drawable.empty())
      return;
    TCanvas* canv = m_main_canvas->GetCanvas();
    if (!canv) { // failed to retrieve the plotting region
      std::cerr << "[WARNING] Failed to retrieve the main plotting canvas!" << std::endl;
      return;
    }
    Draw(canv);
    PostDraw(canv);
  }

  void ROOTMonitorWindow::Draw(TCanvas* canv){
    canv->cd();
    if (m_canv_needs_refresh) { // book the pads and monitors placeholders
      canv->Clear();
      if (m_drawable.size() > 1) {
        int ncol = ceil(sqrt(m_drawable.size()));
        int nrow = ceil(m_drawable.size()*1./ncol);
        canv->Divide(ncol, nrow);
      }
      for (size_t i = 0; i < m_drawable.size(); ++i) {
        auto& dr = m_drawable.at(i);
        auto pad = canv->cd(i+1);
        dr->object->Draw(dr->draw_opt);
        pad->SetTicks();
        pad->SetGrid();
        if (!dr->time_series.empty()) {
          TAxis* axis = nullptr;
          if (dr->object->InheritsFrom("TH1"))
            axis = dynamic_cast<TH1*>(dr->object)->GetXaxis();
          else if (dr->object->InheritsFrom("TGraph"))
            axis = dynamic_cast<TGraph*>(dr->object)->GetXaxis();
          if (axis) {
            axis->SetTimeDisplay(1);
            axis->SetTimeFormat(dr->time_series.c_str());
          }
        }
      }
      canv->SetTicks();
      canv->SetGrid();
      m_canv_needs_refresh = false;
    }
  }

  void ROOTMonitorWindow::PostDraw(TCanvas* canv){
    // canvas(es) flushing procedure
    canv->Modified();
    canv->Update();
    size_t i = 0;
    for (size_t i = 0; i < m_drawable.size(); ++i) {
      auto& dr = m_drawable.at(i);
      auto pad = canv->GetPad(i+1);
      // clean all non-persistent objects
      if (!dr->persist)
        CleanObject(dr->object);
      // monitor vertical range to be set at the end
      if (dr->min_y != kInvalidValue && dr->max_y != kInvalidValue) {
        if (dr->object->InheritsFrom("TH1"))
          dynamic_cast<TH1*>(dr->object)->GetYaxis()->SetRangeUser(dr->min_y, dr->max_y);
        else if (dr->object->InheritsFrom("TGraph")) {
          auto gr = dynamic_cast<TGraph*>(dr->object);
          gr->SetMaximum(dr->max_y);
          gr->SetMinimum(dr->min_y);
        }
      }
      if (pad) {
        pad->Update();
        pad->Modified();
      }
    }
  }

  //--- monitoring elements helpers

  TObject* ROOTMonitorWindow::Get(const std::string& name){
    auto it = m_objects.find(name);
    if (it == m_objects.end())
      throw std::runtime_error("Failed to retrieve object with path \""+std::string(name)+"\"!");
    return it->second.object;
  }

  void ROOTMonitorWindow::DrawElement(TGListTreeItem* it, int val){
    m_drawable.clear();
    for (auto& obj : m_objects)
      if (obj.second.item == it)
        m_drawable.emplace_back(&obj.second);
    if (m_drawable.empty()) // did not find in objects, must be a directory
      for (auto& obj : m_objects)
        if (obj.second.item->GetParent() == it)
          m_drawable.emplace_back(&obj.second);
    if (m_drawable.empty()) // did not find in directories either, must be a summary
      if (m_summ_objects.count(it) > 0)
        for (auto& obj : m_summ_objects[it])
    m_drawable.emplace_back(obj);
    m_canv_needs_refresh = true;
  }

  void ROOTMonitorWindow::DrawMenu(TGListTreeItem* it, int but, int x, int y){
    if (but == 3)
      m_context_menu->Popup(x, y, this);
  }

  ROOTMonitorWindow::MonitoredObject& ROOTMonitorWindow::GetMonitor(const TObject* obj){
    for (auto& o : m_objects)
      if (o.second.object == obj)
        return o.second;
    throw std::runtime_error("Failed to retrieve an object!");
  }

  void ROOTMonitorWindow::ClearMonitors(){
    // clear non-persistent objects before next refresh
    for (auto& dr : m_objects)
      CleanObject(dr.second.object);
  }

  void ROOTMonitorWindow::CleanObject(TObject* obj){
    if (obj->InheritsFrom("TMultiGraph")) { // special case for this one
      for (auto gr : *dynamic_cast<TMultiGraph*>(obj))
        delete gr;
    }
    else if (obj->InheritsFrom("TGraph"))
      dynamic_cast<TGraph*>(obj)->Set(0);
    else if (obj->InheritsFrom("TGraph2D"))
      dynamic_cast<TGraph2D*>(obj)->Set(0);
    else if (obj->InheritsFrom("TH1"))
      dynamic_cast<TH1*>(obj)->Reset();
    else
      std::cerr
        << "[WARNING] monitoring object with class name "
        << "\"" << obj->ClassName() << "\" cannot be cleared"
        << std::endl;
  }

  //--- monitors hierarchy

  TGListTreeItem* ROOTMonitorWindow::BookStructure(const std::string& path, TGListTreeItem* par){
    auto tok = TString(path).Tokenize("/");
    if (tok->IsEmpty())
      return par;
    TGListTreeItem* prev = nullptr;
    std::string full_path;
    for (int i = 0; i < tok->GetEntriesFast()-1; ++i) {
      const auto iter = tok->At(i);
      TString dir_name = dynamic_cast<TObjString*>(iter)->String();
      full_path += dir_name+"/";
      if (m_dirs.count(full_path) == 0)
        m_dirs[full_path] = m_tree_list->AddItem(prev, dir_name);
      prev = m_dirs[full_path];
    }
    return prev;
  }

  void ROOTMonitorWindow::AddSummary(const std::string& path, const TObject* obj){
    for (auto& o : m_objects) {
      if (o.second.object != obj)
        continue;
      if (m_dirs.count(path) == 0) {
        auto obj_name = path;
        // keep only the last part
        obj_name.erase(0, obj_name.rfind('/')+1);
        m_dirs[path] = m_tree_list->AddItem(BookStructure(path), obj_name.c_str());
        m_dirs[path]->SetPictures(m_icon_summ, m_icon_summ);
      }
      auto& objs = m_summ_objects[m_dirs[path]];
      if (std::find(objs.begin(), objs.end(), &o.second) == objs.end())
        objs.emplace_back(&o.second);
      m_left_canv->MapSubwindows();
      m_left_canv->MapWindow();
      return;
    }
    throw std::runtime_error("Failed to retrieve an object for summary \""+path+"\"");
  }
}
