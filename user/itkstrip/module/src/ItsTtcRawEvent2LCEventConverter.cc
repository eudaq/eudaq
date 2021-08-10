#include "eudaq/LCEventConverter.hh"
#include "eudaq/RawEvent.hh"

#include "IMPL/TrackerRawDataImpl.h"
#include "IMPL/TrackerDataImpl.h"
#include "UTIL/CellIDEncoder.h"

#include "ItsTtcCommon.h"

class ItsTtcRawEvent2LCEventConverter: public eudaq::LCEventConverter{
  typedef std::vector<uint8_t>::const_iterator datait;
public:
  bool Converting(eudaq::EventSPC d1, eudaq::LCEventSP d2, eudaq::ConfigSPC conf) const override;
  static const uint32_t m_id_factory = eudaq::cstr2hash("ITS_TTC");
  static const uint32_t m_id1_factory = eudaq::cstr2hash("ITS_TTC_DUT");
  static const uint32_t m_id2_factory = eudaq::cstr2hash("ITS_TTC_Timing");

};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::LCEventConverter>::
    Register<ItsTtcRawEvent2LCEventConverter>(ItsTtcRawEvent2LCEventConverter::m_id_factory);
  auto dummy1 = eudaq::Factory<eudaq::LCEventConverter>::
    Register<ItsTtcRawEvent2LCEventConverter>(ItsTtcRawEvent2LCEventConverter::m_id1_factory);
  auto dummy2 = eudaq::Factory<eudaq::LCEventConverter>::
    Register<ItsTtcRawEvent2LCEventConverter>(ItsTtcRawEvent2LCEventConverter::m_id2_factory);
}

bool ItsTtcRawEvent2LCEventConverter::Converting(eudaq::EventSPC d1, eudaq::LCEventSP d2,
						 eudaq::ConfigSPC conf) const{
  auto raw = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);
  if (!raw){
    EUDAQ_THROW("dynamic_cast error: from eudaq::Event to eudaq::RawEvent");
  }
  uint32_t deviceId = d1->GetStreamN();
  ItsTtc::TtcInfo eTtc(raw->GetBlock(0), deviceId);

  std::string deviceName;
  if (deviceId<300) {
  	deviceName = "_TIMING";
  } else {
  	deviceName = "_DUT";
  }

  d2->parameters().setValue("PTDC"+deviceName+".TYPE", std::to_string(eTtc.getPTdcType()));
  d2->parameters().setValue("PTDC"+deviceName+".L0ID", std::to_string(eTtc.getPTdcL0ID()));
  d2->parameters().setValue("PTDC"+deviceName+".TS", std::to_string(eTtc.getPTdcTs()));
  d2->parameters().setValue("PTDC"+deviceName+".BIT", std::to_string(eTtc.getPTdcBit()));
  d2->parameters().setValue("TLU"+deviceName+".TLUID", std::to_string(eTtc.getTluID()));
  d2->parameters().setValue("TLU"+deviceName+".L0ID", std::to_string(eTtc.getTluL0ID()));
  d2->parameters().setValue("TDC"+deviceName+".DATA", std::to_string(eTtc.getTdc()));
  d2->parameters().setValue("TDC"+deviceName+".L0ID", std::to_string(eTtc.getL0ID()));
  d2->parameters().setValue("TS"+deviceName+".DATA", std::to_string(eTtc.getTimestamp()));
  d2->parameters().setValue("TS"+deviceName+".L0ID", std::to_string(eTtc.getL0ID()));
  d2->parameters().setValue("TTC"+deviceName+".BCID", std::to_string(eTtc.getTtcBCID())); //ITSDAQ
  return true; 
}
