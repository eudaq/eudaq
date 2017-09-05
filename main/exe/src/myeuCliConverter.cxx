#include "eudaq/OptionParser.hh"
#include "eudaq/DataConverter.hh"
#include "eudaq/FileWriter.hh"
#include "eudaq/FileReader.hh"
#include <iostream>
#include <fstream>

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
  std::ofstream writerCSV;
  std::vector<std::string> tag_vec = {"timer","ch0","ch10","ch20","ch30"};
  std::string csv_str;
  for (auto const& s: tag_vec) {
    csv_str+=s+',';
  }
  std::cout<< csv_str <<"\n";
    
  reader = eudaq::Factory<eudaq::FileReader>::MakeUnique(eudaq::str2hash(type_in), infile_path);

  if(!type_out.empty()){
    if (type_out=="csv"){
      writerCSV.open (outfile_path);
      /* first line for excel read csv file directly: sep=<delimeter> */
      writerCSV << "sep=,\n";
      /* column title line: */
      writerCSV << csv_str <<"\n";
    }
    else  
      writer = eudaq::Factory<eudaq::FileWriter>::MakeUnique(eudaq::str2hash(type_out), outfile_path);
  }
  

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
      auto ev2 = reader->GetNextEvent();
      if (!ev2) break;
      else ev = ev2;
    }
    
    if(print_ev_in)
      ev->Print(std::cout);

    if(writer)
      writer->WriteEvent(ev);

    if(writerCSV.is_open()){
      std::string csv_line;
      for (auto const& tag: tag_vec) csv_line+=ev->GetTag(tag)+',';
      writerCSV << csv_line <<"\n";
    }
    
    evtCounting++;    
  }

  if (writerCSV.is_open())
    writerCSV.close();
  
  std::cout<<"[Converter:info] # of Evt counting ==> "
	   << evtCounting <<"\n";
  return 0;
}
