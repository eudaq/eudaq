#include "eudaq/LCEventConverter.hh"
#include "eudaq/RawDataEvent.hh"

namespace eudaq{

  class RawEvent2LCEventConverter: public LCEventConverter{
  public:
    bool Converting(EventSPC d1, LCEventSP d2, ConfigurationSPC conf) const override;
    static const uint32_t m_id_factory = cstr2hash("RawDataEvent");
  };

  namespace{
    auto dummy0 = Factory<LCEventConverter>::
      Register<RawEvent2LCEventConverter>(RawEvent2LCEventConverter::m_id_factory);
  }
  
  bool RawEvent2LCEventConverter::Converting(EventSPC d1, LCEventSP d2, ConfigurationSPC conf) const {
    auto ev = std::dynamic_pointer_cast<const RawDataEvent>(d1);
    if(!ev)
      return false;
    
    uint32_t id = str2hash(ev->GetSubType());
    auto cvt = Factory<LCEventConverter>::MakeUnique(id);
    if(cvt){
      cvt->Converting(d1, d2, conf);
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
