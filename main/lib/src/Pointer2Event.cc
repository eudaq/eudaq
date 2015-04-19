#include "eudaq/EventSynchronisationBase.hh"
#include "eudaq/Event.hh"
#include "eudaq/PointerEvent.hh"
#include <string>
#include <queue>
#include "eudaq/DetectorEvent.hh"


namespace eudaq{

  class Pointer2Event : public SyncBase {
  public:

    using reference_t = PointerEvent::reference_t;
   // public interface

    Pointer2Event(Parameter_ref param);
    virtual bool pushEvent(event_sp ev, size_t Index = 0) ;
    virtual bool getNextEvent(event_sp& ev);
    virtual bool OutputIsEmpty() const;
    virtual bool InputIsEmpty() const;
    virtual bool InputIsEmpty(size_t FileID) const;


  private:
    
    bool isDataEvent(const Event& ev);
    bool detectorEventReplacePointer(event_sp& ev);
    event_sp replaceReferenceByDataEvent(const Event& pev);
    std::map< reference_t, event_sp> m_buffer;
    std::queue<event_sp> m_output;

  };



  bool Pointer2Event::isDataEvent(const Event& ev){
    return ev.get_id() != PointerEvent::eudaq_static_id();
  }

  bool Pointer2Event::InputIsEmpty() const
  {
    return m_buffer.empty();
  }

  bool Pointer2Event::InputIsEmpty(size_t FileID) const
  {
    return InputIsEmpty();
  }

  bool Pointer2Event::OutputIsEmpty() const
  {
    return m_output.empty();
  }

  bool Pointer2Event::pushEvent(event_sp ev, size_t Index /*= 0*/)
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


      m_buffer[ev->getUniqueID()] = ev;
      return true;
    }

    event_sp outevent = replaceReferenceByDataEvent(*ev);

 
    m_output.push(outevent);
    return true;
  }

  event_sp Pointer2Event::replaceReferenceByDataEvent(const Event &ev)
  {
    const PointerEvent& pev = dynamic_cast<const PointerEvent&> (ev);
    event_sp dummy = m_buffer[pev.getReference()];
    if (pev.getCounter()==0)
    {
     // m_buffer.erase(pev.getReference());
    }
    detectorEventReplacePointer(dummy);
    return dummy;
  }

  bool Pointer2Event::getNextEvent(event_sp& ev)
  {
    if (OutputIsEmpty())
    {
      return false;
    }

    ev = m_output.front();
    m_output.pop();
    return true;
  }

  Pointer2Event::Pointer2Event(Parameter_ref param) :SyncBase(param)
  {

  }

  bool Pointer2Event::detectorEventReplacePointer(event_sp& ev)
  {
    if (ev->get_id() != DetectorEvent::eudaq_static_id())
    {
      return true; // nothing to do
    }

    DetectorEvent *det = dynamic_cast<DetectorEvent*> (ev.get());
    std::vector<event_sp> dummy_vec;
    for (size_t i = 0; i < det->NumEvents(); ++i)
    {
      auto subEvent = det->GetEvent(i);
      dummy_vec.push_back(replaceReferenceByDataEvent(*subEvent));
    }
    det->clearEvents();
    for (auto& e : dummy_vec)
    {
      det->AddEvent(e);
    }

    return true;
  }

  registerSyncClass(Pointer2Event, "Pointer2Events");
}