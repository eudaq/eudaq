#include "EventSynchronisationMultiTSEvents.hh"

#include <memory>


#define ADDTLUMarker    0x8000000000000000
#define RemoveTLUMarker 0x7FFFFFFFFFFFFFFF

const char* tlu_trigger = "TLU_trigger";

uint64_t AddTLUMarker2TimeStamp(uint64_t timestamp){
  return timestamp | ADDTLUMarker;

}

uint64_t RemoveTLUMarkerFromTimeStamp(uint64_t timeStamp){
  return timeStamp & RemoveTLUMarker;
}

bool isTLUmarkedTimeStamp(uint64_t timeStamp){

  return timeStamp > ADDTLUMarker;
}

using namespace std;




namespace eudaq{



  void syncToMultiTSEvents::Process_Event_is_sync(std::shared_ptr<Event> ev, eudaq::Event const & tlu)
  {

    int nr_tlutrigger = 0;
    nr_tlutrigger = ev->GetTag(tlu_trigger, nr_tlutrigger);
    ev->SetTag("TLU_trigger", ++nr_tlutrigger);
    ev->pushTimeStamp(tlu.GetTimestamp());


  }

  void syncToMultiTSEvents::Process_Event_is_late(std::shared_ptr<Event> ev, eudaq::Event const & tlu)
  {

    int nr_tlutrigger = 0;
    nr_tlutrigger = ev->GetTag(tlu_trigger, nr_tlutrigger);


    if (nr_tlutrigger>0)
    {

      shared_ptr<DetectorEvent> det = make_shared<DetectorEvent>(ev->GetRunNumber(), ev->GetEventNumber(), ev->GetTimestamp(1));
    
      det->AddEvent(ev);

      m_outPutQueue.push(det);

     
    }





  }

  syncToMultiTSEvents::syncToMultiTSEvents(SyncBase::Parameter_ref sync) :Sync2TLU(sync)
  {

  }

  void syncToMultiTSEvents::makeDetectorEvent()
  {



      for (auto& e : m_ProducerEventQueue)
      {
        m_outPutQueue.push(e.front());
        e.pop();
      }
      event_queue_pop();


  }


  registerSyncClass(syncToMultiTSEvents, "aida");
}
