#include "eudaq/StdEventConverter.hh"

namespace eudaq{

  template DLLEXPORT
  std::map<uint32_t, typename Factory<StdEventConverter>::UP(*)()>&
  Factory<StdEventConverter>::Instance<>();
  
  bool StdEventConverter::Convert(EventSPC d1, StandardEventSP d2){
    uint32_t id = d1->GetEventID();
    auto cvt = Factory<StdEventConverter>::MakeUnique(id);
    if(cvt){
      return cvt->Converting(d1, d2);
    }
    else{
      std::cerr<<"StdEventConverter: WARNING, no converter for EventID = "<<d1<<"\n";
      return false;
    }
  }
  
  StandardEventSP StdEventConverter::MakeSharedStdEvent(uint32_t run, uint32_t stm){
    uint32_t id = cstr2hash("_DET"); //TODO: check the register, hash or add?
    return std::dynamic_pointer_cast<StandardEvent>(Factory<Event>::MakeShared(id, run, stm));
  }

}
