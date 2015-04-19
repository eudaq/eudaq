#include "eudaq/EventSynchronisationBase.hh"
#include "eudaq/Event.hh"
#include "eudaq/PointerEvent.hh"
#include <string>
#include <queue>
#include "eudaq/DetectorEvent.hh"
#include "eudaq/PluginManager.hh"


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
    reference_t InternalPushEvent(event_sp&& ev);
    bool isDataEvent(const Event& ev);
    reference_t pushDetectorEvent(event_sp&& ev);
    reference_t pushDataEvent(event_sp&& ev);
    bool pushPointerEvent(event_sp&& ev);
    event_sp replaceReferenceByDataEvent(const Event& pev);
    
    void removeReference(const Event& ev);
    bool bufferEmpty() const;
    void setCounterForPointerEvents(Event* ev);

    struct Buffer_struckt 
    {
      std::vector<reference_t> m_buffer;
      std::queue<event_sp> m_output;
      counter_t externalReference(const Event& ev) const;
      counter_t externalReference(reference_t ev_id) const;
      bool ReferenceFound(const Event& ev);
      reference_t push(event_sp && ev);
      reference_t push(reference_t ev);
      size_t size()const;
      void pop_front(event_sp& ev);
      void setCounterForPointerEvents(Event* ev);
    } m_buf;

    reference_t m_ref=0;
    static const counter_t m_internalRef = 2;
    size_t m_bufferSize = 100;
  };

  bool Event2Pointer::Buffer_struckt::ReferenceFound(const Event& ev)
  {
    if (ev.get_id()==PointerEvent::eudaq_static_id())
    {
      return false;
    }
    return  (std::find(m_buffer.begin(), m_buffer.end(), ev.getUniqueID()) != m_buffer.end());
  }

  Event2Pointer::counter_t Event2Pointer::Buffer_struckt::externalReference(reference_t ev_id) const
  {
    return  (std::find(m_buffer.begin(), m_buffer.end(), ev_id) != m_buffer.end());
  }

  Event2Pointer::counter_t Event2Pointer::Buffer_struckt::externalReference(const Event& ev) const
  {
    return externalReference(ev.getUniqueID());
  }

  Event2Pointer::reference_t Event2Pointer::Buffer_struckt::push(event_sp && ev)
  {
    reference_t id = ev->getUniqueID();
    m_buffer.push_back(id);
    m_output.push(ev);
    return id;
  }

  Event2Pointer::reference_t Event2Pointer::Buffer_struckt::push(reference_t id)
  {
    m_buffer.push_back(id);
    m_output.push(nullptr);
    return id;
  }

  size_t Event2Pointer::Buffer_struckt::size() const
  {
    return m_buffer.size();
  }



  void Event2Pointer::Buffer_struckt::pop_front(event_sp& ev)
  {
    do 
    {
      ev = m_output.front();
      m_output.pop();
      m_buffer.erase(m_buffer.begin());
    } while (!ev);
     
    setCounterForPointerEvents(ev.get());


    
  }

  void Event2Pointer::Buffer_struckt::setCounterForPointerEvents(Event* ev)
  {
    if (!ev)
    {
      return;
    }
    if (ev->get_id() == PointerEvent::eudaq_static_id())
    {
      auto pev = dynamic_cast<PointerEvent*>(ev);
      auto id = pev->getReference();
      auto mycount = std::count(m_buffer.begin(), m_buffer.end(), id);
      std::cout << mycount << std::endl;
      pev->setCounter(mycount);
    }
    if (ev->get_id() == DetectorEvent::eudaq_static_id())
    {
      auto det = dynamic_cast<DetectorEvent*>(ev);
      for (size_t i = 0; i < det->NumEvents(); ++i)
      {
        setCounterForPointerEvents(det->GetEventPtr(i).get());
      }
    }

  }



  bool Event2Pointer::isDataEvent(const Event& ev){
    return ev.get_id() != PointerEvent::eudaq_static_id();
  }

  bool Event2Pointer::InputIsEmpty() const
  {
    return true; // m_buffer.empty();
  }

  bool Event2Pointer::InputIsEmpty(size_t FileID) const
  {
    return InputIsEmpty();
  }

  bool Event2Pointer::OutputIsEmpty() const
  {
    return m_buf.size()==0;
  }

  bool Event2Pointer::bufferEmpty() const {
    return m_buf.size() < m_bufferSize;

  }

  Event2Pointer::reference_t Event2Pointer::pushDetectorEvent(event_sp&& ev)
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
      auto ref = InternalPushEvent(std::move(subPointer));
      dummy.push_back(std::make_shared<PointerEvent> (ref, 0));
    }

    det->clearEvents();

    for (auto& e:dummy)
    {
      det->AddEvent(e);
    }
   
    auto ref = pushDataEvent(std::move(ev));
    
    return pushDataEvent(std::move(std::make_shared<PointerEvent>(ref,0)));

  }

  Event2Pointer::reference_t Event2Pointer::pushDataEvent(event_sp&& ev)
  {
    if (!m_buf.ReferenceFound(*ev))
    {
      return m_buf.push(std::move(ev));
    }
    
    return m_buf.push(ev->getUniqueID());

  }

  bool Event2Pointer::pushEvent(event_sp ev, size_t Index /*= 0*/)
  {
    if (ev)
    {

     InternalPushEvent(std::move(ev));
     return true;
    }
    m_bufferSize = 0;
    return false;
  }

  Event2Pointer::reference_t Event2Pointer::InternalPushEvent(event_sp&& ev)
  {
    if (ev->get_id() == DetectorEvent::eudaq_static_id()){
      return pushDetectorEvent(std::move(ev));
    }

    return pushDataEvent(std::move(ev));
  }

  bool Event2Pointer::getNextEvent(event_sp& ev)
  {
    if (OutputIsEmpty() ||bufferEmpty() )
    {
      return false;
    }
    m_buf.pop_front(ev);
  
    return true;
  }



  registerSyncClass(Event2Pointer, "Events2Pointer");
}