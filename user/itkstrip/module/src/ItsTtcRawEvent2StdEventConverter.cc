#include "eudaq/StdEventConverter.hh"
#include "eudaq/RawEvent.hh"

class ItsTtcRawEvent2StdEventConverter: public eudaq::StdEventConverter{
public:
  bool Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2, eudaq::ConfigSPC conf) const override;
  static const uint32_t m_id_factory = eudaq::cstr2hash("ITS_TTC");
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::
    Register<ItsTtcRawEvent2StdEventConverter>(ItsTtcRawEvent2StdEventConverter::m_id_factory);
}

bool ItsTtcRawEvent2StdEventConverter::Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2,
						  eudaq::ConfigSPC conf) const{
  auto raw = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);
  if (!raw){
    EUDAQ_THROW("dynamic_cast error: from eudaq::Event to eudaq::RawEvent");
  }

  //DO NOTHING
  return true;
}
