#include "eudaq/Monitor.hh"
#include "eudaq/ROOTMonitorWeb.hh"    // includes in header to avoid
#include "eudaq/ROOTMonitorWindow.hh" // user to include them later

#ifndef __CINT__
#include <RQ_OBJECT.h>
#include <TApplication.h>
#endif

class TH1D;
class TGraph;

namespace eudaq {
  class ROOTMonitor : public Monitor {
    static constexpr const char *NAME = "eudaq::ROOTMonitor";
    RQ_OBJECT(NAME)
  public:
    explicit ROOTMonitor(const std::string &name, const std::string &title,
                const std::string &runcontrol, bool web = false);

    // signals
    void LoadRAWFileAsync(const char *path);

    // EUDAQ-overriden methods
    void DoInitialise() override final;
    void DoConfigure() override final;
    void DoStartRun() override final;
    void DoStopRun() override final;
    void DoTerminate() override final;
    void DoReset() override final;
    void DoReceive(eudaq::EventSP ev) override final;

    // user-overridable methods
    virtual void AtInitialisation() {}
    virtual void AtConfiguration() {}
    virtual void AtRunStart() {}
    virtual void AtRunStop() {}
    virtual void AtEventReception(eudaq::EventSP ev) = 0;
    virtual void AtReset() {}

  private:
    void LoadRAWFile(const std::string &path);

    bool m_interrupt = false;
    std::unique_ptr<TApplication> m_app;
    std::future<void> m_daemon;
    std::vector<std::future<void>> m_daemon_load;
    unsigned long long m_num_evt_mon = 0ull;

    // global monitoring plots
    TH1D *m_glob_evt_reco_time, *m_glob_evt_num_subevt;
    TGraph *m_glob_evt_vs_ts, *m_glob_rate_vs_ts;
    unsigned long long m_glob_last_evt_ts = 0ull;

  protected:
    std::unique_ptr<ROOTMonitorBaseWindow> m_monitor;
  };
} // namespace eudaq
