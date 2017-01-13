#ifndef EUDAQ_INCLUDED_Monitor
#define EUDAQ_INCLUDED_Monitor

#include "eudaq/Event.hh"
#include "eudaq/CommandReceiver.hh"
#include "eudaq/FileReader.hh"
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

    void OnIdle() override;
    void OnStartRun() override;
    void OnStopRun() override;
    bool ProcessEvent();
    virtual void OnEvent(EventSPC /*ev*/) = 0;
    virtual void Exec() override;
  protected:
    bool m_callstart;
    bool m_done;
    FileReaderSP m_reader;
    unsigned limit;
    unsigned skip;
    unsigned int skip_events_with_counter;
    unsigned int counter_for_skipping;
  };
}

#endif // EUDAQ_INCLUDED_Monitor
