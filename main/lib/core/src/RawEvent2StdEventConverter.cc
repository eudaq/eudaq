#include "eudaq/StdEventConverter.hh"
#include "eudaq/RawEvent.hh"

namespace eudaq{

  class RawEvent2StdEventConverter: public StdEventConverter{
  public:
    bool Converting(EventSPC d1, StandardEventSP d2, ConfigurationSPC conf) const override;
    static const uint32_t m_id_factory = cstr2hash("RawEvent");
  };

  namespace{
    auto dummy0 = Factory<StdEventConverter>::
      Register<RawEvent2StdEventConverter>(RawEvent2StdEventConverter::m_id_factory);
  }
  
  bool RawEvent2StdEventConverter::Converting(EventSPC d1, StandardEventSP d2, ConfigurationSPC conf) const {
    auto ev = std::dynamic_pointer_cast<const RawEvent>(d1);
    if(!ev){
      EUDAQ_ERROR("ERROR, the input event is not RawEvent");
      return false;
    }

    uint32_t id = ev->GetExtendWord();
    auto cvt = Factory<StdEventConverter>::MakeUnique(id);
    if(cvt){
      return cvt->Converting(d1, d2, conf);
    }
    else{
      EUDAQ_WARN("WARNING, no StdEventConverter for RawEvent with ExtendWord("
		 + std::to_string(id)+ ") and Description("+ ev->GetDescription()+ ")");
      return false;
    }
  }  
}

