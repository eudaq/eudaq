#include "eudaq/StdEventConverter.hh"
#include "eudaq/RawEvent.hh"

class ALPIDEStatusEvent2StdEventConverter:public eudaq::StdEventConverter{
public:
  bool Converting(eudaq::EventSPC rawev,eudaq::StdEventSP stdev,eudaq::ConfigSPC conf) const override;
};

#define REGISTER_CONVERTER(name) namespace{auto dummy##name=eudaq::Factory<eudaq::StdEventConverter>::Register<ALPIDEStatusEvent2StdEventConverter>(eudaq::cstr2hash(#name));}
REGISTER_CONVERTER(ALPIDE_plane_0_status)
REGISTER_CONVERTER(ALPIDE_plane_1_status)
REGISTER_CONVERTER(ALPIDE_plane_2_status)
REGISTER_CONVERTER(ALPIDE_plane_3_status)
REGISTER_CONVERTER(ALPIDE_plane_4_status)
REGISTER_CONVERTER(ALPIDE_plane_5_status)
REGISTER_CONVERTER(ALPIDE_plane_6_status)
REGISTER_CONVERTER(ALPIDE_plane_7_status)
REGISTER_CONVERTER(ALPIDE_plane_8_status)
REGISTER_CONVERTER(ALPIDE_plane_9_status)
REGISTER_CONVERTER(ALPIDE_plane_10_status)
REGISTER_CONVERTER(ALPIDE_plane_11_status)
REGISTER_CONVERTER(ALPIDE_plane_12_status)
REGISTER_CONVERTER(ALPIDE_plane_13_status)
REGISTER_CONVERTER(ALPIDE_plane_14_status)
REGISTER_CONVERTER(ALPIDE_plane_15_status)
REGISTER_CONVERTER(ALPIDE_plane_16_status)
REGISTER_CONVERTER(ALPIDE_plane_17_status)
REGISTER_CONVERTER(ALPIDE_plane_18_status)
REGISTER_CONVERTER(ALPIDE_plane_19_status)

bool ALPIDEStatusEvent2StdEventConverter::Converting(eudaq::EventSPC in,eudaq::StdEventSP out,eudaq::ConfigSPC conf) const{
  auto rawev=std::dynamic_pointer_cast<const eudaq::RawEvent>(in);

  std::string descr=rawev->GetDescription();
  //std::cout << "### EUDAQ2 ### Found subevent with description: " << descr << std::endl;
  
  out->SetTag("IDDD",in->GetTag("IDDD"));
  out->SetTag("IDDA",in->GetTag("IDDA"));
  out->SetTag("Temperature",in->GetTag("Temperature"));
  out->SetTag("Event",in->GetTag("Event"));
  out->SetTag("Time",in->GetTag("Time"));

  // always return false. Force Corryvreckan to skip events of this type
  return false;
}
