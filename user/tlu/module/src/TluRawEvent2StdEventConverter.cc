#include "eudaq/StdEventConverter.hh"
#include "eudaq/RawEvent.hh"

class TluRawEvent2StdEventConverter: public eudaq::StdEventConverter{
public:
  bool Converting(eudaq::EventSPC d1, eudaq::StandardEventSP d2, eudaq::ConfigurationSPC conf) const override;
  static const uint32_t m_id_factory = eudaq::cstr2hash("TluRawDataEvent");
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::
    Register<TluRawEvent2StdEventConverter>(TluRawEvent2StdEventConverter::m_id_factory);
}

bool TluRawEvent2StdEventConverter::Converting(eudaq::EventSPC d1, eudaq::StandardEventSP d2, eudaq::ConfigurationSPC conf) const{
  if(!d2->IsFlagPacket()){
    d2->SetFlag(d1->GetFlag());
    d2->SetRunN(d1->GetRunN());
    d2->SetEventN(d1->GetEventN());
    d2->SetStreamN(d1->GetStreamN());
    d2->SetTriggerN(d1->GetTriggerN(), d1->IsFlagTrigger());
    d2->SetTimestamp(d1->GetTimestampBegin(), d1->GetTimestampEnd(), d1->IsFlagTimestamp());
  }
  else{
    static const std::string TLU("TLU");
    auto stm = std::to_string(d1->GetStreamN());
    d2->SetTag(TLU, stm+"_"+d2->GetTag(TLU));
    d2->SetTag(TLU+stm+"_TSB", std::to_string(d1->GetTimestampBegin()));
    d2->SetTag(TLU+stm+"_TSE", std::to_string(d1->GetTimestampEnd()));
    d2->SetTag(TLU+stm+"_TRG", std::to_string(d1->GetTriggerN()));
  }

  // Set times for StdEvent in picoseconds (timestamps provided in nanoseconds):
  d2->SetTimeBegin(d1->GetTimestampBegin() * 1000);
  d2->SetTimeEnd(d1->GetTimestampEnd() * 1000);

  // Identify the detetor type
  d2->SetDetectorType("TLU");
  return true;
}
