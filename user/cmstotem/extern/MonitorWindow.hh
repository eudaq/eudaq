#ifndef MonitorWindow_hh
#define MonitorWindow_hh

#ifndef __CINT__
# include "TGFrame.h"
# include "TGStatusBar.h"
# include "TGListTree.h"
# include "TGToolBar.h"
# include "TContextMenu.h"
# include "TRootEmbeddedCanvas.h"
# include "RQ_OBJECT.h"
#endif
#include <functional>

class TApplication;
class TTree;
namespace eudaq{ class Event; }

class MonitorWindow : public TGMainFrame {
  static constexpr const char* NAME = "MonitorWindow";
  RQ_OBJECT(NAME)
public:
  MonitorWindow(TApplication*, const std::string& name);
  ~MonitorWindow();

  /// List of FSM states
  enum class Status { idle, configured, running, error };
  friend std::ostream& operator<<(std::ostream&, const Status&);

  /// Reset the status bar events counters
  void ResetCounters();
  void SetRunNumber(int run);
  void SetLastEventNum(int num);
  void SetMonitoredEventsNum(int num);

  /// Update the FSM
  void SetStatus(Status st);
  void SaveFile();
  void CleanMonitors();

  /// Add a new monitor to the stack, as a simple TObject-derivative
  template<typename T, typename... Args> T* Book(const std::string& path, const std::string& name, Args&&... args) {
    if (m_objects.count(path) != 0)
      return Get<T>(path);
    auto obj = new T(std::forward<Args>(args)...);
    auto item = m_tree_list->AddItem(BookStructure(path), name.c_str());
    auto it_icon = m_obj_icon.find(obj->ClassName());
    if (it_icon != m_obj_icon.end())
      item->SetPictures(it_icon->second, it_icon->second);
    m_objects[path] = MonitoredObject{item, obj, true, ""};
    m_left_canv->MapSubwindows();
    m_left_canv->MapWindow();
    return obj;
  }
  /// Retrieve a monitored object by its path and type
  template<typename T> T* Get(const std::string& name) {
    return dynamic_cast<T*>(Get(name));
  }
  /// Retrieve a monitored object by its path
  TObject* Get(const std::string& name);
  void SetPersistant(const TObject* obj, bool pers = true);
  void SetDrawOptions(const TObject* obj, Option_t* opt);

  void DrawElement(TGListTreeItem*, int);
  void DrawMenu(TGListTreeItem*, int, int, int);

  void Update();
  void SwitchUpdate(bool);
  void Quit();

private:
  /// List of status bar attributes
  enum class StatusBarPos { status = 0, run_number, tot_events, an_events, num_parts };
  TGListTreeItem* BookStructure(const std::string& path, TGListTreeItem* par = nullptr);

  // ROOT GUI objects handled
  TGHorizontalFrame* m_top_win;
  TGVerticalFrame* m_left_bar;
  TGStatusBar* m_status_bar;
  TGCanvas* m_left_canv;
  TGToolBar* m_toolbar;
  TGButton* m_button_save, *m_button_clean;
  TRootEmbeddedCanvas* m_main_canvas;
  TGListTree* m_tree_list;
  TContextMenu* m_context_menu;

  const TGPicture* m_icon_db, *m_icon_save, *m_icon_del;
  const TGPicture* m_icon_th1, *m_icon_th2, *m_icon_tgraph, *m_icon_track;

  /// Timer for auto-refresh loop
  std::unique_ptr<TTimer> m_timer;
  struct MonitoredObject {
    TGListTreeItem* item = nullptr;
    TObject* object = nullptr;
    bool persist = true;
    Option_t* draw_opt = "";
  };
  /// List of all objects handled and monitored
  std::map<std::string, MonitoredObject> m_objects;
  std::map<std::string, TGListTreeItem*> m_dirs;
  /// List of all summary plots sharing one canvas
  std::map<std::string, std::vector<std::string> > m_summaries;
  std::map<std::string, const TGPicture*> m_obj_icon = {
    {"TH1", m_icon_th1}, {"TH1F", m_icon_th1}, {"TH1D", m_icon_th1}, {"TH1I", m_icon_th1},
    {"TH2", m_icon_th2}, {"TH2F", m_icon_th2}, {"TH2D", m_icon_th2}, {"TH2I", m_icon_th2},
    {"TGraph", m_icon_tgraph}, {"TMultiGraph", m_icon_track}
  };
  /// List of all objects to be drawn on main canvas
  std::vector<MonitoredObject*> m_drawable;

  /// Parent owning application
  TApplication* m_parent = nullptr;
  /// Current "FSM" status
  Status m_status = Status::idle;

  ClassDef(MonitorWindow, 0);
};

#endif
