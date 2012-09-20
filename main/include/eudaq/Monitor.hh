#ifndef EUDAQ_INCLUDED_Monitor
#define EUDAQ_INCLUDED_Monitor

#include "eudaq/counted_ptr.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/CommandReceiver.hh"
#include "eudaq/FileReader.hh"
#include <string>

namespace eudaq {

  /**
   * The base class from which all Monitors should inherit.
   */
  class Monitor : public CommandReceiver {
  public:
    /**
     * The constructor.
     * \param runcontrol A string containing the address of the RunControl to connect to.
     */
    Monitor(const std::string & name, const std::string & runcontrol, const unsigned lim,
            const unsigned skip_, const unsigned int skip_evts, const std::string & datafile = "");
    virtual ~Monitor() {}

    bool ProcessEvent();
    virtual void OnIdle();

    virtual void OnEvent(const StandardEvent & /*ev*/ ) {};
    virtual void OnBadEvent(counted_ptr<Event> /*ev*/) {}
    virtual void OnStartRun(unsigned param);
    virtual void OnStopRun();

    counted_ptr<DetectorEvent> LastBore() const { return m_lastbore; }
  protected:
    unsigned m_run;
    bool m_callstart;
    counted_ptr<FileReader> m_reader;
    counted_ptr<DetectorEvent> m_lastbore;
    unsigned limit;
    unsigned skip;
    unsigned int skip_events_with_counter;
    unsigned int counter_for_skipping;
  };

}

#endif // EUDAQ_INCLUDED_Monitor
