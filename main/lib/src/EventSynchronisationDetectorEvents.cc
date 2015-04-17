#include "eudaq/EventSynchronisationDetectorEvents.hh"
#include "eudaq/DetectorEvent.hh"


#include <memory>
#include "eudaq/PluginManager.hh"


using namespace std;


namespace eudaq{

  

  void syncToDetectorEvents::makeDetectorEvent()
  {
    
    auto &TLU = getFirstTLUQueue().front();
    shared_ptr<DetectorEvent> det = make_shared<DetectorEvent>(TLU->GetRunNumber(), TLU->GetEventNumber(), TLU->GetTimestamp());
    for (auto&e : m_ProducerEventQueue){
      if (!e.empty())
      {
        det->AddEvent(e.front());
      }
      else{
        det->AddEvent(shared_ptr<Event>(nullptr));
      }
    }
    
    m_outPutQueue.push(det);

    if (m_sync)
    {
      event_queue_pop_TLU_event();
    }
    else
    {
      event_queue_pop();
    }



  }



  void syncToDetectorEvents::storeCurrentOrder()
  {
    shared_ptr<TLUEvent> tlu = std::dynamic_pointer_cast<TLUEvent>(getFirstTLUQueue().back());
    for (size_t i = 1; i < m_ProducerEventQueue.size(); ++i)
    {
      shared_ptr<Event> currentEvent = m_ProducerEventQueue[i].back();
      eudaq::PluginManager::setCurrentTLUEvent(*currentEvent, *tlu);
    }

  }

  syncToDetectorEvents::syncToDetectorEvents(SyncBase::Parameter_ref sync) :Sync2TLU(sync)
  {

  }


  registerSyncClass(syncToDetectorEvents, "DetectorEvents");
}