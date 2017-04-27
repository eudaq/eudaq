#ifndef EventSynchronisationCompareWithTLU_h__
#define EventSynchronisationCompareWithTLU_h__





#include <queue>
#include <memory>
#include <string>
#include "DetectorEvent.hh"
#include "FileSerializer.hh"
#include "EventSynchronisationBase.hh"

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
   // using Detectorevent_sp = std::shared_ptr < eudaq::DetectorEvent > ;

    //////////////////////////////////////////////////////////////////////////
    // public interface


    virtual bool pushEvent(event_sp ev,size_t Index=0) override;
    virtual bool getNextEvent(event_sp& ev) override;
    virtual bool OutputIsEmpty() const override;
    virtual bool InputIsEmpty() const override;
    virtual bool InputIsEmpty(size_t fileId) const override;

    
    //////////////////////////////////////////////////////////////////////////


    Sync2TLU(Parameter_ref);
    virtual ~Sync2TLU() {}

  private:
    
    bool mergeBoreEvent(event_sp& ev);
    int AddEventToProducerQueue(int fileIndex, event_sp Ev);
    int AddBaseEventToProducerQueue(int fileIndex, event_sp Ev);
    void clearOutputQueue();

    void addBORE_Event(unsigned int fileIndex, event_sp BOREEvent);
    void addBORE_BaseEvent(unsigned int fileIndex, event_sp BOREEvent);
    bool SyncNEvents(size_t N);
    void PrepareForEvents();
    bool SubEventQueueIsEmpty(int i) const;
    bool SyncFirstEvent();
    bool outputQueueIsEmpty() const;
    
    virtual int   CompareEvents(eudaq::Event const & Current_event, eudaq::Event const & tlu_reference_event);
    virtual void Process_Event_is_sync(event_sp ev, eudaq::Event const & tlu) {}
    virtual void Process_Event_is_late(event_sp ev, eudaq::Event const & tlu) {}
    virtual void Process_Event_is_early(event_sp ev, eudaq::Event const & tlu) {}

    virtual void storeCurrentOrder(){}
    virtual void makeDetectorEvent(){}

    event_sp m_bore;
    size_t  m_event_id = 0;
    /** The empty destructor. Need to add it to make it virtual.
     */

  protected:

    bool compareTLUwithEventQueues(event_sp& tlu_event);
    bool compareTLUwithEventQueue(event_sp& tlu_event, Sync2TLU::eventqueue_t& event_queue);
    bool Event_Queue_Is_Empty() const;
    void event_queue_pop_TLU_event();
    void event_queue_pop();
    eventqueue_t& getQueuefromId(unsigned producerID);
    eventqueue_t& getQueuefromId(unsigned fileIndex, unsigned eventIndex);
    bool ProducerQueueExist(unsigned producerID);



    eventqueue_t& getFirstTLUQueue();
    unsigned getUniqueID(unsigned fileIndex, unsigned eventIndex) const ;
    //unsigned getTLU_UniqueID(unsigned fileIndex);
    std::map<unsigned, size_t> m_ProducerId2Eventqueue;
    size_t m_registertProducer = 0;
    std::vector<size_t> m_ProducerProFileReader;
    /* This vector saves for each producer an event queue */

    std::vector<eventqueue_t> m_ProducerEventQueue;

    eventqueue_t m_outPutQueue;
    eventqueue_t m_Bore_buffer;

    
    int m_TLUs_found = 0;
    size_t m_TLU_pos = 0;
    bool isAsync_ = false ,m_firstConfig=true;
    size_t NumberOfEventsToSync_;
    uint64_t longTimeDiff_;
    bool m_BOREeventWasEmtpy = false;
  
  };



}//namespace eudaq



#endif // EventSynchronisationCompareWithTLU_h__
