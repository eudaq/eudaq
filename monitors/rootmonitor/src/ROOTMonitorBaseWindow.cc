#include "eudaq/ROOTMonitorBaseWindow.hh"

#include <TApplication.h>
#include <TFile.h>
#include <TKey.h>
#include <TObjString.h>
#include <TTimer.h>

#include <TGraph.h>
#include <TGraph2D.h>
#include <TH1.h>
#include <TH2.h>
#include <TMultiGraph.h>

#include <iostream>
#include <fstream>

namespace eudaq {
  ROOTMonitorBaseWindow::ROOTMonitorBaseWindow(TApplication *par,
                                               const std::string &name)
      : m_parent(par), m_icon_save(gClient->GetPicture("bld_save.xpm")),
        m_icon_del(gClient->GetPicture("bld_delete.xpm")),
        m_icon_open(gClient->GetPicture("bld_open.xpm")),
        m_icon_th1(gClient->GetPicture("h1_t.xpm")),
        m_icon_th2(gClient->GetPicture("h2_t.xpm")),
        m_icon_tprofile(gClient->GetPicture("profile_t.xpm")),
        m_icon_track(gClient->GetPicture("eve_track.xpm")),
        m_icon_summ(gClient->GetPicture("draw_t.xpm")),
        m_timer(new TTimer(1000, kTRUE)) {
    // m_tree_list = new TGListTree(nullptr, kHorizontalFrame);
    // m_tree_list->Connect("DoubleClicked(TGListTreeItem*, Int_t)", NAME, this,
    //                      "DrawElement(TGListTreeItem*, Int_t)");
    ResetCounters();

    m_timer->Connect("Timeout()", NAME, this, "Update()");

    SwitchUpdate(true);
  }

  ROOTMonitorBaseWindow::~ROOTMonitorBaseWindow() {
    Quit();
    // unregister the icons
    gClient->FreePicture(m_icon_save);
    gClient->FreePicture(m_icon_del);
    gClient->FreePicture(m_icon_th1);
    gClient->FreePicture(m_icon_th2);
    gClient->FreePicture(m_icon_tprofile);
    gClient->FreePicture(m_icon_track);
    gClient->FreePicture(m_icon_summ);
    for (auto &obj : m_objects)
      delete obj.second.object;
  }

  //--- signal handling

  void ROOTMonitorBaseWindow::Quit() { Emit("Quit()"); }

  void ROOTMonitorBaseWindow::FillFromRAWFile(const char *path) {
    Emit("FillFromRAWFile(const char*)", path);
  }

  //--- counters/status bookeeping

  void ROOTMonitorBaseWindow::SetCounters(unsigned long long evt_recv,
                                          unsigned long long evt_mon) {
    m_last_event = evt_recv;
    m_last_event_mon = evt_mon;
  }

  void ROOTMonitorBaseWindow::ResetCounters() {
    if (m_clear_between_runs)
      ClearMonitors();
  }

  void ROOTMonitorBaseWindow::SetRunNumber(int run) { m_run_number = run; }

  void ROOTMonitorBaseWindow::SetLastEventNum(int num) {
    if (num >= 0)
      m_last_event = num;
  }

  void ROOTMonitorBaseWindow::SetMonitoredEventsNum(int num) {
    if (num >= 0)
      m_last_event_mon = num;
  }

  void ROOTMonitorBaseWindow::SetStatus(Status::State st) { m_status = st; }

  //--- file I/O

  void ROOTMonitorBaseWindow::LoadFile(const char *filename) {
    // filter by extension
    TString s_ext(filename);
    s_ext.Remove(0, s_ext.Last('.') + 1).ToLower();
    if (s_ext == "root")
      FillFileObject("", TFile::Open(filename, "read"), ""); // handled by GC
    else if (s_ext == "raw") {
      FillFromRAWFile(filename);
      Update();
    }
  }

