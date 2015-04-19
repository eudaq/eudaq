#include "eudaq/EventSynchronisationBase.hh"
#include "eudaq/Event.hh"
#include "eudaq/PointerEvent.hh"
#include <string>
#include <queue>
#include "eudaq/DetectorEvent.hh"


namespace eudaq{

  class Event2Pointer : public SyncBase {
  public:

    using reference_t = PointerEvent::reference_t;
    using  counter_t = PointerEvent::counter_t;
   // public interface

    Event2Pointer(Parameter_ref param):SyncBase(param){}
    virtual bool pushEvent(event_sp ev, size_t Index = 0) ;
    virtual bool getNextEvent(event_sp& ev);
     virtual bool OutputIsEmpty() const;
    virtual bool InputIsEmpty() const;
    virtual bool InputIsEmpty(size_t FileID) const;


  private:
    bool InternalPushEvent(event_sp&& ev);
    bool isDataEvent(const Event& ev);
    bool pushDetectorEvent(event_sp&& ev);
    reference_t pushDataEvent(event_sp&& ev);
    bool pushPointerEvent(event_sp&& ev);
    event_sp replaceReferenceByDataEvent(const Event& pev);
    bool ReferenceFound(const Event& ev);
    counter_t externalReference(const event_sp& ev);
    void removeReference(const Event& ev);
   std::vector<reference_t> m_buffer;
    std::queue<event_sp> m_output;
    reference_t m_ref=0;
    static const counter_t m_internalRef = 2;
  };



  bool Event2Pointer::isDataEvent(const Event& ev){
    return ev.get_id() != PointerEvent::eudaq_static_id();
  }

  bool Event2Pointer::InputIsEmpty() const
  {
    return m_buffer.empty();
  }

  bool Event2Pointer::InputIsEmpty(size_t FileID) const
  {
    return InputIsEmpty();
  }

  bool Event2Pointer::OutputIsEmpty() const
  {
    return m_output.empty();
  }


  bool Event2Pointer::pushDetectorEvent(event_sp&& ev)
  {
    DetectorEvent* det = dynamic_cast<DetectorEvent*>(ev.get());
      if (!det)
      {
      return false;
      }
      std::vector<event_sp> dummy;
    for (size_t i = 0; i < det->NumEvents(); ++i)
    {
      auto subPointer = det->GetEventPtr(i);
      counter_t counter = externalReference(subPointer);
      auto ref = InternalPushEvent(std::move(subPointer));
      dummy.push_back(std::make_shared<PointerEvent> (ref, counter));
    }

    det->clearEvents();

    for (auto& e:dummy)
    {
      det->AddEvent(e);
    }
    counter_t counter = externalReference(ev);
    auto ref = pushDataEvent(std::move(ev));
    
    pushDataEvent(std::move(std::make_shared<PointerEvent>(ref,counter)));

  }

  Event2Pointer::reference_t Event2Pointer::pushDataEvent(event_sp&& ev)
  {
    if (!ReferenceFound(*ev))
    {
      m_buffer.push_back(ev->getUniqueID());
      m_output.push(ev);
    }
    if (!externalReference(ev))
    {
      removeReference(*ev);
    }

    return ev->getUniqueID();

  }

  bool Event2Pointer::ReferenceFound(const Event& ev)
  {
    return (std::find(m_buffer.begin(), m_buffer.end(), ev.getUniqueID())  != m_buffer.end());
  }

  void Event2Pointer::removeReference(const Event& ev)
  {
    auto it = std::find(m_buffer.begin(), m_buffer.end(), ev.getUniqueID());
    if (it != m_buffer.end()){
      m_buffer.erase(it);
    }
  }

  Event2Pointer::counter_t Event2Pointer::externalReference(const event_sp& ev)
  {
   
    return (ev.use_count() - m_internalRef);
  }

  bool Event2Pointer::pushEvent(event_sp ev, size_t Index /*= 0*/)
  {
    return InternalPushEvent(std::move(ev));
  }

  bool Event2Pointer::InternalPushEvent(event_sp&& ev)
  {
    if (ev->get_id() == DetectorEvent::eudaq_static_id()){
      return pushDetectorEvent(std::move(ev));
    }

    return pushDataEvent(std::move(ev));
  }

  bool Event2Pointer::getNextEvent(event_sp& ev)
  {
    if (OutputIsEmpty())
    {
      return false;
    }
    ev = m_output.front();
    m_output.pop();
    
    return true;
  }

  registerSyncClass(Event2Pointer, "dePointer");
}