#ifndef EventSynchronisationDetectorEvents_h__
#define EventSynchronisationDetectorEvents_h__

#include "EventSynchronisationCompareWithTLU.hh"
#include <memory>
#include <queue>

namespace eudaq{

  class DLLEXPORT syncToDetectorEvents: public Sync2TLU {
  public:
    syncToDetectorEvents(SyncBase::Parameter_ref sync);
    virtual void makeDetectorEvent();
    
  private:
        
    virtual void storeCurrentOrder();
    
  };

}//namespace eudaq




#endif // EventSynchronisationDetectorEvents_h__
