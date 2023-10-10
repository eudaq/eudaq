#ifndef EUDAQ_INCLUDED_ROOTMonitorWindow
#define EUDAQ_INCLUDED_ROOTMonitorWindow

#include "eudaq/ROOTMonitorBaseWindow.hh"

#ifndef __CINT__
#include <RQ_OBJECT.h>
#include <TContextMenu.h>
#include <TGButton.h>
#include <TGFrame.h>
#include <TGListTree.h>
#include <TGStatusBar.h>
#include <TGToolBar.h>
#include <TRootEmbeddedCanvas.h>
#endif
#include <functional>

class TApplication;

namespace eudaq {
  class Event;
  /// A placeholder for ROOT monitoring
  class ROOTMonitorWindow final : public TGMainFrame,
                                  public ROOTMonitorBaseWindow {
    static constexpr const char *NAME = "eudaq::ROOTMonitorWindow";
    RQ_OBJECT(NAME)
  public:
    explicit ROOTMonitorWindow(TApplication *, const std::string &name);

    void SetStatus(Status::State st) override;
    void ResetCounters() override;
    void SetRunNumber(int run) override;
    void SetLastEventNum(int num = -1) override;
    void SetMonitoredEventsNum(int num = -1) override;

    void LoadFile(const char *filename) override;
    void OpenFileDialog() override;
    void SaveFileDialog() override;

    /// Action triggered when a hierarchised monitor structure is to be shown
    void DrawMenu(TGListTreeItem *, int, int, int);

    void Update(); ///< Refresh the displayed monitor(s)
    void Quit();   ///< Clean up everything before terminating the application
    void FillFromRAWFile(const char *path); ///< Process monitors from RAW file

  private:
    /// List of status bar attributes
    enum class StatusBarPos {
      status = 0,
      run_number,
      tot_events,
      an_events,
      num_parts
    };
    void MapCanvas() override;
    void Draw(TCanvas *canv);
    void PostDraw(TCanvas *canv);

    // ROOT GUI objects handled
    TGHorizontalFrame *m_top_win{nullptr};
    TGVerticalFrame *m_left_bar{nullptr};
    TGStatusBar *m_status_bar{nullptr};
    TGCanvas *m_left_canv{nullptr};
    TGToolBar *m_toolbar{nullptr};
    TGButton *m_button_open{nullptr}, *m_button_save{nullptr},
        *m_button_clean{nullptr};
    TRootEmbeddedCanvas *m_main_canvas{nullptr};
    TGCheckButton *m_update_toggle{nullptr}, *m_refresh_toggle{nullptr};
    TGListTree *m_tree_list{nullptr};
    TContextMenu *m_context_menu{nullptr};

    ClassDef(ROOTMonitorWindow, 0);
  };
} // namespace eudaq

#endif
