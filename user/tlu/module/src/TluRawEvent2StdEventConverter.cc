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
  }
  else{
    static const std::string TLU("TLU");
    auto stm = std::to_string(d1->GetStreamN());
    d2->SetTag(TLU, stm+"_"+d2->GetTag(TLU));
    d2->SetTag(TLU+stm+"_TSB", std::to_string(d1->GetTimestampBegin()));
    d2->SetTag(TLU+stm+"_TSE", std::to_string(d1->GetTimestampEnd()));
    d2->SetTag(TLU+stm+"_TRG", std::to_string(d1->GetTriggerN()));
  }

  auto triggersFired = std::stoi(d1->GetTag("TRIGGER" , "0"), nullptr, 2); // interpret as binary
  auto finets0 = std::stoi(d1->GetTag("FINE_TS0", "0"));
  auto finets1 = std::stoi(d1->GetTag("FINE_TS1", "0"));
  auto finets2 = std::stoi(d1->GetTag("FINE_TS2", "0"));
  auto finets3 = std::stoi(d1->GetTag("FINE_TS3", "0"));
  auto finets4 = std::stoi(d1->GetTag("FINE_TS4", "0"));
  auto finets5 = std::stoi(d1->GetTag("FINE_TS5", "0"));

  // Set times for StdEvent in picoseconds (timestamps provided in nanoseconds):
  // Note: __builtin_popcount counts the "ones" in a binary, see here:
  // https://www.geeksforgeeks.org/count-set-bits-in-an-integer/
  // I.e. add up all timestamps for which the triggersFired bit is 1 and divide by number of active triggers:
  // Factor 25./32. -> fine timestamp in 781ps binning
  // auto finets = ((finets0 * ((triggersFired >> 0) & 0x1))
  //              + (finets1 * ((triggersFired >> 1) & 0x1))
  //              + (finets2 * ((triggersFired >> 2) & 0x1))
  //              + (finets3 * ((triggersFired >> 3) & 0x1))
  //              + (finets4 * ((triggersFired >> 4) & 0x1))
  //              + (finets5 * ((triggersFired >> 5) & 0x1)))
  //              / __builtin_popcount(triggersFired & 0x3F); // count "ones" in binary
  // The averaging works fine, I tested that with a cout!

  auto finets = finets0; //--> this works perfectly!
  // auto finets = finets1; //--> also works perfectly!
  // auto earliest_ts = (finets0 < finets1) ? finet caution overflow!

  auto coarse_ts = static_cast<uint64_t>(d1->GetTimestampBegin() / 25);

  uint8_t coarse_3lsb = 0x3 & coarse_ts;
  uint8_t finets_3msb = (0xE0 & finets) >> 5;

  // I think the problem relates to rounding errors when averaging fineTS0 and fineTS1

  // Casting to 32 bit is essential to detect the overflow in the 8th bit!
  if(static_cast<uint32_t>(coarse_3lsb - finets_3msb) > 0x100) { // 200ns (range of 3 bits with 25ns binning)
      // but WHY is it 2 and not 1?
      coarse_ts -= 2;
  }

  // with combined fineTS: replace the 3lsb of the coarse timestamp with the 3msb of the finets:
  auto ts = static_cast<uint64_t>((25. / 32. * (((coarse_ts << 5) & 0xFFFFFFFFFFFFFF00) + (finets & 0xFF))) * 1000.);

  d2->SetTimeBegin(ts);
  d2->SetTimeEnd(d1->GetTimestampEnd() * 1000);
  d2->SetTimestamp(ts, d1->GetTimestampEnd(), d1->IsFlagTimestamp());

  // Identify the detetor type
  d2->SetDetectorType("TLU");
  d2->SetTag("TRIGGER", triggersFired);
  d2->SetTag("FINE_TS0", finets0);
  d2->SetTag("FINE_TS1", finets1);
  d2->SetTag("FINE_TS2", finets2);
  d2->SetTag("FINE_TS3", finets3);
  d2->SetTag("FINE_TS4", finets4);
  d2->SetTag("FINE_TS5", finets5);
  return true;
}
