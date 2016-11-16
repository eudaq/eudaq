#include "Processor.hh"
#include "Event.hh"
#include <random>
#include <algorithm> 


namespace eudaq{

  class UnpackSyncEventPS: public Processor{
  public:
    UnpackSyncEventPS();
    void ProcessEvent(EventSPC ev) final override;
  };

  namespace{
  auto dummy0 = Factory<Processor>::Register<UnpackSyncEventPS>(eudaq::cstr2hash("UnpackSyncEventPS"));
  }

  UnpackSyncEventPS::UnpackSyncEventPS()
    :Processor("UnpackSyncEventPS"){
  }

  void UnpackSyncEventPS::ProcessEvent(EventSPC ev){
    // if(ev->GetEventID() != cstr2hash("SYNC"))
    //   return;
    // std::cout<<"\n\n.................NEW SYNC EVENT.........\n";
    // ev->Print(std::cout);
    uint32_t n = ev->GetNumSubEvent();
    for(uint32_t i = 0; i< n; i++){
      auto subev = ev->GetSubEvent(i);
      ForwardEvent(subev);
      // subev->Print(std::cout, 20);
    }
    
  }


}
