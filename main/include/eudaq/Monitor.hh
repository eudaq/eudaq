#ifndef EUDAQ_INCLUDED_Monitor
#define EUDAQ_INCLUDED_Monitor

#include "eudaq/counted_ptr.hh"
#include "eudaq/DetectorEvent.hh"
#include "eudaq/CommandReceiver.hh"
#include "eudaq/FileSerializer.hh"
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
    Monitor(const std::string & name, const std::string & runcontrol, const std::string & datafile = "");
    virtual ~Monitor() {}

    bool ProcessEvent();
    virtual void OnIdle();

    virtual void OnEvent(counted_ptr<DetectorEvent> ev) = 0;
    virtual void OnBadEvent(counted_ptr<Event> /*ev*/) {}
    virtual void OnStartRun(unsigned param);
    virtual void OnStopRun();

    counted_ptr<DetectorEvent> LastBore() const { return m_lastbore; }
  protected:
    unsigned m_run;
    std::string m_datafile;
    bool m_callstart;
    counted_ptr<FileDeserializer> m_ser;
    counted_ptr<DetectorEvent> m_lastbore;
  };

}

#endif // EUDAQ_INCLUDED_Monitor
