#include "eudaq/LCEventConverter.hh"

namespace eudaq{
  
  template DLLEXPORT
  std::map<uint32_t, typename Factory<LCEventConverter>::UP(*)()>&
  Factory<LCEventConverter>::Instance<>();
  
  bool LCEventConverter::Convert(EventSPC d1, LCEventSP d2, const Configuration &conf){    
    if(d1->IsFlagBit(Event::FLAG_PACKET)){
      d2->parameters().setValue("EventFlag", (int)d1->GetFlag());
      d2->parameters().setValue("RunNumber", (int)d1->GetRunN());
      d2->parameters().setValue("EventNumber", (int)d1->GetEventN());
      d2->parameters().setValue("StreamNumber", (int)d1->GetStreamN());
      d2->parameters().setValue("TimestampBegin", (int)d1->GetTimestampBegin());
      d2->parameters().setValue("TimestampEnd", (int)d1->GetTimestampEnd());
      size_t nsub = d1->GetNumSubEvent();
      for(size_t i=0; i<nsub; i++){
	auto subev = d1->GetSubEvent(i);
	if(!LCEventConverter::Convert(subev, d2, conf))
	  return false;
      }
      return true;
    }
    
    uint32_t id = d1->GetEventID();
    auto cvt = Factory<LCEventConverter>::MakeUnique(id);
    if(cvt){
      return cvt->Converting(d1, d2, conf);
    }
    else{
      std::cerr<<"LCEventConverter: WARNING, no converter for EventID = "<<d1<<"\n";
      return false;
    }
  }
  

}
