#include "eudaq/StdEventConverter.hh"
#include "eudaq/RawDataEvent.hh"

namespace eudaq{

  class DLLEXPORT TluRawEvent2StdEventConverter: public StdEventConverter{
  public:
    bool Converting(EventSPC d1, StandardEventSP d2) const override;
    static const uint32_t m_id_cvt = cstr2hash("TluRawDataEvent");
  };
  
  namespace{
    auto dummy0 = Factory<StdEventConverter>::
      Register<TluRawEvent2StdEventConverter>(TluRawEvent2StdEventConverter::m_id_cvt);
  }  

  bool TluRawEvent2StdEventConverter::Converting(EventSPC d1, StandardEventSP d2) const{
    static const std::string TLU("TLU");
    auto ev = std::dynamic_pointer_cast<const RawDataEvent>(d1);
    if(!ev)
      return false;
    auto stm = std::to_string(d1->GetStreamN());
    d2->SetTag(TLU, stm+"_"+d2->GetTag(TLU));
    d2->SetTag(TLU+stm+"_TSB", std::to_string(ev->GetTimestampBegin()));
    d2->SetTag(TLU+stm+"_TSE", std::to_string(ev->GetTimestampEnd()));
    d2->SetTag(TLU+stm+"_EVN", std::to_string(ev->GetEventN()));
    return true;
  }

}
