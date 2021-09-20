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

  uint8_t triggersFired;
  uint32_t finets0;
  uint32_t finets1;
  uint32_t finets2;
  uint32_t finets3;
  uint32_t finets4;
  uint32_t finets5;

  uint8_t triggerMask{0x3F};
  uint32_t delay_scint0{0}, delay_scint1{0}, delay_scint2{0}, delay_scint3{0}, delay_scint4{0}, delay_scint5{0};
  if(conf != nullptr) {
      triggerMask = conf->Get("trigger_mask", 0x3F);
      delay_scint0 = conf->Get("delay_scint0", 0); // in 781.25ps bins
      delay_scint1 = conf->Get("delay_scint1", 0); // in 781.25ps bins
      delay_scint2 = conf->Get("delay_scint2", 0); // in 781.25ps bins
      delay_scint3 = conf->Get("delay_scint3", 0); // in 781.25ps bins
      delay_scint4 = conf->Get("delay_scint4", 0); // in 781.25ps bins
      delay_scint5 = conf->Get("delay_scint5", 0); // in 781.25ps bins
  }

  // try/catch for std::stoi()
  try {
    triggersFired = triggerMask & std::stoi(d1->GetTag("TRIGGER" , "0"), nullptr, 2); // interpret as binary and combine with triggerMask
  } catch (...) {
    EUDAQ_WARN("EUDAQ2 RawEvent flag TRIGGER cannot be interpreted as integer. Cannot calculate precise TLU TS. Return false.");
    return false;
  }

  // try/catch for std::stoi()
  try {
    // Subtract delay from fine timestamp:
    finets0 = static_cast<uint32_t>(std::stoi(d1->GetTag("FINE_TS0", "0"))) - delay_scint0;
    finets1 = static_cast<uint32_t>(std::stoi(d1->GetTag("FINE_TS1", "0"))) - delay_scint1;
    finets2 = static_cast<uint32_t>(std::stoi(d1->GetTag("FINE_TS2", "0"))) - delay_scint2;
    finets3 = static_cast<uint32_t>(std::stoi(d1->GetTag("FINE_TS3", "0"))) - delay_scint3;
    finets4 = static_cast<uint32_t>(std::stoi(d1->GetTag("FINE_TS4", "0"))) - delay_scint4;
    finets5 = static_cast<uint32_t>(std::stoi(d1->GetTag("FINE_TS5", "0"))) - delay_scint5;
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
  uint32_t finets_avg = (finets_vec.empty() ? 0 : (0xFF & finets_vec.at(0)));

  // Calculate "rolling average":
  for(int i=1; i<finets_vec.size(); i++) {
      // 128 = half the fineTS counter range -> 128*780ps = 99ns = 3m (TOF)
      // For a time difference larger than 99ns, the overflow detection doesn't work anymore.
      if(abs(finets_avg - finets_vec.at(i)) > 128) { // 128*780ps = 99ns
          finets_vec.at(i) += 0xFF; // overflow compensation
      }
      // Need to weight average with previous number of iterations:
      // (Don't truncate to 8-bit for correct averaging, i.e. don't apply 0xFF)
      finets_avg = ((i*finets_avg + finets_vec.at(i)) / (i+1));

      // This leads to rounding errors on the 781ps binning level
      // but otherwise the overflow detection logic breaks down.
  }

  auto coarse_ts = static_cast<uint64_t>(d1->GetTimestampBegin() / 25);

  uint8_t coarse_3lsb = 0x3 & coarse_ts;
  uint8_t finets_avg_3msb = (0xE0 & finets_avg) >> 5;

  // Casting to 32 bit is essential to detect the overflow in the 8th bit!
  // 0x100 = 200ns (range of 3 bits with 25ns binning)
  if((static_cast<uint32_t>(coarse_3lsb) - finets_avg_3msb) > 0x100) {
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
  d2->SetTag("TRIGGER", d1->GetTag("TRIGGER" , "0"));

  // forward original tags:
  d2->SetTag("FINE_TS0", d1->GetTag("FINE_TS0", "0")); // forward original tag
  d2->SetTag("FINE_TS1", d1->GetTag("FINE_TS1", "0")); // forward original tag
  d2->SetTag("FINE_TS2", d1->GetTag("FINE_TS2", "0")); // forward original tag
  d2->SetTag("FINE_TS3", d1->GetTag("FINE_TS3", "0")); // forward original tag
  d2->SetTag("FINE_TS4", d1->GetTag("FINE_TS4", "0")); // forward original tag
  d2->SetTag("FINE_TS5", d1->GetTag("FINE_TS5", "0")); // forward original tag

  // calculate (delayed) fine timestamps in ns:
  double finets0_ns = (finets0 & 0xFF) * 25. / 32.; // 781ps binning
  double finets1_ns = (finets1 & 0xFF) * 25. / 32.; // 781ps binning
  double finets2_ns = (finets2 & 0xFF) * 25. / 32.; // 781ps binning
  double finets3_ns = (finets3 & 0xFF) * 25. / 32.; // 781ps binning
  double finets4_ns = (finets4 & 0xFF) * 25. / 32.; // 781ps binning
  double finets5_ns = (finets5 & 0xFF) * 25. / 32.; // 781ps binning

  // differences between (delayed) fine timestamps:
  d2->SetTag("DIFF_FINETS01_del_ns", std::to_string((finets0_ns - finets1_ns)).c_str());
  d2->SetTag("DIFF_FINETS02_del_ns", std::to_string((finets0_ns - finets2_ns)).c_str());
  d2->SetTag("DIFF_FINETS03_del_ns", std::to_string((finets0_ns - finets3_ns)).c_str());
  d2->SetTag("DIFF_FINETS04_del_ns", std::to_string((finets0_ns - finets4_ns)).c_str());
  d2->SetTag("DIFF_FINETS05_del_ns", std::to_string((finets0_ns - finets5_ns)).c_str());

  d2->SetTag("DIFF_FINETS12_del_ns", std::to_string((finets1_ns - finets2_ns)).c_str());
  d2->SetTag("DIFF_FINETS13_del_ns", std::to_string((finets1_ns - finets3_ns)).c_str());
  d2->SetTag("DIFF_FINETS14_del_ns", std::to_string((finets1_ns - finets4_ns)).c_str());
  d2->SetTag("DIFF_FINETS15_del_ns", std::to_string((finets1_ns - finets5_ns)).c_str());

  d2->SetTag("DIFF_FINETS23_del_ns", std::to_string((finets3_ns - finets3_ns)).c_str());
  d2->SetTag("DIFF_FINETS24_del_ns", std::to_string((finets3_ns - finets4_ns)).c_str());
  d2->SetTag("DIFF_FINETS25_del_ns", std::to_string((finets3_ns - finets5_ns)).c_str());

  d2->SetTag("DIFF_FINETS34_del_ns", std::to_string((finets4_ns - finets4_ns)).c_str());
  d2->SetTag("DIFF_FINETS35_del_ns", std::to_string((finets4_ns - finets5_ns)).c_str());

  d2->SetTag("DIFF_FINETS45_del_ns", std::to_string((finets5_ns - finets5_ns)).c_str());

  return true;
}
