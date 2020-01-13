#include "eudaq/OptionParser.hh"
#include "eudaq/FileReader.hh"
#include "eudaq/StdEventConverter.hh"

#include <iostream>

int main(int /*argc*/, const char **argv) {
  eudaq::OptionParser op("EUDAQ Command Line FileReader modified for TLU data", "2.1", "EUDAQ FileReader (TLU)");
  eudaq::Option<std::string> file_input(op, "i", "input", "", "string", "input file");
  eudaq::Option<uint32_t> eventl(op, "e", "event", 0, "uint32_t", "event number low");
  eudaq::Option<uint32_t> eventh(op, "E", "eventhigh", 0, "uint32_t", "event number high");
  eudaq::Option<uint32_t> triggerl(op, "tg", "trigger", 0, "uint32_t", "trigger number low");
  eudaq::Option<uint32_t> triggerh(op, "TG", "triggerhigh", 0, "uint32_t", "trigger number high");
  eudaq::Option<uint32_t> timestampl(op, "ts", "timestamp", 0, "uint32_t", "timestamp low");
  eudaq::Option<uint32_t> timestamph(op, "TS", "timestamphigh", 0, "uint32_t", "timestamp high");
  eudaq::OptionFlag stat(op, "s", "statistics", "enable print of statistics");
  eudaq::OptionFlag stdev(op, "std", "stdevent", "enable converter of StdEvent");

  op.Parse(argv);

  std::string infile_path = file_input.Value();
  std::string type_in = infile_path.substr(infile_path.find_last_of(".")+1);
  if(type_in=="raw")
    type_in = "native";

  uint32_t eventl_v = eventl.Value();
  uint32_t eventh_v = eventh.Value();
  uint32_t triggerl_v = triggerl.Value();
  uint32_t triggerh_v = triggerh.Value();
  uint32_t timestampl_v = timestampl.Value();
  uint32_t timestamph_v = timestamph.Value();
  bool not_all_zero = eventl_v||eventh_v||triggerl_v||triggerh_v||timestampl_v||timestamph_v;

  eudaq::FileReaderUP reader;
  reader = eudaq::Factory<eudaq::FileReader>::MakeUnique(eudaq::str2hash(type_in), infile_path);
  uint32_t event_count = 0;

  std::cout<< "run,event,trigger,timestamp_low,timestamp_high,particles,triggersFired,scaler0,scaler1,scaler2,scaler3,scaler4,scaler5,finets0,finets1,finets2,finets3,finets4,finets5" <<std::endl;

  while(1) {
    auto ev = reader->GetNextEvent();
    if(!ev)
      break;

    bool in_range_evn = false;
    if(eventl_v!=0 || eventh_v!=0){
      uint32_t ev_n = ev->GetEventN();
      if(ev_n >= eventl_v && ev_n < eventh_v){
        in_range_evn = true;
      }
    }
    else
      in_range_evn = true;

    bool in_range_tgn = false;
    if(triggerl_v!=0 || triggerh_v!=0){
      uint32_t tg_n = ev->GetTriggerN();
      if(tg_n >= triggerl_v && tg_n < triggerh_v){
        in_range_tgn = true;
      }
    }
    else
      in_range_tgn = true;

    bool in_range_tsn = false;
    if(timestampl_v!=0 || timestamph_v!=0){
      uint32_t ts_beg = ev->GetTimestampBegin();
      uint32_t ts_end = ev->GetTimestampEnd();
      if(ts_beg >= timestampl_v && ts_end <= timestamph_v){
        in_range_tsn = true;
      }
    }
    else
      in_range_tsn = true;

    if (ev->GetDescription()=="TluRawDataEvent" && in_range_evn) {
      auto runNumber = ev->GetRunNumber();
      auto eventNumber = ev->GetEventNumber();
      auto triggerNumber = ev->GetTriggerN();
      auto timeStampBegin = ev->GetTimestampBegin();
      auto timeStampEnd = ev->GetTimestampEnd();
      auto particles = ev->GetTag("PARTICLES", "NAN");
      auto triggersFired = ev->GetTag("TRIGGER", "NAN");
      auto scaler0 = ev->GetTag("SCALER0", "NAN");
      auto scaler1 = ev->GetTag("SCALER1", "NAN");
      auto scaler2 = ev->GetTag("SCALER2", "NAN");
      auto scaler3 = ev->GetTag("SCALER3", "NAN");
      auto scaler4 = ev->GetTag("SCALER4", "NAN");
      auto scaler5 = ev->GetTag("SCALER5", "NAN");
      auto finets0 = ev->GetTag("FINE_TS0", "NAN");
      auto finets1 = ev->GetTag("FINE_TS1", "NAN");
      auto finets2 = ev->GetTag("FINE_TS2", "NAN");
      auto finets3 = ev->GetTag("FINE_TS3", "NAN");
      auto finets4 = ev->GetTag("FINE_TS4", "NAN");
      auto finets5 = ev->GetTag("FINE_TS5", "NAN");
      std::cout << runNumber << "," <<
      		   eventNumber << "," <<
		   triggerNumber << "," <<
		   timeStampBegin << "," <<
		   timeStampEnd << "," <<
		   particles << "," <<
		   triggersFired << "," <<
		   scaler0 << "," << scaler1 << "," << scaler2 << "," << scaler3 << "," << scaler4 << "," << scaler5 << "," <<
		   finets0 << "," << finets1 << "," << finets2 << "," << finets3 << "," << finets4 << "," << finets5 <<
		   std::endl;
      // ev->Print(std::cout);
    }

    auto subevents = ev->GetSubEvents();
      for (auto &subev: subevents) {
        auto subeventDescription = subev->GetDescription();
        // std::cout<< subeventDescription << std::endl;
        if (subeventDescription=="TluRawDataEvent" && in_range_evn) {
	  auto runNumber = subev->GetRunNumber();
	  auto eventNumber = subev->GetEventNumber();
	  auto triggerNumber = subev->GetTriggerN();
	  auto timeStampBegin = subev->GetTimestampBegin();
	  auto timeStampEnd = subev->GetTimestampEnd();
          auto particles = subev->GetTag("PARTICLES", "NAN");
	  auto triggersFired = subev->GetTag("TRIGGER" , "NAN");
          auto scaler0 = subev->GetTag("SCALER0", "NAN"); 
          auto scaler1 = subev->GetTag("SCALER1", "NAN"); 
          auto scaler2 = subev->GetTag("SCALER2", "NAN");
          auto scaler3 = subev->GetTag("SCALER3", "NAN");
          auto scaler4 = subev->GetTag("SCALER4", "NAN");
          auto scaler5 = subev->GetTag("SCALER5", "NAN");
          auto finets0 = subev->GetTag("FINE_TS0", "NAN");
          auto finets1 = subev->GetTag("FINE_TS1", "NAN");
          auto finets2 = subev->GetTag("FINE_TS2", "NAN");
          auto finets3 = subev->GetTag("FINE_TS3", "NAN");
          auto finets4 = subev->GetTag("FINE_TS4", "NAN");
          auto finets5 = subev->GetTag("FINE_TS5", "NAN");
          std::cout << runNumber << "," <<
      		   eventNumber << "," <<
		   triggerNumber << "," <<
		   timeStampBegin << "," <<
		   timeStampEnd << "," <<
		   particles << "," <<
		   triggersFired << "," <<
		   scaler0 << "," << scaler1 << "," << scaler2 << "," << scaler3 << "," << scaler4 << "," << scaler5 << "," <<
		   finets0 << "," << finets1 << "," << finets2 << "," << finets3 << "," << finets4 << "," << finets5 <<
		   std::endl;
          //subev->Print(std::cout);
          }
        }
      event_count++;
    }
  std::cout << "There are " << event_count << " Events" << std::endl;
  return 0;
}
