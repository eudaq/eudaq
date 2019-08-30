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

  enum class Status { idle, configured, running, error };
  friend std::ostream& operator<<(std::ostream&, const Status&);

  void ResetCounters();
  void SetRunNumber(int run);
  void SetStatus(Status st);
  void SetTree(TTree*);
  void SaveTree();

  /*template<typename T, typename E> using FillingMethod = std::function<void(T*,E*)>;
  template<typename T, typename E, typename... Args> void Book(const FillingMethod<T,E>& meth, const char* path, Args&&... args) {
    //m_objects.emplace_back(std::make_pair(new T(std::forward<Args>(args)...), meth));
    m_objects.emplace_back(std::pair<TObject*,FillingMethod<TObject,eudaq::Event> >(new T(std::forward<Args>(args)...), meth));
  }*/
  //template<typename T> using FillingMethod = std::function<void(T*,eudaq::Event*)>;
  template<typename T, typename... Args> T* Book(const char* path, Args&&... args) {
    auto obj = new T(std::forward<Args>(args)...);
    auto item = m_tree_list->AddItem(nullptr, path);
    auto it_icon = m_obj_icon.find(obj->ClassName());
    if (it_icon != m_obj_icon.end())
      item->SetPictures(it_icon->second, it_icon->second);
    m_objects[path] = std::make_pair(item, obj);
    return obj;
  }
  template<typename T> T* Get(const char* name) {
    return dynamic_cast<T*>(Get(name));
  }
  TObject* Get(const char* name);

  void DrawElement(TGListTreeItem*, int);
  void DrawMenu(TGListTreeItem*, int, int, int);

  void Update();
  void SwitchUpdate(bool);
  void Quit();

private:
  enum class StatusBarPos { status = 0, run_number, tot_events, an_events, num_parts };

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
  const TGPicture* m_icon_th1, *m_icon_th2, *m_icon_tgraph;

  std::unique_ptr<TTimer> m_timer;
  std::map<std::string, std::pair<TGListTreeItem*, TObject*> > m_objects;
  std::map<std::string, const TGPicture*> m_obj_icon = {
    {"TH1", m_icon_th1}, {"TH1F", m_icon_th1}, {"TH1D", m_icon_th1}, {"TH1I", m_icon_th1},
    {"TH2", m_icon_th2}, {"TH2F", m_icon_th2}, {"TH2D", m_icon_th2}, {"TH2I", m_icon_th2},
    {"TGraph", m_icon_tgraph}
  };
  std::vector<TObject*> m_drawable;
  //std::vector<std::pair<TObject*, FillingMethod<TObject,eudaq::Event> > > m_objects;
  //std::vector<std::pair<TObject*, std::function<void(TObject*,eudaq::Event*)> > > m_objects;

  TApplication* m_parent = nullptr;
  TTree* m_tree = nullptr;

  Status m_status = Status::idle;

  ClassDef(MonitorWindow, 0);
};

#endif
