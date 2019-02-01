#include "eudaq/OptionParser.hh"
#include "eudaq/DataConverter.hh"
#include "eudaq/FileWriter.hh"
#include "eudaq/FileReader.hh"
#include <iostream>

#include "eudaq/DataCollector.hh"

int main(int /*argc*/, const char **argv) {

  eudaq::OptionParser op("EUDAQ Command Line DataMerger", "2.0", 
          "Event building by event ID; EUDAQ1 event defined by trigger-global-busy communication: one trigger = one read-out from each device");
  eudaq::Option<std::string> file_input_tlu(op, "tlu", "tlu", "", "string",
          "input file 1");
  eudaq::Option<std::string> file_input_ni(op, "ni", "ni", "", "string",
          "input file 2");
  eudaq::Option<std::string> file_input_dut(op, "dut", "dut", "", "string",
          "input file 3");

  eudaq::Option<std::string> file_output(op, "o", "output", "", "string",
          "output file");
  eudaq::OptionFlag iprint(op, "ip", "iprint", "enable print of input Event");

  try{
      op.Parse(argv);
  }
  catch (...) {
      return op.HandleMainException();
  }

  std::string infile_path_tlu = file_input_tlu.Value();
  std::string infile_path_ni = file_input_ni.Value();
  std::string infile_path_dut = file_input_dut.Value();
  if(infile_path_tlu.empty() || infile_path_ni.empty() || infile_path_dut.empty()){
      std::cout<<"option --help to get help"<<std::endl;
      return 1;
  }

  std::string outfile_path = file_output.Value();
  std::string type_in_tlu = infile_path_tlu.substr(infile_path_tlu.find_last_of(".")+1);
  std::string type_in_ni = infile_path_ni.substr(infile_path_ni.find_last_of(".")+1);
  std::string type_in_dut = infile_path_dut.substr(infile_path_dut.find_last_of(".")+1);
  std::string type_out = outfile_path.substr(outfile_path.find_last_of(".")+1);
  bool print_ev_in = iprint.Value();

  if(type_in_tlu=="raw" && type_in_ni=="raw" && type_in_dut=="raw"){
      type_in_tlu = "native";
      type_in_ni = "native";
      type_in_dut = "native";
  }
  if(type_out=="raw")
      type_out = "native";


  eudaq::FileReaderUP reader_tlu;
  eudaq::FileReaderUP reader_ni;
  eudaq::FileReaderUP reader_dut;
  eudaq::FileWriterUP writer;


  reader_tlu = eudaq::Factory<eudaq::FileReader>::MakeUnique(eudaq::str2hash(type_in_tlu), infile_path_tlu);
  reader_ni = eudaq::Factory<eudaq::FileReader>::MakeUnique(eudaq::str2hash(type_in_ni), infile_path_ni);
  reader_dut = eudaq::Factory<eudaq::FileReader>::MakeUnique(eudaq::str2hash(type_in_dut), infile_path_dut);
  if(!type_out.empty())
      writer = eudaq::Factory<eudaq::FileWriter>::MakeUnique(eudaq::str2hash(type_out), outfile_path);

  uint32_t event_count = 0;
  uint32_t trigger_count = 0;

  auto ev_tlu = reader_tlu->GetNextEvent(); // was ev_slow
  auto ev_ni = reader_ni->GetNextEvent(); // was ev_fast
  auto ev_dut = reader_dut->GetNextEvent(); // was ev_fast2



  int sync_event_number = 0;
  const uint32_t run_number = ev_tlu->GetRunN();


  while(1){

      if(!ev_tlu)
      {
        std::cout << "No more TLU events..." << std::endl;
        break;
      }

      // initialise new event
      auto ev_sync =  eudaq::Event::MakeUnique("MimosaTlu");
      ev_sync->SetFlagPacket(); // copy from Ex0Tg
      ev_sync->SetTriggerN(ev_tlu->GetTriggerN());
      ev_sync->SetEventN(sync_event_number);
      ev_sync->SetRunN(run_number);


      std::cout << "Sync Event created..." << std::endl;

      //Now get the NI events matching the TLU event trigger ID

      if(ev_ni){
        while(ev_tlu->GetEventN() > ev_ni->GetEventN()){
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




      std::cout << "NI matched with TLU..." << std::endl;

      //Now the TLU and NI should be in ev_sync, get the DUT in sync too

      if(ev_dut){
        while(ev_tlu->GetEventN() > ev_dut->GetEventN()){
          ev_dut = reader_dut->GetNextEvent();

          if(!ev_dut){
            std::cout << "No more DUT events..." << std::endl;
            break;
          }
        }
      }

      if(!ev_dut){
        std::cout << "No more DUT events..." << std::endl;
        break;
      }


      std::cout << "FEI4 matched with TLU..." << std::endl;

      ev_sync->AddSubEvent(ev_tlu);

      if(ev_tlu->GetEventN() == ev_ni->GetEventN()) ev_sync->AddSubEvent(ev_ni);

      if(ev_tlu->GetEventN() == ev_dut->GetEventN()) ev_sync->AddSubEvent(ev_dut);

      std::cout << "Sub events added..." << std::endl;

      if(writer && ev_tlu->GetEventN() == ev_ni->GetEventN() && ev_tlu->GetEventN() == ev_dut->GetEventN()){
          writer->WriteEvent(std::move(ev_sync));
          sync_event_number++;
      }

      std::cout << "Synch event written..." << std::endl;

      ev_tlu = reader_tlu->GetNextEvent();


      if(!ev_tlu){
        std::cout << "No more TLU events..." << std::endl;
        break;
      }

      std::cout << "Next TLU event retrieved..." << std::endl;


    }


  return 0;
}