  void ROOTMonitorBaseWindow::FillFileObject(const std::string &path,
                                             TObject *obj,
                                             const std::string &path_par) {
    const std::string full_path =
        path_par.empty() ? path : path_par + "/" + path;
    if (obj->IsFolder())
      for (const auto &&key : *((TDirectory *)obj)->GetListOfKeys())
        FillFileObject(key->GetName(), ((TKey *)key)->ReadObj(), path);
    if (obj->InheritsFrom("TH2"))
      Book<TH2D>(full_path, path, *(TH2D *)obj);
    else if (obj->InheritsFrom("TH1"))
      Book<TH1D>(full_path, path, *(TH1D *)obj);
    else if (obj->InheritsFrom("TGraph"))
      Book<TGraph>(full_path, path, *(TGraph *)obj);
    else if (!path.empty())
      std::cerr << "Failed to load a specific object: " << path << std::endl;
  }

  void ROOTMonitorBaseWindow::SaveFile(const char *filename) {
    // save all collections
    auto file = std::unique_ptr<TFile>(TFile::Open(filename, "recreate"));
    for (auto &obj : m_objects) {
      TString s_file(obj.first);
      auto pos = s_file.Last('/');
      TString s_path;
      if (pos != kNPOS) { // substructure found
        s_path = s_file;
        s_path.Remove(pos);
        s_file.Remove(0, pos + 1);
        if (!file->GetDirectory(s_path))
          file->mkdir(s_path);
      }
      file->cd(s_path);
      if (obj.second.object->Write(s_file) == 0)
        std::cerr << "[WARNING] Failed to write \"" << obj.first
                  << "\" into the output file!" << std::endl;
    }
    file->Close();
  }

  //--- live monitoring switches

  void ROOTMonitorBaseWindow::SwitchUpdate(bool up) {
    if (!up)
      m_timer->Stop();
    else if (up)
      m_timer->Start(-1, kFALSE); // update automatically
    m_canv_needs_refresh = true;
  }

  void ROOTMonitorBaseWindow::SwitchClearRuns(bool clear) {
    m_clear_between_runs = clear;
  }

  //--- graphical part

  void ROOTMonitorBaseWindow::Update() {
    SetLastEventNum(m_last_event);
    SetMonitoredEventsNum(m_last_event_mon);
  }

  void ROOTMonitorBaseWindow::Draw() {
    if (m_canv_needs_refresh) { // book the pads and monitors placeholders
      for (size_t i = 0; i < m_drawable.size(); ++i) {
        auto &dr = m_drawable.at(i);
        if (!dr->time_series.empty()) {
          TAxis *axis = nullptr;
          if (dr->object->InheritsFrom("TH1"))
            axis = dynamic_cast<TH1 *>(dr->object)->GetXaxis();
          else if (dr->object->InheritsFrom("TGraph"))
            axis = dynamic_cast<TGraph *>(dr->object)->GetXaxis();
          if (axis) {
            axis->SetTimeDisplay(1);
            axis->SetTimeFormat(dr->time_series.c_str());
          }
        }
      }
      m_canv_needs_refresh = false;
    }
  }

  void ROOTMonitorBaseWindow::PostDraw() {
    for (size_t i = 0; i < m_drawable.size(); ++i) {
      auto &dr = m_drawable.at(i);
      // clean all non-persistent objects
      if (!dr->persist)
        CleanObject(dr->object);
      // monitor vertical range to be set at the end
      if (dr->min_y != kInvalidValue && dr->max_y != kInvalidValue) {
        if (dr->object->InheritsFrom("TH1"))
          dynamic_cast<TH1 *>(dr->object)
              ->GetYaxis()
              ->SetRangeUser(dr->min_y, dr->max_y);
        else if (dr->object->InheritsFrom("TGraph")) {
          auto gr = dynamic_cast<TGraph *>(dr->object);
          gr->SetMaximum(dr->max_y);
          gr->SetMinimum(dr->min_y);
        }
      }
    }
  }

  //--- monitoring elements helpers

