#ifndef EUDAQ_INCLUDED_ROOTMonitorWindow
#define EUDAQ_INCLUDED_ROOTMonitorWindow

#include "eudaq/Status.hh"

#ifndef __CINT__
#include "RQ_OBJECT.h"
#include "TContextMenu.h"
#include "TGButton.h"
#include "TGFrame.h"
#include "TGListTree.h"
#include "TGStatusBar.h"
#include "TGToolBar.h"
#include "TRootEmbeddedCanvas.h"
#endif
#include <functional>

class TApplication;
class TTree;

namespace eudaq {
  class Event;
  /// A placeholder for ROOT monitoring
  class ROOTMonitorWindow : public TGMainFrame {
    static constexpr const char *NAME = "eudaq::ROOTMonitorWindow";
    RQ_OBJECT(NAME)
  public:
    ROOTMonitorWindow(TApplication *, const std::string &name);
    ~ROOTMonitorWindow();

    void SetCounters(unsigned long long evt_recv, unsigned long long evt_mon);
    /// Reset the status bar events counters
    void ResetCounters();
    /// Specify the run number retrueved from run control
    void SetRunNumber(int run);
    /// Specify the number of events collected
    void SetLastEventNum(int num = -1);
    /// Specify the number of events already processed
    void SetMonitoredEventsNum(int num = -1);
    /// Add a summary page including several monitors
    void AddSummary(const std::string &path, const TObject *obj);

    /// Update the FSM
    void SetStatus(Status::State st);
    /// Launch a "Open file" dialog to load a INI/CFG file for
    /// initialisation/configuration of this monitor
    void OpenConfigFileDialog();
    /// Launch a "Open file" dialog to load a RAW/ROOT file and reproduce its
    /// monitoring
    void OpenDataFileDialog();
    /// Initialise/configure the monitor with user-steered cards
    void LoadConfigFile(const char *filename);
    /// Load all monitored object from a RAW/ROOT file
    void LoadDataFile(const char *filename);
    /// Launch a "Save as..." dialog to save all monitored objects into a ROOT
    /// file
    void SaveFileDialog();
    /// Save all monitored objects into a ROOT file
    void SaveFile(const char *filename);
    /// Clean all monitored objects before a new run
    void ClearMonitors();

    /// Add a new monitor to the stack, as a simple TObject-derivative
    template <typename T, typename... Args>
    T *Book(const std::string &path, const std::string &name, Args &&...args) {
      if (m_objects.count(path) != 0)
        return Get<T>(path);
      auto obj = new T(std::forward<Args>(args)...);
      auto item = m_tree_list->AddItem(BookStructure(path), name.c_str());
      auto it_icon = m_obj_icon.find(obj->ClassName());
      if (it_icon != m_obj_icon.end())
        item->SetPictures(it_icon->second, it_icon->second);
      MonitoredObject mon;
      mon.item = item;
      mon.object = obj;
      m_objects[path] = mon;
      m_left_canv->MapSubwindows();
      m_left_canv->MapWindow();
      return obj;
    }
    /// Retrieve a monitored object by its path and type
    template <typename T> T *Get(const std::string &name) {
      return dynamic_cast<T *>(Get(name));
    }
    /// Retrieve a monitored object by its path
    TObject *Get(const std::string &name);
    /// Specify if an object is required to be cleaned at each refresh
    void SetPersistant(const TObject *obj, bool pers = true) {
      GetMonitor(obj).persist = pers;
    }
    /// Specify the drawing properties of the object in the monitor
    void SetDrawOptions(const TObject *obj, Option_t *opt) {
      GetMonitor(obj).draw_opt = opt;
    }
    /// Specify the y axis range for a monitored object
    void SetRangeY(const TObject *obj, double min, double max) {
      auto &mon = GetMonitor(obj);
      mon.min_y = min, mon.max_y = max;
    }
    /// Specify if the x-axis should be associated with time
    void SetTimeSeries(const TObject *obj, const std::string &time) {
      GetMonitor(obj).time_series = time;
    }

