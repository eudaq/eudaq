#include "eudaq/StdEventConverter.hh"
#include "eudaq/RawEvent.hh"
#include <bitset>

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
  auto nTriggerInputs = __builtin_popcount(triggersFired & 0x3F); // count "ones" in binary
  // Note: __builtin_popcount counts the "ones" in a binary, see here:
  // https://www.geeksforgeeks.org/count-set-bits-in-an-integer/
  auto finets0 = std::stoi(d1->GetTag("FINE_TS0", "0"));
  auto finets1 = std::stoi(d1->GetTag("FINE_TS1", "0"));
  auto finets2 = std::stoi(d1->GetTag("FINE_TS2", "0"));
  auto finets3 = std::stoi(d1->GetTag("FINE_TS3", "0"));
  auto finets4 = std::stoi(d1->GetTag("FINE_TS4", "0"));
  auto finets5 = std::stoi(d1->GetTag("FINE_TS5", "0"));

  // add all valid trigger to vector:
  std::vector<uint8_t> finets_vec;
  if ((triggersFired >> 0) & 0x1) {
      finets_vec.emplace_back(finets0 & 0xFF);
  }
  if ((triggersFired >> 1) & 0x1) {
      finets_vec.emplace_back(finets1 & 0xFF);
  }
  if ((triggersFired >> 2) & 0x1) {
      finets_vec.emplace_back(finets2 & 0xFF);
  }
  if ((triggersFired >> 3) & 0x1) {
      finets_vec.emplace_back(finets3 & 0xFF);
  }
  if ((triggersFired >> 4) & 0x1) {
      finets_vec.emplace_back(finets4 & 0xFF);
  }
  if ((triggersFired >> 5) & 0x1) {
      finets_vec.emplace_back(finets5 & 0xFF);
  }
  if(finets_vec.size() < 2) {
      std::cout << "ERROR: received less than 2 trigger inputs" << std::endl;
      return false;

      // or throw exception:
      throw eudaq::DataInvalid("Received less than 2 trigger inputs.");

      // or is that even fine to have only one trigger input???
  }

  // Now average over all vector elements:
  // Initialize to 0th vector element:
  uint32_t finets_avg = 0xFF & finets_vec.at(0);

  // Calculate "rolling average":
  for(int i=1; i<finets_vec.size(); i++) {
      if(abs(finets_avg - finets_vec.at(i)) > 0xF0) {
          finets_avg += 0xFF; // overflow compensation
      }
      // Need to weight average with previous number of iterations:
      finets_avg = ((i*finets_avg + finets_vec.at(i)) / (i+1)) & 0xFF;

      // finets_avg += finets_vec.at(i);
      // finets_avg = (finets_avg / 2) & 0xFF;

      // This leads to rounding errors on the 781ps binning level
      // --> Convert to double in nanoseconds?
  }

  // This works well: using ONLY fineTS0 OR fineTS1:
  // auto finets_avg = finets0;
  // auto finets_avg = finets1;

  // This also works well:
  // Consider overflow:
  // if(abs(finets0 - finets1) > 0xF0) {
  //     // This only works for 2 inputs:
  //     // finets_avg = (finets_avg + 128) & 0xFF;
  //     finets1 += 0xFF;
  // }
  // uint8_t finets_avg = 0xFF & ((finets0 + finets1) / 2);

  auto coarse_ts = static_cast<uint64_t>(d1->GetTimestampBegin() / 25);

  uint8_t coarse_3lsb = 0x3 & coarse_ts;
  uint8_t finets_avg_3msb = (0xE0 & finets_avg) >> 5;

  // Casting to 32 bit is essential to detect the overflow in the 8th bit!
  // 0x100 = 200ns (range of 3 bits with 25ns binning)
  if(static_cast<uint32_t>(coarse_3lsb - finets_avg_3msb) > 0x100) {
      // coarse_ts is delayed by 2 clock cycles -> roll back
      coarse_ts -= 2;
  }

  // with combined fineTS: replace the 3lsb of the coarse timestamp with the 3msb of the finets_avg:
  auto ts = static_cast<uint64_t>((25. / 32. * (((coarse_ts << 5) & 0xFFFFFFFFFFFFFF00) + (finets_avg & 0xFF))) * 1000.);

  // Set times for StdEvent in picoseconds (timestamps provided in nanoseconds):
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
  // if(d1->IsFlagTrigger()) {
  //     std::cout << "TLU trigger number: " << d1->GetTriggerN() << std::endl;
  // }
  return true;
}
