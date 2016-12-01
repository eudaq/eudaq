#include "eudaq/StdEventConverter.hh"
#include "eudaq/DetectorEvent.hh"

namespace eudaq{

  class DLLEXPORT RawEvent2StdEventConverter: public StdEventConverter{
  public:
    bool Converting(EventSPC d1, StandardEventSP d2) const override;
    static const uint32_t m_id_factory = cstr2hash("RawDataEvent");
  };

  namespace{
    auto dummy0 = Factory<StdEventConverter>::
      Register<RawEvent2StdEventConverter>(RawEvent2StdEventConverter::m_id_factory);
  }
  
  bool RawEvent2StdEventConverter::Converting(EventSPC d1, StandardEventSP d2) const {
    auto ev = std::dynamic_pointer_cast<const RawDataEvent>(d1);
    if(!ev)
      return false;
    
    if(d1->IsFlagBit(Event::FLAG_PACKET)){
      d2->SetFlag(ev->GetFlag());
      d2->ClearFlagBit(Event::FLAG_PACKET);
      d2->SetRunN(ev->GetRunN());
      d2->SetEventN(ev->GetEventN());
      d2->SetStreamN(ev->GetStreamN());
      d2->SetTimestamp(ev->GetTimestampBegin(), ev->GetTimestampEnd());
      size_t nsub = d2->GetNumSubEvent();
      for(size_t i=0; i<nsub; i++){
	auto subev = d1->GetSubEvent(i);
	if(!StdEventConverter::Convert(subev, d2))
	  return false;
      }
      return  true;
    }
    
    uint32_t id = str2hash(ev->GetSubType());
    auto cvt = Factory<StdEventConverter>::MakeUnique(id);
    if(cvt){
      cvt->Converting(d1, d2);
      return true;
    }
    else{
      std::cerr<<"RawEent2StdEventConverter: "
	       <<"WARNING,  no converter for RawDataEvent SubType = "
	       <<ev->GetSubType()<<" ConverterID = "<<id<<"\n";
      return false;
    }
  }  
}
