#include "EventSynchronisationCompareWithTLU.hh"

#include "Event.hh"
#include "PluginManager.hh"
#include "Configuration.hh"
#include "OptionParser.hh"
#include "Logger.hh"
#include "DataConverterPlugin.hh"

#include <iostream>
#include <memory>


#define FILEINDEX_OFFSET 10000
using std::cout;
using std::endl;
using std::shared_ptr;
using namespace std;
namespace eudaq{
  Sync2TLU::Sync2TLU(Parameter_ref sync) : SyncBase(sync)
  {


  }






  void Sync2TLU::addBORE_BaseEvent(unsigned int fileIndex, event_sp BOREEvent)
  {
    
    if (BOREEvent->IsPacket())
    {
      m_outPutQueue.push(BOREEvent);
      for (size_t i = 0; i < PluginManager::GetNumberOfEvents(*BOREEvent); ++i)
      {
        addBORE_BaseEvent(fileIndex, PluginManager::ExtractEventN(BOREEvent, i));
      }

    }
    else
    {
      addBORE_Event(fileIndex, BOREEvent);
    }

  }



  void Sync2TLU::addBORE_Event(unsigned int fileIndex, event_sp BOREEvent)
  {

    ++m_registertProducer;
    m_ProducerEventQueue.resize(m_registertProducer);


    if (m_ProducerProFileReader.size() < fileIndex + 1)
    {
      auto oldsize = m_ProducerProFileReader.size();
      m_ProducerProFileReader.resize(fileIndex+1);
      for (auto i = oldsize; i< fileIndex + 1; ++i){
        m_ProducerProFileReader[i] = 0;
      }
      
    }
    m_ProducerProFileReader[fileIndex]++;
    auto identifier = PluginManager::getUniqueIdentifier(*BOREEvent);
    if (identifier > FILEINDEX_OFFSET)
    {
      EUDAQ_THROW("The unique identifier for this Event is too large. Increase the value of FILEINDEX_OFFSET or return a smaller number from PluginManager::getUniqueIdentifier(BOREEvent)");
    }

    if (PluginManager::isTLU(*BOREEvent))
    {
      if (m_TLUs_found==0)
      {
      m_TLU_pos = m_event_id;
      }
     
      ++m_TLUs_found;
    }

    m_ProducerId2Eventqueue[getUniqueID(fileIndex, identifier)] = m_event_id++;


    size_t elements = 0;
    for (auto&e1:m_ProducerId2Eventqueue)
    {
      for (auto&e2 : m_ProducerId2Eventqueue)
      {
        if (e2.second==e1.second)
        {
          elements++;
        }

      }

    }

    if (elements!=m_ProducerId2Eventqueue.size())
    {
      EUDAQ_THROW("Duplication in the producer event queue id.");
    }
    m_Bore_buffer.push(BOREEvent);
  }

  int Sync2TLU::AddBaseEventToProducerQueue(int fileIndex, event_sp Ev)
  {
    if (!Ev)
    {
      return false;
    }
    if (Ev->IsPacket())
    {
      for (size_t i = 0; i < PluginManager::GetNumberOfEvents(*Ev); ++i)
      {
        AddBaseEventToProducerQueue(fileIndex, PluginManager::ExtractEventN(Ev, i));
      }

    }
    else
    {
      AddEventToProducerQueue(fileIndex, Ev);
    }
    return true;
  }
  

  int Sync2TLU::AddEventToProducerQueue(int fileIndex, event_sp Ev)
  {
    if (Ev)
    {
      auto identifier = PluginManager::getUniqueIdentifier(*Ev);
  
      try{
        auto &q = getQueuefromId(fileIndex, identifier);
        q.push(Ev);
      }
      catch (...){
        std::cerr << "\n error in Sync2TLU::AddEventToProducerQueue(int fileIndex, std::shared_ptr<eudaq::Event> Ev) \n unkown event id: " << identifier << "\n for the event: \n";
        Ev->Print(cout);
      }

    }
    return true;
  }

  bool Sync2TLU::Event_Queue_Is_Empty() const
  {
    for (auto& q : m_ProducerEventQueue)
    {
      if (q.empty())
      {
        return true;
      }
    }
    return false;
  }

