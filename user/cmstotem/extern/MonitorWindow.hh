#ifndef MonitorWindow_hh
#define MonitorWindow_hh

#ifndef __CINT__
# include "TGFrame.h"
# include "TGStatusBar.h"
# include "TGListTree.h"
#endif

class TApplication;

class MonitorWindow : public TGMainFrame {
public:
  MonitorWindow(TApplication*, const std::string& name);
  ~MonitorWindow();

  void SetRunNumber(int run);
  void Update();
  void Quit();

private:
  static constexpr const char* NAME = "MonitorWindow";

  // ROOT GUI objects handled
  TGHorizontalFrame* m_top_win;
  TGVerticalFrame* m_left_bar;
  TGStatusBar* m_status_bar;
  TGCanvas* m_left_canv;
  TApplication* m_parent;

  ClassDef(MonitorWindow, 0);
};

#endif
