#include "eudaq/StdEventConverter.hh"
#include "eudaq/DetectorEvent.hh"

namespace eudaq{

  class DLLEXPORT DetEvent2StdEventConverter: public StdEventConverter{
  public:
    bool Converting(EventSPC d1, StandardEventSP d2) const override;
    static const uint32_t m_id_cvt = cstr2hash("DetectorEvent");
  };

  namespace{
    auto dummy0 = Factory<StdEventConverter>::
      Register<DetEvent2StdEventConverter>(DetEvent2StdEventConverter::m_id_cvt);
  }
  
  bool DetEvent2StdEventConverter::Converting(EventSPC d1, StandardEventSP d2) const{
    auto ev = std::dynamic_pointer_cast<const DetectorEvent>(d1);
    if(!ev)
      return false;
    d2->SetFlag(ev->GetFlag());
    d2->SetRunN(ev->GetRunN());
    d2->SetEventN(ev->GetEventN());
    d2->SetStreamN(ev->GetStreamN());
    d2->SetTimestamp(ev->GetTimestampBegin(), ev->GetTimestampEnd());
    bool ret = true;
    size_t nsub = d2->GetNumSubEvent();
    for(size_t i=0; i<nsub; i++){
      auto subev = d1->GetSubEvent(i);
      if(!StdEventConverter::Convert(subev, d2))
	return false;
    }
    return true;
  }
}
