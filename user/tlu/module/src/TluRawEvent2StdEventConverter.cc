#include "eudaq/StdEventConverter.hh"
#include "eudaq/RawEvent.hh"
#include <cmath> // for round()

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

  uint8_t triggersFired;
  uint32_t finets0;
  uint32_t finets1;
  uint32_t finets2;
  uint32_t finets3;
  uint32_t finets4;
  uint32_t finets5;

  // Should I read these in at m_first_time and store in static member variable or read in each time?
  double tof_scint0 = conf->Get("tof_scint0", 1.); // in nanoseconds
  double tof_scint1 = conf->Get("tof_scint1", 1.); // in nanoseconds
  double tof_scint2 = conf->Get("tof_scint2", 1.); // in nanoseconds
  double tof_scint3 = conf->Get("tof_scint3", 1.); // in nanoseconds
  double tof_scint4 = conf->Get("tof_scint4", 1.); // in nanoseconds
  double tof_scint5 = conf->Get("tof_scint5", 1.); // in nanoseconds
  try {
    triggersFired = 0x3F & std::stoi(d1->GetTag("TRIGGER" , "0"), nullptr, 2); // interpret as binary
  } catch (...) {
    EUDAQ_WARN("EUDAQ2 RawEvent flag TRIGGER cannot be interpreted as integer. Cannot calculate precise TLU TS. Return false.");
    return false;
  }
  try {
    finets0 = std::stoi(d1->GetTag("FINE_TS0", "0")) + static_cast<uint32_t>(round(tof_scint0 / 0.78125)); // convert into 781.25ps bins and round
    finets1 = std::stoi(d1->GetTag("FINE_TS1", "0")) + static_cast<uint32_t>(round(tof_scint1 / 0.78125)); // convert into 781.25ps bins and round
    finets2 = std::stoi(d1->GetTag("FINE_TS2", "0")) + static_cast<uint32_t>(round(tof_scint2 / 0.78125)); // convert into 781.25ps bins and round
    finets3 = std::stoi(d1->GetTag("FINE_TS3", "0")) + static_cast<uint32_t>(round(tof_scint3 / 0.78125)); // convert into 781.25ps bins and round
    finets4 = std::stoi(d1->GetTag("FINE_TS4", "0")) + static_cast<uint32_t>(round(tof_scint4 / 0.78125)); // convert into 781.25ps bins and round
    finets5 = std::stoi(d1->GetTag("FINE_TS5", "0")) + static_cast<uint32_t>(round(tof_scint5 / 0.78125)); // convert into 781.25ps bins and round
} catch (...) {
    EUDAQ_WARN("EUDAQ2 RawEvent flag FINE_TS<0-5> cannot be interpreted as integer. Cannot calculate precise TLU TS. Return false.");
    return false;
}

  // add all valid trigger to vector:
  std::vector<uint32_t> finets_vec;
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

  // Now average over all vector elements:
  // Initialize to 0th vector element:
  uint32_t finets_avg = 0xFF & finets_vec.at(0);

  // Calculate "rolling average":
  for(int i=1; i<finets_vec.size(); i++) {
      // 128 = half the fineTS counter range -> 128*780ps = 99ns = 3m (TOF)
      // For a time difference larger than 99ns, the overflow detection doesn't work anymore.
      // If the delay (TOF/cabling) between scintillators exceeds 99ns, the TOFs should be
      // specified as input parameters for correction.
      if(abs(finets_avg - finets_vec.at(i)) > 128) { // 128*780ps = 99ns
          finets_vec.at(i) += 0xFF; // overflow compensation
      }
      // Need to weight average with previous number of iterations:
      // (Don't truncate to 8-bit for correct averaging, i.e. don't apply 0xFF)
      finets_avg = ((i*finets_avg + finets_vec.at(i)) / (i+1));

      // This leads to rounding errors on the 781ps binning level
      // --> Convert to double in nanoseconds?
  }

  /*
  // This works well: using ONLY fineTS0 OR fineTS1:
  auto finets_avg = finets0;
  auto finets_avg = finets1;
  */

  /*
  // This also works well (only for 2 inputs):
  // Consider overflow:
  if(abs(finets0 - finets1) > 0xF0) {
      finets1 += 0xFF;
  }
  finets_avg = 0xFF & ((finets0 + finets1) / 2);
  */

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
  return true;
}
