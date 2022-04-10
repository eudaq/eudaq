#include "eudaq/StdEventConverter.hh"
#include "eudaq/RawEvent.hh"

#include "ItsTtcCommon.h"

class ItsTtcRawEvent2StdEventConverter: public eudaq::StdEventConverter{
public:
  bool Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2, eudaq::ConfigSPC conf) const override;
  static const uint32_t m_id_factory = eudaq::cstr2hash("ITS_TTC");
  static const uint32_t m_id1_factory = eudaq::cstr2hash("ITS_TTC_DUT");
  static const uint32_t m_id2_factory = eudaq::cstr2hash("ITS_TTC_Timing");
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::
    Register<ItsTtcRawEvent2StdEventConverter>(ItsTtcRawEvent2StdEventConverter::m_id_factory);
  auto dummy1 = eudaq::Factory<eudaq::StdEventConverter>::
    Register<ItsTtcRawEvent2StdEventConverter>(ItsTtcRawEvent2StdEventConverter::m_id1_factory);
  auto dummy2 = eudaq::Factory<eudaq::StdEventConverter>::
    Register<ItsTtcRawEvent2StdEventConverter>(ItsTtcRawEvent2StdEventConverter::m_id2_factory);
}

bool ItsTtcRawEvent2StdEventConverter::Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2,
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
  //  std::cout << "deviceId TTC" << deviceId << std::endl;
  d2->SetTag("PTDC"+deviceName+".TYPE", eTtc.getPTdcType());
  d2->SetTag("PTDC"+deviceName+".L0ID", eTtc.getPTdcL0ID());
  d2->SetTag("PTDC"+deviceName+".TS", eTtc.getPTdcTs());
  d2->SetTag("PTDC"+deviceName+".BIT", eTtc.getPTdcBit());
  d2->SetTag("TLU"+deviceName+".TLUID", eTtc.getTluID());
  d2->SetTag("TLU"+deviceName+".L0ID", eTtc.getTluL0ID());
  d2->SetTag("TDC"+deviceName+".DATA", eTtc.getTdc());
  d2->SetTag("TDC"+deviceName+".L0ID", eTtc.getL0ID());
  d2->SetTag("TS"+deviceName+".DATA", eTtc.getTimestamp());
  d2->SetTag("TS"+deviceName+".L0ID", eTtc.getL0ID());
  d2->SetTag("TTC"+deviceName+".BCID", eTtc.getTtcBCID()); //ITSDAQ
  return true;
}
