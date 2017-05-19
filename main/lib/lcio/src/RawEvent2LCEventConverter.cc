#include "eudaq/LCEventConverter.hh"
#include "eudaq/RawEvent.hh"

namespace eudaq{

  class RawEvent2LCEventConverter: public LCEventConverter{
  public:
    bool Converting(EventSPC d1, LCEventSP d2, ConfigurationSPC conf) const override;
    static const uint32_t m_id_factory = cstr2hash("RawEvent");
  };

  namespace{
    auto dummy0 = Factory<LCEventConverter>::
      Register<RawEvent2LCEventConverter>(RawEvent2LCEventConverter::m_id_factory);
  }
  
  bool RawEvent2LCEventConverter::Converting(EventSPC d1, LCEventSP d2, ConfigurationSPC conf) const {
    auto ev = std::dynamic_pointer_cast<const RawDataEvent>(d1);
    if(!ev){
      EUDAQ_ERROR("ERROR, the input event is not RawDataEvent");
      return false;
    }
    uint32_t id = ev->GetExtendWord();
    auto cvt = Factory<LCEventConverter>::MakeUnique(id);
    if(cvt){
      cvt->Converting(d1, d2, conf);
      return true;
    }
    else{
      EUDAQ_WARN("WARNING, no LCEventConverter for RawDataEvent with ExtendWord("
		 + std::to_string(id)+ ") and Description("+ ev->GetDescription()+ ")");
      return false;
    }
  }  
}
