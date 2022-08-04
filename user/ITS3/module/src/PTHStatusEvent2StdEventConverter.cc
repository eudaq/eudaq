#include "eudaq/StdEventConverter.hh"
#include "eudaq/RawEvent.hh"

class PTHStatusEvent2StdEventConverter:public eudaq::StdEventConverter{
public:
  bool Converting(eudaq::EventSPC rawev,eudaq::StdEventSP stdev,eudaq::ConfigSPC conf) const override;
};

#define REGISTER_CONVERTER(name) namespace{auto dummy##name=eudaq::Factory<eudaq::StdEventConverter>::Register<PTHStatusEvent2StdEventConverter>(eudaq::cstr2hash(#name));}
REGISTER_CONVERTER(PTH_status)
REGISTER_CONVERTER(PTH_0_status)

bool PTHStatusEvent2StdEventConverter::Converting(eudaq::EventSPC in,eudaq::StdEventSP out,eudaq::ConfigSPC conf) const{
  auto rawev=std::dynamic_pointer_cast<const eudaq::RawEvent>(in);


  std::string descr=rawev->GetDescription();
  //std::cout << "### EUDAQ2 ### Found subevent with description: " << descr << std::endl;

  out->SetTag("Time",       in->GetTag("Time"));
  out->SetTag("Pressure",   in->GetTag("Pressure"));
  out->SetTag("Temperature",in->GetTag("Temperature"));
  out->SetTag("Humidity",   in->GetTag("Humidity"));

  // always return false. Force Corryvreckan to skip events of this type
  return false;
}
