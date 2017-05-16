#include "eudaq/LCEventConverter.hh"
#include "eudaq/RawEvent.hh"

#include "IMPL/TrackerRawDataImpl.h"
#include "IMPL/TrackerDataImpl.h"
#include "UTIL/CellIDEncoder.h"

class ItsTtcRawEvent2LCEventConverter: public eudaq::LCEventConverter{
  typedef std::vector<uint8_t>::const_iterator datait;
public:
  bool Converting(eudaq::EventSPC d1, eudaq::LCEventSP d2, eudaq::ConfigSPC conf) const override;
  static const uint32_t m_id_factory = eudaq::cstr2hash("ItsTtcRaw");
  static const uint32_t PLANE_ID_OFFSET_TTC = 30;
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::LCEventConverter>::
    Register<ItsTtcRawEvent2LCEventConverter>(ItsTtcRawEvent2LCEventConverter::m_id_factory);
}

bool ItsTtcRawEvent2LCEventConverter::Converting(eudaq::EventSPC d1, eudaq::LCEventSP d2,
						 eudaq::ConfigSPC conf) const{
  auto raw = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);
  if (!raw){
    EUDAQ_THROW("dynamic_cast error: from eudaq::Event to eudaq::RawEvent");
  }
  //TODO
  return true; 
}
