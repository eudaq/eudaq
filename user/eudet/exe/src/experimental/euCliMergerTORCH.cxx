#include "eudaq/OptionParser.hh"
#include "eudaq/DataConverter.hh"
#include "eudaq/FileWriter.hh"
#include "eudaq/FileReader.hh"
#include <iostream>

#include "eudaq/DataCollector.hh"

int main(int /*argc*/, const char **argv) {

  eudaq::OptionParser op("EUDAQ Command Line DataMerger", "2.0", "The Data Merger launcher of EUDAQ");
  eudaq::Option<std::string> file_input_1(op, "is", "input1", "", "string",
          "input file 1");
  eudaq::Option<std::string> file_input_2(op, "if1", "input2", "", "string",
          "input file 2");
  eudaq::Option<std::string> file_input_3(op, "if2", "input3", "", "string",
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

  std::string infile_path_1 = file_input_1.Value();
  std::string infile_path_2 = file_input_2.Value();
  std::string infile_path_3 = file_input_3.Value();
  if(infile_path_1.empty() || infile_path_2.empty() || infile_path_3.empty()){
      std::cout<<"option --help to get help"<<std::endl;
      return 1;
  }

  std::string outfile_path = file_output.Value();
  std::string type_in_1 = infile_path_1.substr(infile_path_1.find_last_of(".")+1);
  std::string type_in_2 = infile_path_2.substr(infile_path_2.find_last_of(".")+1);
  std::string type_in_3 = infile_path_3.substr(infile_path_3.find_last_of(".")+1);
  std::string type_out = outfile_path.substr(outfile_path.find_last_of(".")+1);
  bool print_ev_in = iprint.Value();

  if(type_in_1=="raw" && type_in_2=="raw" && type_in_3=="raw"){
      type_in_1 = "native";
      type_in_2 = "native";
      type_in_3 = "native";
  }
  if(type_out=="raw")
      type_out = "native";

  //TODO find out which reader goes with which stream... Assumming the NI stream is the slow reader.
  eudaq::FileReaderUP reader_slow;
  eudaq::FileReaderUP reader_fast;
  eudaq::FileReaderUP reader_fast2; //Addng in extra reader to handle the additional stream... I think this is correct...
  eudaq::FileWriterUP writer;

  // TODO: get type and define fast and slow device
  reader_slow = eudaq::Factory<eudaq::FileReader>::MakeUnique(eudaq::str2hash(type_in_1), infile_path_1);
  reader_fast = eudaq::Factory<eudaq::FileReader>::MakeUnique(eudaq::str2hash(type_in_2), infile_path_2);
  reader_fast2 = eudaq::Factory<eudaq::FileReader>::MakeUnique(eudaq::str2hash(type_in_3), infile_path_3);
  if(!type_out.empty())
      writer = eudaq::Factory<eudaq::FileWriter>::MakeUnique(eudaq::str2hash(type_out), outfile_path);

  uint32_t event_count = 0;
  uint32_t trigger_count = 0;

  auto ev_slow = reader_slow->GetNextEvent();
  auto ev_fast = reader_fast->GetNextEvent();
  auto ev_fast2 = reader_fast2->GetNextEvent();


  uint32_t trigger_ID_slow = ev_slow->GetTriggerN();
  uint32_t trigger_ID_fast = ev_fast->GetTriggerN();
  uint32_t trigger_ID_fast2 = ev_fast2->GetTriggerN();

  std::cout<< "First trigger ID slow " << trigger_ID_slow
      << " fast " << trigger_ID_fast << " fast2 " << trigger_ID_fast2 << std::endl;


  //Altered the code up to here. Now need to understand logic so can change appropriately.
  //Note changes up to this point build correctly.
  while(1){
      if(!ev_slow || !ev_fast || !ev_fast2){
          std::cout<< "empty" << std::endl;
          break;
      }

      // initialise new event
      auto ev_sync =  eudaq::Event::MakeUnique("MimosaTlu");
      ev_sync->SetFlagPacket(); // copy from Ex0Tg
      ev_sync->SetTriggerN(ev_slow->GetTriggerN());
      ev_sync->SetEventN(ev_slow->GetEventN());

      // find next common trigger id
      // count up trigger ID slow, if smaller
      if(trigger_ID_slow < trigger_ID_fast){
          // count up trigger ID slow
          while(trigger_ID_slow < trigger_ID_fast){
              ev_slow = reader_slow->GetNextEvent();
              trigger_ID_slow = ev_slow->GetTriggerN();
          }
          std::cout<< "Same trigger ID slow " << trigger_ID_slow
              << " fast " << trigger_ID_fast << std::endl;
      }
      // count up trigger ID fast, if smaller, and do not write data
      else if(trigger_ID_slow > trigger_ID_fast){
          trigger_count = 1;
          while(trigger_ID_slow > trigger_ID_fast){
              ev_fast = reader_fast->GetNextEvent();
              trigger_ID_fast = ev_fast->GetTriggerN();
              trigger_count ++;
          }
          std::cout<< " plus fast trigger " << trigger_ID_fast
              << " (" << trigger_count << ")"  << std::endl;
      }
      // count up trigger ID fast, if equal
      else if(trigger_ID_slow == trigger_ID_fast){
          std::cout<< "Trigger ID " << trigger_ID_slow << std::endl;


          uint32_t sub_ev_slow_number = ev_slow->GetNumSubEvent();
          for(uint32_t index = 0; index < sub_ev_slow_number; index++){
              auto sub_ev_slow = ev_slow->GetSubEvent(index);
              ev_sync->AddSubEvent(sub_ev_slow);
          }

          // next trigger ID slow for next loop
          ev_slow = reader_slow->GetNextEvent();
          trigger_ID_slow = ev_slow->GetTriggerN();
          //This is being used to exit the main loop once all slow events have been read. CHANGE THIS APPROACH.
          if(!ev_slow){
              std::cout<< "no slow event left" << std::endl;
              break;
          }

          // loop: get all fast trigger
          trigger_count = 0;
          while(trigger_ID_slow > trigger_ID_fast){
              // std::cout<< trigger_ID_fast << std::endl;
              if(!ev_fast){
                  std::cout<< "no fast event left" << std::endl;
                  break;
              }
              // write fast device subevents
              uint32_t sub_ev_fast_number = ev_fast->GetNumSubEvent();
              for(uint32_t index = 0; index < sub_ev_fast_number; index++){
                  auto sub_ev_fast = ev_fast->GetSubEvent(index);
                  ev_sync->AddSubEvent(sub_ev_fast);
              }
              // next fast sub event
              ev_fast = reader_fast->GetNextEvent();
              trigger_ID_fast = ev_fast->GetTriggerN();

              trigger_count ++;

          }
          //TODO: Implement new code here!
          trigger_count = 0;


          while(trigger_ID_slow > trigger_ID_fast2){
              // std::cout<< trigger_ID_fast << std::endl;
              if(!ev_fast2){
                  std::cout<< "no fast2 events left" << std::endl;
                  break;
              }


              // write fast device subevents
              uint32_t sub_ev_fast_number2 = ev_fast2->GetNumSubEvent();
              for(uint32_t index = 0; index < sub_ev_fast_number2; index++){
                  auto sub_ev_fast2 = ev_fast2->GetSubEvent(index);
                  ev_sync->AddSubEvent(sub_ev_fast2);
              }
              // next fast2 sub event


              ev_fast2 = reader_fast2->GetNextEvent();
              trigger_ID_fast2 = ev_fast2->GetTriggerN();


              trigger_count ++;

          }







      }
      else{
          std::cout<< "???" << std::endl;
          break;
      }

      if(writer)
          //writer->WriteEvent(std::move(ev_sync));
          writer->WriteEvent(std::move(ev_sync));

      if(print_ev_in){
          ev_sync->Print(std::cout);
      }

      event_count ++;
      //if(event_count > 10)
      //    break;
  }


  return 0;
}