  bool Sync2TLU::SubEventQueueIsEmpty(int FileID) const
  {
    if (m_ProducerEventQueue.empty())
    {
      return true;
    }
    for (auto& e : m_ProducerId2Eventqueue)
    {
      if (e.first >= getUniqueID(FileID, 0) && e.first < getUniqueID(FileID + 1, 0))
      {
        if (m_ProducerEventQueue[e.second].empty()){
          return true;
        }
      }
    }
    return false;
  }



  bool Sync2TLU::ProducerQueueExist(unsigned producerID)
  {
    if (m_ProducerId2Eventqueue.find(producerID) == m_ProducerId2Eventqueue.end()){

      return false;
    }


    if (m_ProducerEventQueue.size() <= m_ProducerId2Eventqueue[producerID]){
      return false;
    }


    return true;
  }

  Sync2TLU::eventqueue_t& Sync2TLU::getQueuefromId(unsigned producerID)
  {
    if (!ProducerQueueExist(producerID)){

      EUDAQ_THROW("unknown Producer ID " + std::to_string(producerID));
    }
    return m_ProducerEventQueue[m_ProducerId2Eventqueue[producerID]];
  }

  Sync2TLU::eventqueue_t& Sync2TLU::getQueuefromId(unsigned fileIndex, unsigned eventIndex)
  {
    return getQueuefromId(getUniqueID(fileIndex, eventIndex));
  }

  Sync2TLU::eventqueue_t& Sync2TLU::getFirstTLUQueue()
  {
    return m_ProducerEventQueue[m_TLU_pos];
  }


  bool Sync2TLU::SyncFirstEvent()
  {

    auto& TLU_queue = getFirstTLUQueue();

    while (!Event_Queue_Is_Empty())
    {
      if (compareTLUwithEventQueues(TLU_queue.front()))
      {
        makeDetectorEvent();
        return true;
      }
      else if (!Event_Queue_Is_Empty())
      {
        TLU_queue.pop();
      }
    }
    return false;
  }


  bool Sync2TLU::compareTLUwithEventQueue(event_sp& tlu_event, eudaq::Sync2TLU::eventqueue_t& event_queue)
  {
    int SyncStatus = Event_IS_Sync;

    while (!event_queue.empty())
    {
      auto currentEvent = event_queue.front();

      SyncStatus = CompareEvents(*currentEvent, *tlu_event);
      if (SyncStatus == Event_IS_Sync)
      {
        Process_Event_is_sync(currentEvent, *tlu_event);
        return true;
      }
      else if (SyncStatus == Event_IS_LATE)
      {
        Process_Event_is_late(currentEvent, *tlu_event);
        isAsync_ = true;
        event_queue.pop();
      }
      else if (SyncStatus == Event_IS_EARLY)
      {
        Process_Event_is_early(currentEvent, *tlu_event);
        isAsync_ = true;
        return false;
      }
    }
    return false;
  }

  bool Sync2TLU::compareTLUwithEventQueues(event_sp& tlu_event)
  {
    for (auto& ev_queue: m_ProducerEventQueue)
    {
      if (ev_queue==getFirstTLUQueue())
      {
        continue;
      }
      if (!compareTLUwithEventQueue(tlu_event, ev_queue)){
        // could not sync event.
        // TLU event is to early or event queue is empty;
        return false;
      }
    }
    return true;

  }

  bool Sync2TLU::SyncNEvents(size_t N)
  {

    while (m_outPutQueue.size() <= N)
    {
      if (!SyncFirstEvent())
      {
        return false;
      }

    }
    return true;
  }



  void Sync2TLU::event_queue_pop()
  {
    for (auto& q : m_ProducerEventQueue)
    {
      q.pop();
    }

  }



  void Sync2TLU::event_queue_pop_TLU_event()
  {
    getFirstTLUQueue().pop();
  }




  void Sync2TLU::clearOutputQueue()
  {

    std::queue<event_sp> empty;
    std::swap(m_outPutQueue, empty);
    
  }

