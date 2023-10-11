#include "eudaq/Logger.hh"
#include "eudaq/ROOTMonitorBaseWindow.hh"

#include <TApplication.h>
#include <TFile.h>
#include <TGraph.h>
#include <TGraph2D.h>
#include <TH1.h>
#include <TH2.h>
#include <TKey.h>
#include <TMultiGraph.h>
#include <TTimer.h>

#include <fstream>
#include <iostream>

using namespace std::string_literals;

namespace eudaq {
  ROOTMonitorBaseWindow::ROOTMonitorBaseWindow(TApplication *par,
                                               const std::string &name)
      : m_parent(par),
        m_folder(new TFolder(kDirName, "eudaq ROOT monitor file hierarchy")),
        m_refresh_timer(new TTimer(1000, kTRUE)) {
    ResetCounters();
    m_refresh_timer->Connect("Timeout()", NAME, this, "UpdateAll()");
    SwitchUpdate(true);
  }

  ROOTMonitorBaseWindow::~ROOTMonitorBaseWindow() {
    Quit();
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
      UpdateAll();
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
      m_refresh_timer->Stop();
    else if (up)
      m_refresh_timer->Start(-1, kFALSE); // update automatically
    m_canv_needs_refresh = true;
  }

  void ROOTMonitorBaseWindow::SwitchClearRuns(bool clear) {
    m_clear_between_runs = clear;
  }

  //--- graphical part

  void ROOTMonitorBaseWindow::UpdateAll() {
    SetLastEventNum(m_last_event);
    SetMonitoredEventsNum(m_last_event_mon);
    Update();
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

  TFolder *ROOTMonitorBaseWindow::GetFolder(const std::string &path,
                                            TFolder *base, bool full) {
    const auto tok = split(path, "/"); // EUDAQ utility
    if (!base)
      base = m_folder.get();
    if (tok.empty())
      return base;
    TFolder *prev = base;
    std::string full_path, sep;
    for (int i = 0; i < tok.size() - 1 + full; ++i) {
      const auto &dir_name = tok.at(i);
      full_path += sep + dir_name, sep = "/";
      if (m_dirs.count(full_path) == 0)
        m_dirs[full_path] = prev->AddFolder(dir_name.data(), path.data());
      prev = m_dirs[full_path];
    }
    return prev;
  }

  TObject *ROOTMonitorBaseWindow::Get(const std::string &name) {
    auto it = m_objects.find(name);
    if (it == m_objects.end())
      EUDAQ_THROW("Failed to retrieve the object with path '" + name + "'.");
    return it->second.object;
  }

  std::string ROOTMonitorBaseWindow::GetFolderPath(const TFolder *dir) const {
    if (strcmp(dir->GetName(), kDirName) == 0)
      return ""s;
    for (const auto &o : m_dirs)
      if (o.second == dir)
        return o.first;
    EUDAQ_THROW("Failed to retrieve the path of a directory with name '"s +
                dir->GetName() + "'.");
  }

  std::string ROOTMonitorBaseWindow::GetPath(const TObject *obj) const {
    for (const auto &o : m_objects)
      if (o.second.object == obj)
        return o.first;
    EUDAQ_THROW("Failed to retrieve the path of an object with name '"s +
                obj->GetName() + "'.");
  }

  ROOTMonitorBaseWindow::MonitoredObject &
  ROOTMonitorBaseWindow::GetMonitor(const TObject *obj) {
    for (auto &o : m_objects)
      if (o.second.object == obj)
        return o.second;
    EUDAQ_THROW(
        "Failed to retrieve the monitoring properties of an object with name '"s +
        obj->GetName() + "'.");
  }

  void ROOTMonitorBaseWindow::AddSummary(const std::string &path,
                                         const TObject *obj) {
    if (m_dirs.count(path) == 0)
      m_dirs[path] = GetFolder(path, nullptr, true);
    auto &objs = m_summ_objects[m_dirs.at(path)];
    auto &obj_mon = GetMonitor(obj);
    if (std::find(objs.begin(), objs.end(), &obj_mon) == objs.end())
      objs.emplace_back(&obj_mon);
    MapCanvas();
    UpdateMonitorsList();
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
      EUDAQ_WARN("Monitoring object with class name '"s + obj->ClassName() +
                 "' cannot be cleared.");
  }
} // namespace eudaq
