#include "eudaq/LCEventConverter.hh"
#include "eudaq/Logger.hh"

class TluRawEvent2LCEventConverter: public eudaq::LCEventConverter{
public:
  bool Converting(eudaq::EventSPC d1, eudaq::LCEventSP d2, eudaq::ConfigurationSPC conf) const override;
  static const uint32_t m_id_factory = eudaq::cstr2hash("TluRawDataEvent");
};
  
namespace{
  auto dummy0 = eudaq::Factory<eudaq::LCEventConverter>::
    Register<TluRawEvent2LCEventConverter>(TluRawEvent2LCEventConverter::m_id_factory);
}

bool TluRawEvent2LCEventConverter::Converting(eudaq::EventSPC d1, eudaq::LCEventSP d2, eudaq::ConfigurationSPC conf) const{
  auto d2impl = std::dynamic_pointer_cast<lcio::LCEventImpl>(d2);
  d2impl->setTimeStamp(d1->GetTimestampBegin());
  d2impl->setEventNumber(d1->GetEventNumber());
  d2impl->setRunNumber(d1->GetRunNumber());
  d2impl->parameters().setValue("EventType", 2); //( eutelescope::EUTELESCOPE::EVENTTYPE, eutelescope::kDE )
  return true;    
}
