#include "eudaq/TTreeEventConverter.hh"
#include "eudaq/RawEvent.hh"

namespace eudaq{

  class RawEvent2TTreeEventConverter: public TTreeEventConverter{
  public:
    bool Converting(EventSPC d1, TTreeEventSP d2, ConfigurationSPC conf) const override;
    static const uint32_t m_id_factory = cstr2hash("RawEvent");
  };

  namespace{
    auto dummy0 = Factory<TTreeEventConverter>::
      Register<RawEvent2TTreeEventConverter>(RawEvent2TTreeEventConverter::m_id_factory);
  }
  
  bool RawEvent2TTreeEventConverter::Converting(EventSPC d1, TTreeEventSP d2, ConfigurationSPC conf) const {
    auto ev = std::dynamic_pointer_cast<const RawDataEvent>(d1);
    if(!ev){
      EUDAQ_ERROR("ERROR, the input event is not RawDataEvent");
      return false;
    }
    uint32_t id = ev->GetExtendWord();
    //    std::cout << " Sub Type " << ev->GetDescription() << std::endl;
    auto cvt = Factory<TTreeEventConverter>::MakeUnique(id);
     if(cvt){
      cvt->Converting(d1, d2, conf);
      return true;
    }
    else{
      EUDAQ_WARN("WARNING, no TTreeEventConverter for RawDataEvent with ExtendWord("
		 + std::to_string(id)+ ") and Description("+ ev->GetDescription()+ ")");
      return false;
    }
  }  
}
