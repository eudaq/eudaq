#include "EventSynchronisationDetectorEvents.hh"
#include "DetectorEvent.hh"
#include "PluginManager.hh"

#include <memory>

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

      event_queue_pop_TLU_event();



  }



  void syncToDetectorEvents::storeCurrentOrder()
  {
    auto tlu = std::dynamic_pointer_cast<TLUEvent>(getFirstTLUQueue().back());
    for (auto& event_queue:m_ProducerEventQueue)
    {
      if (event_queue==getFirstTLUQueue())
      {
        continue;
      }
      auto currentEvent = event_queue.back();
      eudaq::PluginManager::setCurrentTLUEvent(*currentEvent, *tlu);
    }

  }

  syncToDetectorEvents::syncToDetectorEvents(SyncBase::Parameter_ref sync) :Sync2TLU(sync)
  {

  }


  registerSyncClass(syncToDetectorEvents, "DetectorEvents");
}
