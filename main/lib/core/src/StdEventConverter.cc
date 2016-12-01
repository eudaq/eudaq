#include "eudaq/StdEventConverter.hh"

namespace eudaq{

  template DLLEXPORT
  std::map<uint32_t, typename Factory<StdEventConverter>::UP(*)()>&
  Factory<StdEventConverter>::Instance<>();
  
  bool StdEventConverter::Convert(EventSPC d1, StandardEventSP d2){
    
    if(d1->IsFlagBit(Event::FLAG_PACKET)){
      d2->SetFlag(d1->GetFlag());
      d2->ClearFlagBit(Event::FLAG_PACKET);
      d2->SetRunN(d1->GetRunN());
      d2->SetEventN(d1->GetEventN());
      d2->SetStreamN(d1->GetStreamN());
      d2->SetTimestamp(d1->GetTimestampBegin(), d1->GetTimestampEnd());
      size_t nsub = d1->GetNumSubEvent();
      for(size_t i=0; i<nsub; i++){
	auto subev = d1->GetSubEvent(i);
	if(!StdEventConverter::Convert(subev, d2))
	  return false;
      }
      return  true;
    }
    
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
    uint32_t id = cstr2hash("StandardEvent"); //TODO: check the register, hash or add?
    return std::dynamic_pointer_cast<StandardEvent>(Factory<Event>::MakeShared(id, run, stm));
  }

}
