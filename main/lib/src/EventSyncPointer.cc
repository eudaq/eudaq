#include "eudaq/EventSynchronisationBase.hh"
#include "eudaq/Event.hh"
#include "eudaq/PointerEvent.hh"
#include <string>
#include <queue>
#include "eudaq/DetectorEvent.hh"


namespace eudaq{

  class EventSyncPointer : public SyncBase {
  public:

    using reference_t = PointerEvent::reference_t;
   // public interface

    EventSyncPointer(Parameter_ref param);
    virtual bool pushEvent(Event_sp ev, size_t Index = 0) ;
    virtual bool getNextEvent(Event_sp& ev);
    virtual bool mergeBoreEvent(Event_sp& ev) { return false; };
    virtual bool OutputIsEmpty() const;
    virtual bool InputIsEmpty() const;
    virtual bool InputIsEmpty(size_t FileID) const;


  private:
    
    bool isDataEvent(const Event& ev);

    event_sp replaceReferenceByDataEvent(const Event& pev);
    std::map<reference_t, event_sp> m_buffer;
    std::queue<event_sp> m_output;
    reference_t m_ref=0;
  };



  bool EventSyncPointer::isDataEvent(const Event& ev){
    return ev.get_id() != PointerEvent::eudaq_static_id();
  }

  bool EventSyncPointer::InputIsEmpty() const
  {
    return m_buffer.empty();
  }

  bool EventSyncPointer::InputIsEmpty(size_t FileID) const
  {
    return InputIsEmpty();
  }

  bool EventSyncPointer::OutputIsEmpty() const
  {
    return m_output.empty();
  }

  bool EventSyncPointer::pushEvent(Event_sp ev, size_t Index /*= 0*/)
  {
    if (!ev)
    {
      if (!OutputIsEmpty())
      {
        return true;
      }
      return false;
    }
    if (isDataEvent(*ev))
    {
      ++m_ref;

      m_buffer[m_ref] = ev;
      return true;
    }

    event_sp outevent = replaceReferenceByDataEvent(*ev);

    if (outevent->get_id() == DetectorEvent::eudaq_static_id())
    {
      DetectorEvent *det = dynamic_cast<DetectorEvent*> (outevent.get());
      std::vector<event_sp> dummy_vec;
      for (size_t i = 0; i < det->NumEvents(); ++i)
      {
        auto subEvent = det->GetEvent(i);
        dummy_vec.push_back(replaceReferenceByDataEvent(*subEvent));
      }
      det->clearEvents();
      for (auto& e:dummy_vec)
      {
        det->AddEvent(e);
      }
    }

    m_output.push(outevent);
    return true;
  }

  event_sp EventSyncPointer::replaceReferenceByDataEvent(const Event &ev)
  {
    const PointerEvent& pev = dynamic_cast<const PointerEvent&> (ev);
    event_sp dummy = m_buffer[pev.getReference()];
    if (pev.getCounter()==0)
    {
      m_buffer.erase(pev.getReference());
    }
    return dummy;
  }

  bool EventSyncPointer::getNextEvent(Event_sp& ev)
  {
    if (OutputIsEmpty())
    {
      return false;
    }

    ev = m_output.front();
    m_output.pop();
    return true;
  }

  EventSyncPointer::EventSyncPointer(Parameter_ref param) :SyncBase(param)
  {

  }

  registerSyncClass(EventSyncPointer, "dePointer");
}