    /// Action triggered when a monitor is to be drawn
    void DrawElement(TGListTreeItem *, int);
    /// Action triggered when a hierarchised monitor structure is to be shown
    void DrawMenu(TGListTreeItem *, int, int, int);

    /// Refresh the displayed monitor(s)
    void Update();
    /// Turn on/off the auto-refresh
    void SwitchUpdate(bool);
    /// Enable/disable monitors cleaning between runs
    void SwitchClearRuns(bool);
    /// Clean up everything before terminating the application
    void Quit();
    /// Initialise this monitor from a INI file
    void InitialiseFromFile(const char *path);
    /// Configure this monitor from a CONF file
    void ConfigureFromFile(const char *path);
    /// Reprocess monitors from a RAW file
    void FillFromRAWFile(const char *path);

  private:
    /// List of status bar attributes
    enum class StatusBarPos {
      status = 0,
      run_number,
      tot_events,
      an_events,
      num_parts
    };
    TGListTreeItem *BookStructure(const std::string &path,
                                  TGListTreeItem *par = nullptr);
    void CleanObject(TObject *);
    void FillFileObject(const std::string &path, TObject *obj,
                        const std::string &path_par = "");
    void Draw(TCanvas *canv);
    void PostDraw(TCanvas *canv);

    // ROOT GUI objects handled
    TGHorizontalFrame *m_top_win;
    TGVerticalFrame *m_left_bar;
    TGStatusBar *m_status_bar;
    TGCanvas *m_left_canv;
    TGToolBar *m_toolbar;
    TGButton *m_button_ini, *m_button_config, *m_button_open, *m_button_save,
        *m_button_clean;
    TRootEmbeddedCanvas *m_main_canvas;
    TGCheckButton *m_update_toggle, *m_refresh_toggle;
    TGListTree *m_tree_list;
    TContextMenu *m_context_menu;

    const TGPicture *m_icon_summ;
    const TGPicture *m_icon_save, *m_icon_del, *m_icon_open, *m_icon_ini;
    const TGPicture *m_icon_th1, *m_icon_th2, *m_icon_tprofile, *m_icon_track;

    /// Timer for auto-refresh loop
    std::unique_ptr<TTimer> m_timer;
    static constexpr double kInvalidValue = 42.424242;
    struct MonitoredObject {
      TGListTreeItem *item = nullptr;
      TObject *object = nullptr;
      bool persist = true;
      double min_y = kInvalidValue, max_y = kInvalidValue;
      std::string time_series = "";
      Option_t *draw_opt = "";
    };

    MonitoredObject &GetMonitor(const TObject *obj);

    /// List of all objects handled and monitored
    std::map<std::string, MonitoredObject> m_objects;
    std::map<std::string, TGListTreeItem *> m_dirs;
    /// List of all summary plots sharing one canvas
    std::map<std::string, std::vector<std::string>> m_summaries;
    std::map<TGListTreeItem *, std::vector<MonitoredObject *>> m_summ_objects;
    std::map<std::string, const TGPicture *> m_obj_icon = {
        {"TH1", m_icon_th1},           {"TH1F", m_icon_th1},
        {"TH1D", m_icon_th1},          {"TH1I", m_icon_th1},
        {"TH2", m_icon_th2},           {"TH2F", m_icon_th2},
        {"TH2D", m_icon_th2},          {"TH2I", m_icon_th2},
        {"TGraph", m_icon_tprofile},   {"TGraph2D", m_icon_th2},
        {"TProfile", m_icon_tprofile}, {"TMultiGraph", m_icon_track}};
    /// List of all objects to be drawn on main canvas
    std::vector<MonitoredObject *> m_drawable;
    bool m_canv_needs_refresh = true;
    bool m_clear_between_runs = true;

    /// Parent owning application
    TApplication *m_parent = nullptr;
    /// Current "FSM" status
    Status::State m_status = Status::STATE_UNINIT;
    int m_run_number = -1;
    unsigned long long m_last_event = 0;
    unsigned long long m_last_event_mon = 0;

    ClassDef(ROOTMonitorWindow, 0);
  };
} // namespace eudaq

#endif
