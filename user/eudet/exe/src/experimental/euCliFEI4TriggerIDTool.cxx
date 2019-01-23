#include "eudaq/OptionParser.hh"
#include "eudaq/DataConverter.hh"
#include "eudaq/FileWriter.hh"
#include "eudaq/FileReader.hh"
#include <iostream>

#include "eudaq/DataCollector.hh"

int main(int /*argc*/, const char **argv) {

  eudaq::OptionParser op("EUDAQ Command Line DataMerger", "2.0", "The Data Merger launcher of EUDAQ");
  eudaq::Option<std::string> file_input(op, "i", "input", "", "string", "input file");

  eudaq::Option<std::string> file_output(op, "o", "output", "", "string", "output file");

  try{
      op.Parse(argv);
  }
  catch (...) {
      return op.HandleMainException();
  }

  std::string infile_path = file_input.Value();
  std::string outfile_path = file_output.Value();

  std::string type_out = outfile_path.substr(outfile_path.find_last_of(".")+1);

  if(infile_path.empty()){
      std::cout<<"option --help to get help"<<std::endl;
      return 1;
  }


  std::string type_in = infile_path.substr(infile_path.find_last_of(".")+1);

  if(type_in=="raw" )
      type_in = "native";

  if(type_out=="raw")
      type_out = "native";

  eudaq::FileReaderUP reader;
  eudaq::FileWriterUP writer;

  reader = eudaq::Factory<eudaq::FileReader>::MakeUnique(eudaq::str2hash(type_in), infile_path);

  if(!type_out.empty())
      writer = eudaq::Factory<eudaq::FileWriter>::MakeUnique(eudaq::str2hash(type_out), outfile_path);

  auto ev = reader->GetNextEvent();
  uint32_t trigger_ID = ev->GetTriggerN();

  //std::cout<< "First trigger ID " << trigger_ID << std::endl;

  bool overflow_reached = false;

  int overflow_count = 0;
  bool flag_param = true;

  while(1){
      if(!ev){
          std::cout<< "empty" << std::endl;
          break;
      }

      // initialise new event
      // auto ev_modified =  eudaq::Event::MakeUnique("MimosaTlu");
      // ev_modified->SetFlagPacket(); // copy from Ex0Tg
      // ev_modified->SetTriggerN(ev->GetTriggerN());
      // ev_modified->SetEventN(ev->GetEventN());



      if (overflow_reached)
      {
        //overflow_reached = true;
        std::cout << "Entered overflow if statement " << overflow_count << " times."<< std::endl;

        uint32_t current_trigger_ID = ev->GetTriggerN();
        uint32_t new_trigger_ID = current_trigger_ID + 32768;


        std::const_pointer_cast<eudaq::Event>(ev)->SetTriggerN(new_trigger_ID, flag_param);

        std::cout << "New triggerID is " << ev->GetTriggerN() << " ." << std::endl;

        //Sub Events
        auto sub_ev = ev->GetSubEvent(0);
        std::const_pointer_cast<eudaq::Event>(sub_ev)->SetTriggerN(new_trigger_ID, flag_param);


        overflow_count++;
      }

      if(trigger_ID == 32767) overflow_reached = true;



      if(writer)
          writer->WriteEvent(std::move(ev));



      ev = reader->GetNextEvent();
      std::cout << "Next event retrieved..." << std::endl;
      trigger_ID = ev->GetTriggerN();

      }






  return 0;
}
