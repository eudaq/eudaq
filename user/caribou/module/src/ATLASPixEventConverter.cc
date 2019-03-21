#include "CaribouEvent2StdEventConverter.hh"

using namespace eudaq;

namespace{
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::
  Register<ATLASPixEvent2StdEventConverter>(ATLASPixEvent2StdEventConverter::m_id_factory);
}

bool ATLASPixEvent2StdEventConverter::Converting(eudaq::EventSPC d1, eudaq::StandardEventSP d2, eudaq::ConfigurationSPC conf) const{
  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);

  // Indicate that data was successfully converted
  return true;
}
