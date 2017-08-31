#include "eudaq/OptionParser.hh"
#include "eudaq/DataConverter.hh"
#include "eudaq/FileWriter.hh"
#include "eudaq/FileReader.hh"
#include <iostream>

int main(int /*argc*/, const char **argv) {
  eudaq::OptionParser op("EUDAQ Command Line DataConverter", "2.0", "The Data Converter launcher of EUDAQ");
  eudaq::Option<std::string> file_input(op, "i", "input", "", "string",
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
  
  std::cout<<"I am a debugger --1\n";
  reader = eudaq::Factory<eudaq::FileReader>::MakeUnique(eudaq::str2hash(type_in), infile_path);
  std::cout<<"I am a debugger --2\n";
  
  if(!type_out.empty()){

    std::cout<<"[output file empty] @0@!"<<std::endl;
    writer = eudaq::Factory<eudaq::FileWriter>::MakeUnique(eudaq::str2hash(type_out), outfile_path);
    
  }
  
  while(1){

    std::cout<<"I am a debugger --3: try evt \n";

    auto ev = reader->GetNextEvent();
    if(!ev){
      std::cout<<"I am a debugger --3: empty ev \n";
 
      break;
    }
    if(print_ev_in)
      ev->Print(std::cout);

    if(writer){
      std::cout<<"I am a debugger --3\n";
      writer->WriteEvent(ev);
    }
  }
  std::cout<<"I arrives the end! \n";

  return 0;
}
