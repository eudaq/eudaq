#include "eudaq/OptionParser.hh"
#include "eudaq/FileReader.hh"
#include "eudaq/StdEventConverter.hh"

#include <iostream>

int main(int /*argc*/, const char **argv) {
  eudaq::OptionParser op("EUDAQ Command Line FileReader modified for TLU", "2.1", "EUDAQ FileReader (TLU)");
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

  std::cout<< "runNumber,eventNumber,triggerNumber,timeStampBegin,triggersFired" <<std::endl;

  while(1){
    auto ev = reader->GetNextEvent();
    if(!ev)
      break;

    auto subevents = ev->GetSubEvents();
        for (auto &subev: subevents){
            auto subeventDescription = subev->GetDescription();
            // std::cout<< subeventDescription << std::endl;
            if (subeventDescription=="TluRawDataEvent") {
		auto eventNumber = subev->GetEventNumber();
		auto runNumber = subev->GetRunNumber();
		auto triggerNumber = subev->GetTriggerN();
		auto triggersFired = subev->GetTag("TRIGGER" , "NAN");
		auto timeStampBegin = subev->GetTimestampBegin();
                uint scalers [6] = { std::stoi(subev->GetTag("SCALER0" , "NAN")), std::stoi(subev->GetTag("SCALER1" , "NAN")) , std::stoi(subev->GetTag("SCALER2" , "NAN")) ,std::stoi(subev->GetTag("SCALER3" , "NAN")) ,std::stoi(subev->GetTag("SCALER4" , "NAN")) ,std::stoi(subev->GetTag("SCALER5" , "NAN"))    };
                uint fine_ts [6] = { std::stoi(subev->GetTag("FINE_TS0" , "NAN")), std::stoi(subev->GetTag("FINE_TS1" , "NAN")) , std::stoi(subev->GetTag("FINE_TS2" , "NAN")) ,std::stoi(subev->GetTag("FINE_TS3" , "NAN")) ,std::stoi(subev->GetTag("FINE_TS4" , "NAN")) ,std::stoi(subev->GetTag("FINE_TS5" , "NAN"))    };
		std::cout<< runNumber << " " << eventNumber << " " << triggerNumber << " " << timeStampBegin << " " << triggersFired << " " << fine_ts[0] << " " << fine_ts[1]<< " " << fine_ts[2] << " " << fine_ts[3] << " " << fine_ts[4] << " " << fine_ts[5] <<std::endl;
                //subev->Print(std::cout);
            }

        }
      // ev->Print(std::cout);
    }
  // std::cout<< "There are "<< event_count << "Events"<<std::endl;
  return 0;
}
