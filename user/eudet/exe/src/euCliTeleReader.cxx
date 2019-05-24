#include "eudaq/OptionParser.hh"
#include "eudaq/FileReader.hh"
#include "eudaq/StdEventConverter.hh"

#include <iostream>

int main(int /*argc*/, const char **argv) {
  eudaq::OptionParser op("EUDAQ Command Line FileReader", "2.1", 
          "EUDAQ FileReader modified for Mimosa/TLU");
  eudaq::Option<std::string> file_input(op, "i", "input", "", "string", "input file");
  eudaq::Option<uint32_t> eventl(op, "e", "event", 0, "uint32_t", "event number low");
  eudaq::Option<uint32_t> eventh(op, "E", "eventhigh", 0, "uint32_t", "event number high");
  eudaq::Option<uint32_t> triggerl(op, "tg", "trigger", 0, "uint32_t", "trigger number low");
  eudaq::Option<uint32_t> triggerh(op, "TG", "triggerhigh", 0, "uint32_t", "trigger number high");
  eudaq::Option<uint32_t> timestampl(op, "ts", "timestamp", 0, "uint32_t", "timestamp low");
  eudaq::Option<uint32_t> timestamph(op, "TS", "timestamphigh", 0, "uint32_t", "timestamp high");
  eudaq::OptionFlag csvout(op, "csv", "csv_output", "for output to save as csv, without last info line");

  op.Parse(argv);

  std::string infile_path = file_input.Value();
  std::string type_in = infile_path.substr(infile_path.find_last_of(".")+1);
  if(type_in=="raw")
    type_in = "native";

  uint32_t eventl_v = eventl.Value();
  uint32_t eventh_v = eventh.Value();
  uint32_t triggerl_v = triggerl.Value();
  uint32_t triggerh_v = triggerh.Value();
  uint64_t timestampl_v = timestampl.Value();
  uint64_t timestamph_v = timestamph.Value();
  bool not_all_zero = eventl_v||eventh_v||triggerl_v||triggerh_v||timestampl_v||timestamph_v;

  eudaq::FileReaderUP reader;
  reader = eudaq::Factory<eudaq::FileReader>::MakeUnique(eudaq::str2hash(type_in), infile_path);
  uint32_t event_count = 0;

  std::cout << "run,event,trigger,timestamp_low,timestamp_high,ni_trigger_number,ni_pivot_pixel" << std::endl;
  while(1){
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
      uint64_t ts_beg = ev->GetTimestampBegin();
      uint64_t ts_end = ev->GetTimestampEnd();
      if(ts_beg >= timestampl_v && ts_end <= timestamph_v){
	in_range_tsn = true;
      }
    }
    else
      in_range_tsn = true;


    if((in_range_evn && in_range_tgn && in_range_tsn) && not_all_zero){
      uint32_t run_number = ev->GetRunN();
      uint32_t event_number = ev->GetEventN();
      uint32_t trigger_number = ev->GetTriggerN();
      uint64_t ts_low = ev->GetTimestampBegin();
      uint64_t ts_high = ev->GetTimestampEnd();
      uint32_t ni_trigger_number = 0;
      uint16_t ni_pivot_pixel = 0;
      auto sub_events = ev->GetSubEvents();
      for(auto &sub_event : sub_events){
          if (sub_event->GetDescription() == "TluRawDataEvent") {
              ts_low = sub_event->GetTimestampBegin();
              ts_high = sub_event->GetTimestampEnd();
          }
          if (sub_event->GetDescription() == "NiRawDataEvent") {
              ni_trigger_number = sub_event->GetTriggerN();
              const std::vector<uint8_t> &data0 = sub_event->GetBlock(0);
              ni_pivot_pixel = eudaq::getlittleendian<uint16_t>(&data0[4]);
          }
      }
      std::cout << run_number << "," 
          << event_number << "," 
          << trigger_number << "," 
          << ts_low << "," 
          << ts_high << "," 
          << ni_trigger_number << "," 
          << ni_pivot_pixel << std::endl; 
    }

    event_count ++;
  }
  if(!csvout.Value())
    std::cout << "There are " << event_count << " Events" << std::endl;
  return 0;
}
