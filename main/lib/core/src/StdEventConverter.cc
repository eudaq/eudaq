#include "eudaq/StdEventConverter.hh"

namespace eudaq{

  template DLLEXPORT
  std::map<uint32_t, typename Factory<StdEventConverter>::UP(*)()>&
  Factory<StdEventConverter>::Instance<>();
  
  bool StdEventConverter::Convert(EventSPC d1, StdEventSP d2, ConfigurationSPC conf){
    if(d1->IsFlagFake()){
      return true;
    }
    
    if(d1->IsFlagPacket()){
      d2->SetVersion(d1->GetVersion());
      d2->SetFlag(d1->GetFlag());
      d2->SetRunN(d1->GetRunN());
      d2->SetEventN(d1->GetEventN());
      d2->SetDeviceN(d1->GetDeviceN());
      d2->SetTriggerN(d1->GetTriggerN(), d1->IsFlagTrigger());
      d2->SetTimestamp(d1->GetTimestampBegin(), d1->GetTimestampEnd(), d1->IsFlagTimestamp());
      d2->SetDescription(d1->GetDescription());
      size_t nsub = d1->GetNumSubEvent();
      for(size_t i=0; i<nsub; i++){
	auto subev = d1->GetSubEvent(i);
	if(!d1->IsFlagFake())
	  if(!StdEventConverter::Convert(subev, d2, conf))
	    return false;
      }
      d2->ClearFlagBit(Event::Flags::FLAG_PACK);
      return  true;
    }
    if(!d2->IsFlagPacket()){
      d2->SetVersion(d1->GetVersion());
      d2->SetFlag(d1->GetFlag());
      d2->SetRunN(d1->GetRunN());
      d2->SetEventN(d1->GetEventN());
      d2->SetDeviceN(d1->GetDeviceN());
      d2->SetTriggerN(d1->GetTriggerN(), d1->IsFlagTrigger());
      d2->SetTimestamp(d1->GetTimestampBegin(), d1->GetTimestampEnd(), d1->IsFlagTimestamp());
      d2->SetDescription(d1->GetDescription());
    }
    uint32_t id = d1->GetType();
    auto cvt = Factory<StdEventConverter>::MakeUnique(id);
    if(cvt){
      return cvt->Converting(d1, d2, conf);
    }
    else{
      std::cerr<<"StdEventConverter: WARNING, no converter for EventID = "<<d1<<"\n";
      return false;
    }
  }
}
