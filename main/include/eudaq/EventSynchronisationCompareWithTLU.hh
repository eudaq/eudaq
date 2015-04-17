#ifndef EventSynchronisationCompareWithTLU_h__
#define EventSynchronisationCompareWithTLU_h__





#include <queue>
#include <memory>
#include <string>
#include "eudaq/DetectorEvent.hh"
#include "eudaq/FileSerializer.hh"
#include "eudaq/factory.hh"
#include "eudaq/EventSynchronisationBase.hh"

// base class for all Synchronization Plugins
// it is desired to be as modular es possible with this approach.
// first step is to separate the events from different Producers. 
// then it will be recombined to a new event
// The Idea is that the user can define a condition that need to be true to define if an event is sync or not




namespace eudaq{


  class OptionParser;

  class DLLEXPORT Sync2TLU : public eudaq::SyncBase{
  public:
    typedef std::queue<std::shared_ptr<eudaq::Event>> eventqueue_t;
   // using DetectorEvent_sp = std::shared_ptr < eudaq::DetectorEvent > ;

    //////////////////////////////////////////////////////////////////////////
    // public interface


    virtual bool pushEvent(Event_sp ev,size_t Index=0) override;
    virtual bool getNextEvent(Event_sp& ev) override;
    virtual bool OutputIsEmpty() const override;
    virtual bool InputIsEmpty() const override;
    virtual bool InputIsEmpty(size_t fileId) const override;

    virtual bool mergeBoreEvent(Event_sp& ev);
    //////////////////////////////////////////////////////////////////////////


    Sync2TLU(Parameter_ref);
    virtual ~Sync2TLU() {}

  private:
    

    int AddEventToProducerQueue(int fileIndex, Event_sp Ev);
    int AddBaseEventToProducerQueue(int fileIndex, Event_sp Ev);
    void clearOutputQueue();

    void addBORE_Event(int fileIndex, Event_sp BOREEvent);
    void addBORE_BaseEvent(int fileIndex, Event_sp BOREEvent);
    bool SyncNEvents(size_t N);
    void PrepareForEvents();
    bool SubEventQueueIsEmpty(int i) const;
    bool SyncFirstEvent();
    bool outputQueueIsEmpty() const;
    
    virtual void Process_Event_is_sync(Event_sp ev, eudaq::Event const & tlu) {}
    virtual void Process_Event_is_late(Event_sp ev, eudaq::Event const & tlu) {}
    virtual void Process_Event_is_early(Event_sp ev, eudaq::Event const & tlu) {}

    virtual void storeCurrentOrder(){}
    virtual void makeDetectorEvent(){}


    size_t  m_event_id = 1;
    /** The empty destructor. Need to add it to make it virtual.
     */

  protected:

    bool compareTLUwithEventQueues(Event_sp& tlu_event);
    bool compareTLUwithEventQueue(Event_sp& tlu_event, Sync2TLU::eventqueue_t& event_queue);
    bool Event_Queue_Is_Empty() const;
    void event_queue_pop_TLU_event();
    void event_queue_pop();
    eventqueue_t& getQueuefromId(unsigned producerID);
    eventqueue_t& getQueuefromId(unsigned fileIndex, unsigned eventIndex);

    eventqueue_t& getFirstTLUQueue();
    unsigned getUniqueID(unsigned fileIndex, unsigned eventIndex) const ;
    //unsigned getTLU_UniqueID(unsigned fileIndex);
    std::map<unsigned, size_t> m_ProducerId2Eventqueue;
    size_t m_registertProducer = 0;
    std::vector<size_t> m_EventsProFileReader;
    /* This vector saves for each producer an event queue */

    std::vector<eventqueue_t> m_ProducerEventQueue;

    std::queue<std::shared_ptr<eudaq::Event>> m_outPutQueue;

    
    int m_TLUs_found = 0;

    bool isAsync_ = false ,m_firstConfig=true;
    size_t NumberOfEventsToSync_;
    uint64_t longTimeDiff_;

    bool m_sync=true,m_preparedforEvents=false;
  };



}//namespace eudaq



#endif // EventSynchronisationCompareWithTLU_h__