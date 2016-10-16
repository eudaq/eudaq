#include "Processor.hh"
#include "Event.hh"
#include <random>
#include <algorithm> 


namespace eudaq{

  class UnpackSyncEventPS: public Processor{
  public:
    UnpackSyncEventPS(std::string cmd);
    void ProcessUserEvent(EVUP ev);

  };


  namespace{
  auto dummy0 = Factory<Processor>::Register<UnpackSyncEventPS, std::string&>(eudaq::cstr2hash("UnpackSyncEventPS"));
  auto dummy1 = Factory<Processor>::Register<UnpackSyncEventPS, std::string&&>(eudaq::cstr2hash("UnpackSyncEventPS"));
  }

  UnpackSyncEventPS::UnpackSyncEventPS(std::string cmd)
    :Processor("UnpackSyncEventPS", ""){
    ProcessCmd(cmd);
  }

  void UnpackSyncEventPS::ProcessUserEvent(EVUP ev){
    // if(ev->GetEventID() != cstr2hash("SYNC"))
    //   return;
    // std::cout<<"\n\n.................NEW SYNC EVENT.........\n";
    // ev->Print(std::cout);
    uint32_t n = ev->GetNumSubEvent();
    for(uint32_t i = 0; i< n; i++){
      auto subev = ev->GetSubEvent(i);
      ForwardEvent( subev->Clone());
      // subev->Print(std::cout, 20);
    }
    
  }


}
