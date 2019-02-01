#include "eudaq/OptionParser.hh"
#include "eudaq/DataConverter.hh"
#include "eudaq/FileWriter.hh"
#include "eudaq/FileReader.hh"
#include <iostream>

#include "eudaq/DataCollector.hh"

int main(int /*argc*/, const char **argv) {

  eudaq::OptionParser op("EUDAQ Command Line DataMerger", "2.0", "The Data Merger launcher of EUDAQ");
  eudaq::Option<std::string> file_input_fasts(op, "fasts", "fasts", "", "string",
          "input file 1");
  eudaq::Option<std::string> file_input_ni(op, "ni", "ni", "", "string",
          "input file 2");

  eudaq::Option<std::string> file_output(op, "o", "output", "", "string",
          "output file");
  eudaq::OptionFlag iprint(op, "ip", "iprint", "enable print of input Event");

  try{
      op.Parse(argv);
  }
  catch (...) {
      return op.HandleMainException();
  }

  std::string infile_path_fasts = file_input_fasts.Value();
  std::string infile_path_ni = file_input_ni.Value();
  if(infile_path_fasts.empty() || infile_path_ni.empty()){
      std::cout<<"option --help to get help"<<std::endl;
      return 1;
  }

  std::string outfile_path = file_output.Value();
  std::string type_in_fasts = infile_path_fasts.substr(infile_path_fasts.find_last_of(".")+1);
  std::string type_in_ni = infile_path_ni.substr(infile_path_ni.find_last_of(".")+1);
  std::string type_out = outfile_path.substr(outfile_path.find_last_of(".")+1);
  bool print_ev_in = iprint.Value();

  if(type_in_fasts=="raw" && type_in_ni=="raw"){
      type_in_fasts = "native";
      type_in_ni = "native";
  }
  if(type_out=="raw")
      type_out = "native";


  eudaq::FileReaderUP reader_fasts;
  eudaq::FileReaderUP reader_ni;
  eudaq::FileWriterUP writer;


  reader_fasts = eudaq::Factory<eudaq::FileReader>::MakeUnique(eudaq::str2hash(type_in_fasts), infile_path_fasts);
  reader_ni = eudaq::Factory<eudaq::FileReader>::MakeUnique(eudaq::str2hash(type_in_ni), infile_path_ni);
  if(!type_out.empty())
      writer = eudaq::Factory<eudaq::FileWriter>::MakeUnique(eudaq::str2hash(type_out), outfile_path);

  uint32_t event_count = 0;
  uint32_t trigger_count = 0;

  auto ev_fasts = reader_fasts->GetNextEvent(); // was ev_slow
  auto ev_ni = reader_ni->GetNextEvent(); // was ev_fast



  int sync_event_number = 0;
  const uint32_t run_number = ev_fasts->GetRunN();


  while(1){

      if(!ev_fasts)
      {
        std::cout << "No more FASTS events..." << std::endl;
        break;
      }

      // initialise new event
      auto ev_sync =  eudaq::Event::MakeUnique("MimosaFasts");
      ev_sync->SetFlagPacket(); // copy from Ex0Tg
      ev_sync->SetTriggerN(ev_fasts->GetTriggerN());
      ev_sync->SetEventN(sync_event_number);
      ev_sync->SetRunN(run_number);


      //std::cout << "Sync Event created..." << std::endl;

      //Now get the NI events matching the FASTS event trigger ID

      if(ev_ni){
        while(ev_fasts->GetTriggerN() > ev_ni->GetTriggerN()){
          ev_ni = reader_ni->GetNextEvent();

          if(!ev_ni){
            std::cout << "No more NI events..." << std::endl;
            break;
          }
        }
      }

      if(!ev_ni){
        std::cout << "No more NI events..." << std::endl;
        break;
      }




      std::cout << "NI matched with FASTS..." << std::endl;

      //Now the FASTS and NI should be in ev_sync

      if(ev_fasts){
        while(ev_fasts->GetTriggerN() > ev_fasts->GetTriggerN()){
          ev_fasts = reader_fasts->GetNextEvent();

          if(!ev_fasts){
            std::cout << "No more FAST events..." << std::endl;
            break;
          }
        }
      }

      if(!ev_fasts){
        std::cout << "No more FASTS events..." << std::endl;
        break;
      }


      std::cout << "Sync Event " << sync_event_number
          << " created, Trigger IDs:" << ev_fasts->GetTriggerN() << " " 
          << ev_ni->GetTriggerN() << std::endl;

      //ev_sync->AddSubEvent(ev_fasts);
      uint32_t n = ev_fasts->GetNumSubEvent();
      for(uint32_t i = 0; i < n; i++){
        auto ev_fasts_sub = ev_fasts->GetSubEvent(i);
        ev_sync->AddSubEvent(ev_fasts_sub);
      }

      if(ev_fasts->GetTriggerN() == ev_ni->GetTriggerN()) ev_sync->AddSubEvent(ev_ni);

      //std::cout << "Sub events added..." << std::endl;

      if(writer && ev_fasts->GetTriggerN() == ev_ni->GetTriggerN()){
          writer->WriteEvent(std::move(ev_sync));
          std::cout << "Synch event written..." << std::endl;
          sync_event_number++;
      }

      ev_fasts = reader_fasts->GetNextEvent();


      if(!ev_fasts){
        std::cout << "No more FASTS events..." << std::endl;
        break;
      }

      std::cout << "Next FASTS event retrieved..." << std::endl;


    }


  return 0;
}
