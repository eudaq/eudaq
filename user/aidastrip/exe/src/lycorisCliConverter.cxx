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

#include "TFile.h"

int main(int /*argc*/, const char **argv) {
  eudaq::OptionParser op("EUDAQ Command Line DataConverter", "2.0", "The Data Converter launcher of EUDAQ");
  eudaq::Option<std::string> file_input(op, "i", "input", "", "string",
					"input file");
  eudaq::Option<std::string> file_output(op, "o", "output", "", "string",
					 "output file: .slcio, .csv");
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
  TFile* rfile;
    
  reader = eudaq::Factory<eudaq::FileReader>::MakeUnique(eudaq::str2hash(type_in), infile_path);

  if(!type_out.empty()){
    if (type_out=="root"){
      rfile = new TFile(outfile_path.c_str(), "recreate");
      std::cout << " Write to " << outfile_path << std::endl;
    }
    else  
      writer = eudaq::Factory<eudaq::FileWriter>::MakeUnique(eudaq::str2hash(type_out), outfile_path);
  }
  
  std::vector<bool> foundkpix(32, false);
  int evtCounting = 0;
  while(1){
    /* 
     * ATTENTION! once empty evt in the middle, no evt to read after the EMPTY one!
     */

    auto ev = reader->GetNextEvent(); //--> when ev is empty, an error catched from throw;


    
    if(!ev) {
      /* 
       * NAIVE protection to check if any evt existed after the empty one TAT;
       */
      std::cout << " -- Empty event, check next one, if empty finish reading." << std::endl;
      auto ev2 = reader->GetNextEvent();
      if (!ev2) break;
      else ev = ev2;
    }
    
    if(print_ev_in)
      ev->Print(std::cout);

    if(writer)
      writer->WriteEvent(ev);

    if(rfile){
      auto block_n_list = ev->GetBlockNumList();
      std::cout<< "[dev] nblocks = "<< ev->NumBlocks() << std::endl;
      for (auto &block_n: block_n_list){
	std::vector<uint8_t> hit = ev -> GetBlock(block_n);
	foundkpix.at(block_n)=true;
      }
      
    }
    
    evtCounting++;    
  }
  std::cout<<"[num of kpix]\t";
  for (uint ff=0; ff<32; ff++){
    if (foundkpix[ff])
      std::cout<<ff<<", ";
    
  }
  std::cout<<"\n";
  
  if (rfile)
    rfile->Close();
  
  std::cout<<"[Converter:info] # of Evt counting ==> "
	   << evtCounting <<"\n";
  return 0;
}
