#include "eudaq/StdEventConverter.hh"
#include "eudaq/RawEvent.hh"

class DPTSStatusEvent2StdEventConverter:public eudaq::StdEventConverter{
public:
  bool Converting(eudaq::EventSPC rawev,eudaq::StdEventSP stdev,eudaq::ConfigSPC conf) const override;
};

#define REGISTER_CONVERTER(name) namespace{auto dummy##name=eudaq::Factory<eudaq::StdEventConverter>::Register<DPTSStatusEvent2StdEventConverter>(eudaq::cstr2hash(#name));}
REGISTER_CONVERTER(DPTS_status)
REGISTER_CONVERTER(DPTS_0_status)

bool DPTSStatusEvent2StdEventConverter::Converting(eudaq::EventSPC in,eudaq::StdEventSP out,eudaq::ConfigSPC conf) const{
  auto rawev=std::dynamic_pointer_cast<const eudaq::RawEvent>(in);

  std::string descr=rawev->GetDescription();
  //std::cout << "### EUDAQ2 ### Found subevent with description: " << descr << std::endl;

  out->SetTag("Event",in->GetTag("Event"));
  out->SetTag("Time",in->GetTag("Time"));

  // always return false. Force Corryvreckan to skip events of this type
  return false;
}
