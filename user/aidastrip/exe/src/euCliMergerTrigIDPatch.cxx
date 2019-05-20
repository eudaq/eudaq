#include "eudaq/OptionParser.hh"
#include "eudaq/DataConverter.hh"
#include "eudaq/FileWriter.hh"
#include "eudaq/FileReader.hh"
#include <iostream>

#include "eudaq/DataCollector.hh"
/*
 * author: Mengqing Wu <mengqing.wu@desy.de>
 * date: 2019-05-17
 * Aim: - short cable from TLU to Mimosa cause the loosing last bit of LSB trigger ID.
 *      - rematch strategy: Mimosa Event_ID with TLU Trigger_ID.
 */

uint32_t GetSubTriggerN(std::shared_ptr<const eudaq::Event> event, std::string desc);
std::shared_ptr<const eudaq::Event> GetDescSubEvent(std::shared_ptr<const eudaq::Event> event, std::string desc);

int main(int /*argc*/, const char **argv) {

  eudaq::OptionParser op("EUDAQ Command Line DataMerger (Patch)", "2.0", 
          "Event building by Trigger ID; Input data from TriggerID synced TLU + Mimosa events; Patch to fix double Trigger ID assigned to two Mimosa Event");
  eudaq::Option<std::string> file_input_async(op, "i", "input", "", "string",
          "input file");

  eudaq::Option<std::string> file_output(op, "o", "output", "", "string",
          "output file");
  eudaq::OptionFlag iprint(op, "ip", "iprint", "enable print of input Event");

  try{
      op.Parse(argv);
  }
  catch (...) {
      return op.HandleMainException();
  }

  std::string infile_path_async = file_input_async.Value();
  //std::string infile_path_ni = file_input_ni.Value();
  //std::string infile_path_dut = file_input_dut.Value();
  std::string outfile_path = file_output.Value();
  if (outfile_path.empty()) outfile_path = "out.raw";
	  
  if(infile_path_async.empty() ){
      std::cout<<"option --help to get help"<<std::endl;
      return 1;
  }


  std::string type_in_async = infile_path_async.substr(infile_path_async.find_last_of(".")+1);
  std::string type_out = outfile_path.substr(outfile_path.find_last_of(".")+1);
  bool print_ev_in = iprint.Value();

  if(type_in_async =="raw" ){
      type_in_async = "native";
      puts("input is raw.\n");
  }
  if(type_out=="raw")
      type_out = "native";


  eudaq::FileReaderUP reader_ni;
  eudaq::FileReaderUP reader_tlu;
  eudaq::FileWriterUP writer;


  reader_ni = eudaq::Factory<eudaq::FileReader>::MakeUnique(eudaq::str2hash(type_in_async), infile_path_async);
  reader_tlu = eudaq::Factory<eudaq::FileReader>::MakeUnique(eudaq::str2hash(type_in_async), infile_path_async);
  if(!type_out.empty())
      writer = eudaq::Factory<eudaq::FileWriter>::MakeUnique(eudaq::str2hash(type_out), outfile_path);

  uint32_t ni_count = 0;
  uint32_t tlu_count = 0;

  uint32_t sync_count = 0;
  uint32_t sync_event_number = 0;
  //  uint32_t sync_trigger_number = 0;
  //  const uint32_t run_number = ev_tlu->GetRunN();

  auto ev_ni = reader_ni->GetNextEvent();
  auto ev_tlu = reader_tlu->GetNextEvent();

  // Global event info
  const uint32_t run_number = ev_ni->GetRunN();
  eudaq::Event *ev_tlu_sub=nullptr;

  //  bool trig_missing = false; --> ToDo
  
  while(1){
	 
	  auto ev_ni_sub = GetDescSubEvent(ev_ni, "NiRawDataEvent");
	  sync_event_number = ev_ni_sub->GetEventN();
	  
	  if (!ev_ni_sub){
		  std::cerr<< "Ni: Empty Sub events!" << std::endl;
		  continue;
	  }
		  
	  while (sync_event_number > GetSubTriggerN(ev_tlu, "TluRawDataEvent")){
		  ev_tlu = reader_tlu->GetNextEvent();
		  if (!ev_tlu){
			  std::cout<< "No more Tlu events..." << std::endl;
			  break;
		  }
	  }
	  
	  if (!ev_tlu)
		  std::cout<< "No more Tlu events..." << std::endl;
	  
	  // debug
	  if (ev_ni_sub->GetEventN() != sync_event_number)
		  std::cerr<< "bug-event N" << std::endl;
	  if (ev_tlu_sub->GetTriggerN() != sync_event_number)
		  std::cerr<< "bug-trigger N" << std::endl;
	  
	  // initialise new event
	  auto ev_sync =  eudaq::Event::MakeUnique("MimosaTlu");
	  ev_sync->SetFlagPacket(); // copy from Ex0Tg
	  ev_sync->SetTriggerN(ev_tlu_sub->GetTriggerN());
	  ev_sync->SetEventN(sync_event_number);
	  ev_sync->SetRunN(run_number);
	  std::cout << "Sync Event created..." << std::endl;
	  
	  
	  
	  
	  ev_ni = reader_ni->GetNextEvent(); 
	  if(!ev_ni){
		  std::cout << "No more Ni events..." << std::endl;
		  break;
	  }
	  
  }// end of while loop

  std::cout << "[info] How many Mimosa events : "<< ni_count << std::endl;
  std::cout << "[info] How many TLU events    : "<< tlu_count << std::endl;
            
  return 0;
}


uint32_t GetSubTriggerN(std::shared_ptr<const eudaq::Event> event, std::string desc){
	/*
	 * Only applicable for non duplicated sub event structure!
	 */
	uint32_t triggern;
	
	for ( auto&sub : event->GetSubEvents() ){
		if ( sub->GetDescription() == desc ){
			triggern = sub->GetTriggerN();
		}
	}
	
	return triggern;
}
std::shared_ptr<const eudaq::Event> GetDescSubEvent(std::shared_ptr<const eudaq::Event> event, std::string desc){
	/*
	 * Only applicable for non duplicated sub event structure!
	 */
	std::shared_ptr<const eudaq::Event> res;
	if (event->GetNumSubEvent()==0)
		puts("[warning] Empty subEvents array.");
	else{
		for ( auto&res: event->GetSubEvents() ){
			if ( res-> GetDescription() == desc)
				return res;
		}
	}
	
	return nullptr;
}
