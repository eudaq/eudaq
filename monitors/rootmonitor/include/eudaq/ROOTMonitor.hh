#include "eudaq/Monitor.hh"
#include "eudaq/ROOTMonitorWindow.hh"

#include "TApplication.h"

namespace eudaq {
  class ROOTMonitor : public Monitor, public TApplication {
  public:
    ROOTMonitor(const std::string & name, const std::string & title, const std::string & runcontrol);

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

  protected:
    std::unique_ptr<ROOTMonitorWindow> m_monitor;

  private:
    std::future<void> m_daemon;
    unsigned long long m_num_evt_mon = 0ull;
  };
}
