#include "eudaq/StdEventConverter.hh"
#include "eudaq/RawEvent.hh"

class POWERStatusEvent2StdEventConverter:public eudaq::StdEventConverter{
public:
  bool Converting(eudaq::EventSPC rawev,eudaq::StdEventSP stdev,eudaq::ConfigSPC conf) const override;
};

#define REGISTER_CONVERTER(name) namespace{auto dummy##name=eudaq::Factory<eudaq::StdEventConverter>::Register<POWERStatusEvent2StdEventConverter>(eudaq::cstr2hash(#name));}
REGISTER_CONVERTER(POWER_status)
REGISTER_CONVERTER(POWER_0_status)
REGISTER_CONVERTER(POWER_1_status)
REGISTER_CONVERTER(POWER_2_status)
REGISTER_CONVERTER(POWER_3_status)

bool POWERStatusEvent2StdEventConverter::Converting(eudaq::EventSPC in,eudaq::StdEventSP out,eudaq::ConfigSPC conf) const{
  auto rawev=std::dynamic_pointer_cast<const eudaq::RawEvent>(in);


  std::string descr=rawev->GetDescription();
  //std::cout << "### EUDAQ2 ### Found subevent with description: " << descr << std::endl;

  out->SetTag("Time",in->GetTag("Time"));
  for (int ch = 1; ch < 5; ch++) {
    out->SetTag("enabled_"     +std::to_string(ch),in->GetTag("enabled_"     +std::to_string(ch)));
    out->SetTag("fused_"       +std::to_string(ch),in->GetTag("fused_"       +std::to_string(ch)));
    out->SetTag("voltage_set_" +std::to_string(ch),in->GetTag("voltage_set_" +std::to_string(ch)));
    out->SetTag("current_set_" +std::to_string(ch),in->GetTag("current_set_" +std::to_string(ch)));
    out->SetTag("voltage_meas_"+std::to_string(ch),in->GetTag("voltage_meas_"+std::to_string(ch)));
    out->SetTag("current_meas_"+std::to_string(ch),in->GetTag("current_meas_"+std::to_string(ch)));
  }
  out->SetTag("IDN",in->GetTag("IDN"));

  // always return false. Force Corryvreckan to skip events of this type
  return false;
}
