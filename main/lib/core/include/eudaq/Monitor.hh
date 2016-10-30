#ifndef EUDAQ_INCLUDED_Monitor
#define EUDAQ_INCLUDED_Monitor

#include "eudaq/StandardEvent.hh"
#include "eudaq/CommandReceiver.hh"
#include "eudaq/RawFileReader.hh"
#include <string>
#include <memory>
namespace eudaq {

  /**
   * The base class from which all Monitors should inherit.
   */
  class DLLEXPORT Monitor : public CommandReceiver {
  public:
    /**
     * The constructor.
     * \param runcontrol A string containing the address of the RunControl to
     * connect to.
     */
    Monitor(const std::string &name, const std::string &runcontrol,
            const unsigned lim, const unsigned skip_,
            const unsigned int skip_evts, const std::string &datafile = "");
    virtual ~Monitor() {}

    bool ProcessEvent();
    virtual void OnIdle();

    virtual void OnEvent(const StandardEvent & /*ev*/){};
    virtual void OnBadEvent(EventSP /*ev*/) {}
    virtual void OnStartRun(unsigned param);
    virtual void OnStopRun();

  protected:
    unsigned m_run;
    bool m_callstart;
    std::shared_ptr<RawFileReader> m_reader;
    unsigned limit;
    unsigned skip;
    unsigned int skip_events_with_counter;
    unsigned int counter_for_skipping;
  };
}

#endif // EUDAQ_INCLUDED_Monitor
