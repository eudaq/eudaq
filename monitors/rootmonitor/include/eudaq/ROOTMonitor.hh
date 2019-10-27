#include "eudaq/Monitor.hh"
#include "eudaq/ROOTMonitorWindow.hh"

#ifndef __CINT__
# include "TApplication.h"
# include "RQ_OBJECT.h"
#endif

class TH1D;
class TGraph;

namespace eudaq {
  class ROOTMonitor : public Monitor {
    static constexpr const char* NAME = "eudaq::ROOTMonitor";
    RQ_OBJECT(NAME)
  public:
    ROOTMonitor(const std::string & name, const std::string & title, const std::string & runcontrol);

    void LoadRAWFileAsync(const char* path);

    void DoInitialise() override;
    void DoConfigure() override;
    void DoStartRun() override;
    void DoStopRun() override;
    void DoTerminate() override;
    void DoReset() override;
    void DoReceive(eudaq::EventSP ev) override;

    virtual void AtInitialisation() {}
    virtual void AtConfiguration() {}
    virtual void AtRunStart() {}
    virtual void AtRunStop() {}
    virtual void AtEventReception(eudaq::EventSP ev) = 0;
    virtual void AtReset() {}

  private:
    void LoadRAWFile(const std::string& path);

    bool m_interrupt = false;
    std::unique_ptr<TApplication> m_app;
    std::future<void> m_daemon;
    std::vector<std::future<void> > m_daemon_load;
    unsigned long long m_num_evt_mon = 0ull;

    // global monitoring plots
    TH1D* m_glob_evt_reco_time, *m_glob_evt_num_subevt;
    TGraph* m_glob_evt_vs_ts, *m_glob_rate_vs_ts;
    unsigned long long m_glob_last_evt_ts = 0ull;

  protected:
    std::unique_ptr<ROOTMonitorWindow> m_monitor;
  };
}