  TObject *ROOTMonitorBaseWindow::Get(const std::string &name) {
    auto it = m_objects.find(name);
    if (it == m_objects.end())
      throw std::runtime_error("Failed to retrieve object with path \"" +
                               std::string(name) + "\"!");
    return it->second.object;
  }

  void ROOTMonitorBaseWindow::DrawElement(TGListTreeItem *it, int val) {
    m_drawable.clear();
    for (auto &obj : m_objects)
      if (obj.second.item == it)
        m_drawable.emplace_back(&obj.second);
    if (m_drawable.empty()) // did not find in objects, must be a directory
      for (auto &obj : m_objects)
        if (obj.second.item->GetParent() == it)
          m_drawable.emplace_back(&obj.second);
    if (m_drawable
            .empty()) // did not find in directories either, must be a summary
      if (m_summ_objects.count(it) > 0)
        for (auto &obj : m_summ_objects[it])
          m_drawable.emplace_back(obj);
    m_canv_needs_refresh = true;
  }

  ROOTMonitorBaseWindow::MonitoredObject &
  ROOTMonitorBaseWindow::GetMonitor(const TObject *obj) {
    for (auto &o : m_objects)
      if (o.second.object == obj)
        return o.second;
    throw std::runtime_error("Failed to retrieve an object!");
  }

  void ROOTMonitorBaseWindow::ClearMonitors() {
    // clear non-persistent objects before next refresh
    for (auto &dr : m_objects)
      CleanObject(dr.second.object);
  }

  void ROOTMonitorBaseWindow::CleanObject(TObject *obj) {
    if (obj->InheritsFrom("TMultiGraph")) { // special case for this one
      for (auto gr : *dynamic_cast<TMultiGraph *>(obj))
        delete gr;
    } else if (obj->InheritsFrom("TGraph"))
      dynamic_cast<TGraph *>(obj)->Set(0);
    else if (obj->InheritsFrom("TGraph2D"))
      dynamic_cast<TGraph2D *>(obj)->Set(0);
    else if (obj->InheritsFrom("TH1"))
      dynamic_cast<TH1 *>(obj)->Reset();
    else
      std::cerr << "[WARNING] monitoring object with class name "
                << "\"" << obj->ClassName() << "\" cannot be cleared"
                << std::endl;
  }

  //--- monitors hierarchy

  TGListTreeItem *ROOTMonitorBaseWindow::BookStructure(const std::string &path,
                                                       TGListTreeItem *par) {
    auto tok = TString(path).Tokenize("/");
    if (tok->IsEmpty())
      return par;
    TGListTreeItem *prev = nullptr;
    std::string full_path;
    for (int i = 0; i < tok->GetEntriesFast() - 1; ++i) {
      const auto iter = tok->At(i);
      TString dir_name = dynamic_cast<TObjString *>(iter)->String();
      full_path += dir_name + "/";
      /*if (m_dirs.count(full_path) == 0)
        m_dirs[full_path] = m_tree_list->AddItem(prev, dir_name);*/
      prev = m_dirs[full_path];
    }
    return prev;
  }

  void ROOTMonitorBaseWindow::AddSummary(const std::string &path,
                                         const TObject *obj) {
    for (auto &o : m_objects) {
      if (o.second.object != obj)
        continue;
      if (m_dirs.count(path) == 0) {
        auto obj_name = path;
        // keep only the last part
        obj_name.erase(0, obj_name.rfind('/') + 1);
        /*m_dirs[path] =
            m_tree_list->AddItem(BookStructure(path), obj_name.c_str());*/
        m_dirs[path]->SetPictures(m_icon_summ, m_icon_summ);
      }
      auto &objs = m_summ_objects[m_dirs[path]];
      if (std::find(objs.begin(), objs.end(), &o.second) == objs.end())
        objs.emplace_back(&o.second);
      MapCanvas();
      return;
    }
    throw std::runtime_error("Failed to retrieve an object for summary \"" +
                             path + "\"");
  }
} // namespace eudaq
