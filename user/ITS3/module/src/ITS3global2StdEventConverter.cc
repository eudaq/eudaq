#include "eudaq/StdEventConverter.hh"
#include "eudaq/RawEvent.hh"
#include <iostream>
#include <algorithm>

class ITS3global2StdEventConverter:public eudaq::StdEventConverter{
public:
  bool Converting(eudaq::EventSPC rawev,eudaq::StdEventSP stdev,eudaq::ConfigSPC conf) const override;
};

namespace{
  auto dummy=eudaq::Factory<eudaq::StdEventConverter>::Register<ITS3global2StdEventConverter>(eudaq::cstr2hash("ITS3global"));
}

bool ITS3global2StdEventConverter::Converting(eudaq::EventSPC in,eudaq::StdEventSP out,eudaq::ConfigSPC conf) const{
  auto subevents=in->GetSubEvents();
  std::sort(
    subevents.begin(),
    subevents.end(),
    [](decltype(*subevents.begin()) a,decltype(*subevents.begin()) b) -> bool {
      return a->GetDeviceN()<b->GetDeviceN();
    }
  );
  for(auto subev:subevents) {
    auto stdev=eudaq::StandardEvent::MakeShared();
    eudaq::StdEventConverter::Convert(subev,stdev,conf);
    for(size_t i=0;i<stdev->NumPlanes();++i) out->AddPlane(stdev->GetPlane(i));
  }
  return true;
}
