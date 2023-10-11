#ifndef EUDAQ_INCLUDED_ROOTMonitorBaseWindow
#define EUDAQ_INCLUDED_ROOTMonitorBaseWindow

#include "eudaq/Status.hh"

#ifndef __CINT__
#include <RQ_OBJECT.h>
#include <TFolder.h>
#endif
#include <functional>

class TApplication;

namespace eudaq {
  class Event;
  /// A placeholder for ROOT monitoring
  class ROOTMonitorBaseWindow /*: public TObject*/ {
    static constexpr const char *NAME = "eudaq::ROOTMonitorBaseWindow";
    static constexpr const char *kDirName = "eudaq";
    RQ_OBJECT(NAME)

  public:
    explicit ROOTMonitorBaseWindow(TApplication *, const std::string &name);
    virtual ~ROOTMonitorBaseWindow();

    /// Update the FSM
    virtual void SetStatus(Status::State st);
    /// Set events received/monitored counters
    virtual void SetCounters(unsigned long long evt_recv,
                             unsigned long long evt_mon);
    /// Reset the status bar events counters
    virtual void ResetCounters();
    /// Specify the run number retrueved from run control
    virtual void SetRunNumber(int run);
    /// Specify the number of events collected
    virtual void SetLastEventNum(int num = -1);
    /// Specify the number of events already processed
    virtual void SetMonitoredEventsNum(int num = -1);

    /// Launch a "Open file" dialog to load a RAW/ROOT file and reproduce its
    /// monitoring
    virtual void OpenFileDialog() = 0;
    /// Launch a "Save as..." dialog to save all monitored objects into a ROOT
    /// file
    virtual void SaveFileDialog() = 0;

    /// Load all monitored object from a RAW/ROOT file
    virtual void LoadFile(const char *filename);
    /// Save all monitored objects into a ROOT file
    virtual void SaveFile(const char *filename);

    /// Clean all monitored objects before a new run
    void ClearMonitors();

    /// Add a new monitor to the stack, as a simple TObject-derivative
    template <typename T, typename... Args>
    T *Book(const std::string &path, const std::string &name, Args &&...args) {
      if (m_objects.count(path) != 0)
        return Get<T>(path);
      auto &mon = m_objects[path];
      mon.object = new T(std::forward<Args>(args)...);
      GetFolder(path)->Add(mon.object);
      MapCanvas();
      UpdateMonitorsList();
      return dynamic_cast<T *>(mon.object);
    }
    /// Retrieve a monitored object by its path and type
    template <typename T> T *Get(const std::string &name) {
      return dynamic_cast<T *>(Get(name));
    }
    /// Retrieve a monitored object by its path
    TObject *Get(const std::string &name);
    TFolder *GetFolder(const std::string &path, TFolder *base = nullptr,
                       bool full = false);
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
    /// Add a summary page including several monitors
    void AddSummary(const std::string &path, const TObject *obj);

    // slots
    void UpdateAll();           ///< Refresh the displayed monitor(s)
    void SwitchUpdate(bool);    ///< Turn on/off the auto-refresh
    void SwitchClearRuns(bool); ///< Toggle cleaning between runs
    void Quit();                ///< Clean up everything
    void FillFromRAWFile(const char *path); ///< Reprocess RAW file

    struct MonitoredObject {
      TObject *object{nullptr};
      bool persist{true};
      double min_y{kInvalidValue}, max_y{kInvalidValue};
      std::string time_series{};
      Option_t *draw_opt{};
    };

  protected:
    static constexpr double kInvalidValue = 42.424242;

    virtual void Update() {} ///< Update all the widgets
    virtual void UpdateMonitorsList() {}
    virtual void MapCanvas() {}

    void CleanObject(TObject *);
    void FillFileObject(const std::string &path, TObject *obj,
                        const std::string &path_par = "");
    void Draw();
    void PostDraw();

    std::string GetFolderPath(const TFolder *obj) const;
    std::string GetPath(const TObject *obj) const;
    MonitoredObject &GetMonitor(const TObject *obj);

    std::map<std::string, TFolder *> m_dirs;
    std::map<std::string, MonitoredObject> m_objects; ///< Objects monitored
    std::map<TFolder *, std::vector<MonitoredObject *>> m_summ_objects;
    std::vector<MonitoredObject *> m_drawable; ///< Objects drawn on canvas

    TApplication *m_parent{nullptr};              ///< Parent owning application
    Status::State m_status{Status::STATE_UNINIT}; ///< Current "FSM" status
    int m_run_number{-1};
    unsigned long long m_last_event{0ull};
    unsigned long long m_last_event_mon{0ull};
    bool m_canv_needs_refresh{true};
    bool m_clear_between_runs{true};

    std::unique_ptr<TFolder> m_folder{nullptr};

  private:
    std::unique_ptr<TTimer> m_refresh_timer{nullptr}; ///< Plots refresh
  };
} // namespace eudaq

#endif
