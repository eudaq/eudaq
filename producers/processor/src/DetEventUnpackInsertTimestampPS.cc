#include "Processor.hh"
#include "DetectorEvent.hh"

using namespace eudaq;

class DetEventUnpackInsertTimestampPS;

namespace{
  auto dummy0 = Factory<Processor>::Register<DetEventUnpackInsertTimestampPS>
    (eudaq::cstr2hash("DetEventUnpackInsertTimestampPS"));
}


class DetEventUnpackInsertTimestampPS: public Processor{
public:
  DetEventUnpackInsertTimestampPS();
  void ProcessEvent(EventSPC ev) final override;
};

DetEventUnpackInsertTimestampPS::DetEventUnpackInsertTimestampPS()
  :Processor("DetEventUnpackInsertTimestampPS"){
}

void DetEventUnpackInsertTimestampPS::ProcessEvent(EventSPC ev){
  if(ev->GetEventID() != Event::str2id("_DET")){
    ForwardEvent(ev);
  }
  auto detev = dynamic_cast<const eudaq::DetectorEvent *>(ev.get());
  size_t nevent = detev->NumEvents();
  uint64_t ts= 0;
  
  for(size_t i= 0; i< nevent; i++){
    auto subevptr = detev->GetEvent(i);
    if(subevptr->GetEventID() == Event::str2id("_TLU")){
      ts  = subevptr->GetTimestampBegin();
    }
  }
  
  for(size_t i= 0; i< nevent; i++){
    auto subevptr = detev->GetEvent(i);
    if(subevptr->GetEventID() == Event::str2id("_RAW")){
      auto subev = subevptr->Clone();
      subev->SetTimestampBegin(ts);
      //TODO:: SetTimestampEnd depending on SUBTYPE    ts+duration
      
      auto raw_type = subevptr->GetSubType();
      uint32_t duration = 0;
      if(raw_type=="NI"){
	duration = 115200 * 48/125; //TODO 115200ns / ts_resulation , 125/40  (48)  ,  >1 <2 frame
      }else if(raw_type=="ITS_ABC"){
	duration = 25 * 48/125; //TODO: 25ns
      }else if(raw_type=="ITS_TTC"){
	duration = 25 * 48/125;
      }else if(raw_type=="USBPIXI4"){
	duration = 25 * 48/125;
      }
      uint64_t ts_end = ts + duration;
      subev->SetTimestampEnd(ts_end);
      
      ForwardEvent(std::move(subev));
    }
  }
}
