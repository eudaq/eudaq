#include "EventSynchronisationBase.hh"

#include <queue>

namespace eudaq{
  
  class OptionParser;

  class NoSync : public eudaq::SyncBase{
  public:
    typedef std::queue<std::shared_ptr<eudaq::Event>> eventqueue_t;
    // using Detectorevent_sp = std::shared_ptr < eudaq::DetectorEvent > ;

    //////////////////////////////////////////////////////////////////////////
    // public interface


    virtual bool pushEvent(event_sp ev, size_t Index = 0) override;
    virtual bool getNextEvent(event_sp& ev) override;
    virtual bool OutputIsEmpty() const override;
    virtual bool InputIsEmpty() const override;
    virtual bool InputIsEmpty(size_t fileId) const override;

    virtual bool mergeBoreEvent(event_sp& ev);
    //////////////////////////////////////////////////////////////////////////


    NoSync(Parameter_ref);
    virtual ~NoSync() {}

  private:
    eventqueue_t m_queue;
  };

  NoSync::NoSync(Parameter_ref ConfParameter) :SyncBase(ConfParameter)
  {

  }

  bool NoSync::pushEvent(event_sp ev, size_t Index /*= 0*/)
  {
    if (!ev)
    {
      return false;
    }

    m_queue.push(std::move(ev));
    return true;
  }

  bool NoSync::getNextEvent(event_sp& ev)
  {
    if (OutputIsEmpty())
    {
      return false;
    }

    ev = m_queue.front();
    m_queue.pop();

    return true;
  }

  bool NoSync::OutputIsEmpty() const
  {
    return m_queue.empty();
  }

  bool NoSync::InputIsEmpty() const
  {
    return m_queue.empty();
  }

  bool NoSync::InputIsEmpty(size_t fileId) const
  {
    return m_queue.empty();
  }

  bool NoSync::mergeBoreEvent(event_sp& ev)
  {
    if (!ev)
    {
      if (! (m_queue.front()->IsBORE())){
        return false;
      }

      getNextEvent(ev);
    }
    return true;
  }
  registerSyncClass(NoSync, "noSync");
}
