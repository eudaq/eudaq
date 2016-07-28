#ifndef EventSynchronisationMultiTSEvents_h__
#define EventSynchronisationMultiTSEvents_h__

#include "EventSynchronisationCompareWithTLU.hh"
#include <memory>
#include <queue>
// base class for all Synchronization Plugins
// it is desired to be as modular es possible with this approach.
// first step is to separate the events from different Producers. 
// then it will be recombined to a new event
// The Idea is that the user can define a condition that need to be true to define if an event is sync or not

namespace eudaq{




  class DLLEXPORT syncToMultiTSEvents: public Sync2TLU {
  public:
    syncToMultiTSEvents(Parameter_ref sync);
    virtual void Process_Event_is_late(std::shared_ptr<eudaq::Event>  ev, eudaq::Event const & tlu);
    virtual void Process_Event_is_sync(std::shared_ptr<eudaq::Event>  ev, eudaq::Event const & tlu);
    virtual void makeDetectorEvent();
    
  private:
    bool m_first_event = true;

  };


}//namespace eudaq



#endif // EventSynchronisationMultiTSEvents_h__
