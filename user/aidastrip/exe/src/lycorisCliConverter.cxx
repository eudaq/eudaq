/*
 * mengqing.wu@desy.de @ 2018-03-05
 * based on eudaq euCliConverter.cxx
 * -- for lycoris telescope funded by AIDA2020 located at DESY/
 */
#include "eudaq/OptionParser.hh"
#include "eudaq/DataConverter.hh"
#include "eudaq/FileWriter.hh"
#include "eudaq/FileReader.hh"

#include <iostream>
#include <fstream>
#include <unordered_map>

#include "KpixEvent.h"
//#include "KpixSample.h"
#include "lycorisRootAnalyzer.h"

int main(int /*argc*/, const char **argv) {
  eudaq::OptionParser op("EUDAQ Command Line DataConverter", "2.0", "The Data Converter launcher of EUDAQ");
  eudaq::Option<std::string> file_input(op, "i", "input", "", "string",
					"input file");
  eudaq::Option<std::string> file_output(op, "o", "output", "", "string",
					 "output file: .slcio, .root");
  eudaq::OptionFlag iprint(op, "ip", "iprint", "enable print of input Event");

  try{
    op.Parse(argv);
  }
  catch (...) {
    return op.HandleMainException();
  }
  
  std::string infile_path = file_input.Value();
  if(infile_path.empty()){
    std::cout<<"option --help to get help"<<std::endl;
    return 1;
  }
  
  std::string outfile_path = file_output.Value();
  std::string type_in = infile_path.substr(infile_path.find_last_of(".")+1);
  std::string type_out = outfile_path.substr(outfile_path.find_last_of(".")+1);
  bool print_ev_in = iprint.Value();

  std::cout<<"outfile_path = "<<outfile_path<<"\n"
	   <<"type_in = "<< type_in <<"\n"
    	   <<"type_out = "<< type_out <<"\n"
	   <<std::endl;
  
  if(type_in=="raw")
    type_in = "native";
  if(type_out=="raw")
    type_out = "native";
  
  eudaq::FileReaderUP reader;
  eudaq::FileWriterUP writer;
  lycorisRootAnalyzer *RootAna;
  stringstream           tmp;
    
  reader = eudaq::Factory<eudaq::FileReader>::MakeUnique(eudaq::str2hash(type_in), infile_path);

  if(!type_out.empty()){
    if (type_out=="root")
      RootAna = new lycorisRootAnalyzer(outfile_path);
    else  
      writer = eudaq::Factory<eudaq::FileWriter>::MakeUnique(eudaq::str2hash(type_out), outfile_path);
  }
  
  int evtCounting = 0;
  KpixEvent    CycleEvent;
  //KpixSample   *sample;
  
  std::vector<bool> kpixfound(32, false);
    
  uint cycleEventN, timestamp, eudaqEventN;
  uint kpix, channel, bucket, value, tstamp;
  
  while(1){
    // loop over kpix cycle event
    auto ev = reader->GetNextEvent(); //--> when ev is empty, an error catched from throw;
    if(!ev) {
      /* NAIVE protection to check if any evt existed after the empty one TAT; */
      std::cout << " -- Empty event, check next one, if empty finish reading." << std::endl;
      auto ev2 = reader->GetNextEvent();
      if (!ev2) break;
      else ev = ev2;
    }
    
    if(print_ev_in)
      ev->Print(std::cout);

    if(writer)
      writer->WriteEvent(ev);

    if(RootAna){
      auto block_n_list = ev->GetBlockNumList();
      // std::cout<< "[dev] nblocks = "<< ev->NumBlocks() << std::endl;
      for (auto &block_n: block_n_list){
	std::vector<uint8_t> block = ev -> GetBlock(block_n);
	if (block.size()==0) EUDAQ_THROW("Empty data!");
	else{
	  
	  size_t size_of_kpix = block.size()/sizeof(uint32_t);
	 
	  uint32_t *kpixEvt = nullptr;
	  if (size_of_kpix)
	    kpixEvt = reinterpret_cast<uint32_t *>(block.data());
	  
	  /* read kpix data */
	  CycleEvent.copy(kpixEvt, size_of_kpix);
	  cycleEventN = CycleEvent.eventNumber();
	  timestamp = CycleEvent.timestamp();
	  eudaqEventN = ev->GetEventN();
 
	  if (eudaqEventN==0){
	    std::cout<<"[dev] # of block.size()/sizeof(uint32_t) = " << size_of_kpix << "\n"
		     <<"\t sizeof(uint32_t) = " << sizeof(uint32_t) << std::endl;
	    
	    std::cout << "\t Uint32_t  = " << kpixEvt << "\n"
		      << "\t evtNum    = " << kpixEvt[0] << std::endl;
	    
	    std::cout << "\t kpixEvent.evtNum = " << CycleEvent.eventNumber() <<std::endl;
	    std::cout << "\t kpixEvent.timestamp = "<< CycleEvent.timestamp() <<std::endl;
	    std::cout << "\t kpixEvent.count = "<< CycleEvent.count() <<std::endl;
	  }

	  RootAna->Analyzing(CycleEvent, eudaqEventN);

	}
      }
      
    }

    evtCounting++;
  }

  std::cout<<"[Converter:info] # of Evt counting ==> "
	   << evtCounting <<"\n";

  RootAna->Closing();
  
  return 0;
}