  void Sync2TLU::PrepareForEvents()
  {

    if (m_ProducerEventQueue.empty())
    {

      EUDAQ_THROW("Producer Event queue is empty");
    }

      if (m_TLUs_found == 0)
      {
          EUDAQ_WARN("No TLU events found in the data\n synchrounsation on event numbers \n");
      }
      else if (m_TLUs_found > 1)
      {
        std::cout << "More than one TLU detected only the first TLU is used for synchronisation " << std::endl;
      }
  
    
 if (!m_Bore_buffer.empty())
 {
   // bore events are buffered in the output queue
 
   mergeBoreEvent(m_bore);
   
   
   

 }

 clearOutputQueue();
   
    

  }

  unsigned Sync2TLU::getUniqueID(unsigned fileIndex, unsigned eventIndex) const
  {

    return (fileIndex + 1)*FILEINDEX_OFFSET + eventIndex;
  }


  bool Sync2TLU::outputQueueIsEmpty() const 
  {
    return m_outPutQueue.empty();

  }

  bool Sync2TLU::getNextEvent(event_sp&   ev)
  {
 
    if (!m_Bore_buffer.empty())
    {
      PrepareForEvents();
      ev = m_bore;
      return true;
    }
    SyncFirstEvent();
    if (!m_outPutQueue.empty())
    {

      ev = m_outPutQueue.front();
      m_outPutQueue.pop();
      return true;
    }
    return false;
  }

  bool Sync2TLU::mergeBoreEvent(event_sp& ev)
  {
   
    if(!ev)
    {
      m_BOREeventWasEmtpy = true;
      auto firstEV = m_Bore_buffer.front();
      ev = std::make_shared<DetectorEvent>(firstEV->GetRunNumber(), firstEV->GetEventNumber(), firstEV->GetTimestamp());
      ev->SetFlags(Event::FLAG_BORE);
    }
    DetectorEvent* det = dynamic_cast<DetectorEvent*>(ev.get());
    while (!m_Bore_buffer.empty())
    {

      auto con = m_Bore_buffer.front()->GetTag("CONFIG", "");
      if (m_BOREeventWasEmtpy && PluginManager::isTLU(*(m_Bore_buffer.front())))
      {
        // overwrite event number if TLU was found later on

        ev->setEventNumber(m_Bore_buffer.front()->GetEventNumber());
        ev->setRunNumber(m_Bore_buffer.front()->GetRunNumber());
        ev->setTimeStamp(m_Bore_buffer.front()->GetTimestamp());

        // only first TLU is used
        m_BOREeventWasEmtpy = false;
      }

      if (m_firstConfig&&!con.empty())
      {
          det->SetTag("CONFIG", con);
          m_firstConfig = false;
          // the file  reader also add the Detector event to the buffer. this detector event does not need to be in the output collection 
      }
      else
      {
        det->AddEvent(m_Bore_buffer.front());
      }
     
      m_Bore_buffer.pop();
    
    }
    return true;
  }

  bool Sync2TLU::pushEvent(event_sp ev,size_t Index)
  {
    if (ev)
    {


      if (ev->IsBORE())
      {
        addBORE_BaseEvent(Index, ev);
      }
      else
      {
        AddBaseEventToProducerQueue(Index, ev);
      }
      
      return true;
    }
    else if(!SubEventQueueIsEmpty(Index))
    {
      return true;
    }
    else
    {
      return false;
    }


  }

  bool Sync2TLU::OutputIsEmpty() const
  {
    return outputQueueIsEmpty();
  }

  bool Sync2TLU::InputIsEmpty() const
  {
    return Event_Queue_Is_Empty();
  }

  bool Sync2TLU::InputIsEmpty(size_t fileId) const
  {
    return SubEventQueueIsEmpty(fileId);
  }

  int Sync2TLU::CompareEvents(eudaq::Event const & Current_event, eudaq::Event const & tlu_reference_event)
  {
    if (m_TLUs_found)
    {
        return PluginManager::IsSyncWithTLU(Current_event, tlu_reference_event);
    }
    return  compareTLU2DUT(tlu_reference_event.GetEventNumber(), Current_event.GetEventNumber());
  }



}
