#include "eudaq/StdEventConverter.hh"
#include "eudaq/RawDataEvent.hh"

namespace eudaq{

  class DLLEXPORT RawEvent2StdEventConverter: public StdEventConverter{
  public:
    bool Converting(EventSPC d1, StandardEventSP d2, const Configuration &conf) const override;
    static const uint32_t m_id_factory = cstr2hash("RawDataEvent");
  };

  namespace{
    auto dummy0 = Factory<StdEventConverter>::
      Register<RawEvent2StdEventConverter>(RawEvent2StdEventConverter::m_id_factory);
  }
  
  bool RawEvent2StdEventConverter::Converting(EventSPC d1, StandardEventSP d2, const Configuration &conf) const {
    auto ev = std::dynamic_pointer_cast<const RawDataEvent>(d1);
    if(!ev)
      return false;
    
    uint32_t id = str2hash(ev->GetSubType());
    auto cvt = Factory<StdEventConverter>::MakeUnique(id);
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
