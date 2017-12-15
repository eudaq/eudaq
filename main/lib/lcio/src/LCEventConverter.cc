#include "eudaq/LCEventConverter.hh"

namespace eudaq{
  
  template DLLEXPORT
  std::map<uint32_t, typename Factory<LCEventConverter>::UP(*)()>&
  Factory<LCEventConverter>::Instance<>();
   bool LCEventConverter::Convert(EventSPC d1, LCEventSP d2, ConfigurationSPC conf){
    if(d1->IsFlagFake()){
      return true;
    }
    
    if(d1->IsFlagPacket()){
      d2->setRunNumber(d1->GetRunN());
      d2->setEventNumber(d1->GetEventN());
      d2->setDetectorName(d1->GetDescription());
      if(d1->IsFlagTimestamp()){
	d2->setTimeStamp(d1->GetTimestampBegin());
      }
      d2->parameters().setValue("EventFlag", (int)d1->GetFlag());
      d2->parameters().setValue("DeviceNumber", (int)d1->GetDeviceN());
      if(d1->IsFlagTrigger()){
	d2->parameters().setValue("TriggerNumber", (int)d1->GetTriggerN());
      }
      if(d1->IsFlagTimestamp()){
	d2->parameters().setValue("TimestampBegin", (int)d1->GetTimestampBegin());
	d2->parameters().setValue("TimestampEnd", (int)d1->GetTimestampEnd());
      }
      size_t nsub = d1->GetNumSubEvent();
      for(size_t i=0; i<nsub; i++){
	auto subev = d1->GetSubEvent(i);
	if(!LCEventConverter::Convert(subev, d2, conf))
	  return false;
      }
      d2->parameters().setValue("EventFlag", (int)(d1->GetFlag() & ~Event::Flags::FLAG_PACK));
      return true;
    }

    uint32_t d2_flag = d2->parameters().getIntVal("EventFlag");
    if((d2_flag & Event::Flags::FLAG_PACK) != Event::Flags::FLAG_PACK){
      d2->setRunNumber(d1->GetRunN());
      d2->setEventNumber(d1->GetEventN());
      d2->setDetectorName(d1->GetDescription());
      if(d1->IsFlagTimestamp()){
	d2->setTimeStamp(d1->GetTimestampBegin());
      }
      d2->parameters().setValue("EventFlag", (int)d1->GetFlag());
      d2->parameters().setValue("DeviceNumber", (int)d1->GetDeviceN());
      if(d1->IsFlagTrigger()){
	d2->parameters().setValue("TriggerNumber", (int)d1->GetTriggerN());
      }
      if(d1->IsFlagTimestamp()){
	d2->parameters().setValue("TimestampBegin", (int)d1->GetTimestampBegin());
	d2->parameters().setValue("TimestampEnd", (int)d1->GetTimestampEnd());
      }
    }
    
    uint32_t id = d1->GetType();
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
