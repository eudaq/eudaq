#include "eudaq/LCEventConverter.hh"
#include "eudaq/Logger.hh"

namespace eudaq{
  using namespace lcio;

  class TLURawEvent2LCEventConverter: public LCEventConverter{
  public:
    bool Converting(EventSPC d1, LCEventSP d2, const Configuration &conf) const override;
    static const uint32_t m_id_factory = cstr2hash("TLURawDataEvent");
  };
  
  namespace{
    auto dummy0 = Factory<LCEventConverter>::
      Register<TLURawEvent2LCEventConverter>(TLURawEvent2LCEventConverter::m_id_factory);
  }

  bool TLURawEvent2LCEventConverter::Converting(EventSPC d1, LCEventSP d2, const Configuration &conf) const{
    auto d2impl = std::dynamic_pointer_cast<LCEventImpl>(d2);
    d2impl->setTimeStamp(d1->GetTimestampBegin());
    d2impl->setEventNumber(d1->GetEventNumber());
    d2impl->setRunNumber(d1->GetRunNumber());
    return true;    
  }
  
}
