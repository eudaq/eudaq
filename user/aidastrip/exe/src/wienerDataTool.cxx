/*
 * mengqing.wu@desy.de
 * Created @ Mar 2nd, 2020
 * based on eudaq euCliConverter.cxx
 * Target: plot out time based wiener values
 */
#include "eudaq/OptionParser.hh"
#include "eudaq/DataConverter.hh"
//#include "eudaq/FileWriter.hh"
#include "eudaq/FileReader.hh"
#include <iostream>
#include <fstream>
#include "TTree.h"

class rawtocsv{
public:
	rawtocsv(){
	};
	~rawtocsv(){
		if (m_writer.is_open())
			m_writer.close();
	};

	void open(std::string fout){
		std::cout << "[info] write raw into csv file." << std::endl;
		m_writer.open(fout);
		/* first line for excel read csv file directly: sep=<delimeter> */
		m_writer << "sep=,\n";
	}
	void close(){
		m_writer.close();
	}
	void addLineFirst(std::map<std::string, std::string> tags){
		/* add only column title line */
		std::string csv_str;
		for (auto const& it: tags){
			m_v_cols.push_back(it.first);
		}
		for (auto const& s: m_v_cols) csv_str+=s+',';
		std::cout << "Your .csv has following collumns:\n"
		          << csv_str <<"\n";
		m_writer << csv_str << "\n";
		
	};
	void addLine(std::map<std::string, std::string> tags){
		/*
		 * tight to the old fashion: doors open to choose tags to print
		 */
		std::string csv_line;
		for (auto const& tag: m_v_cols)
			csv_line += tags.at(tag)+',';
		m_writer << csv_line << "\n";
	};

private:
	std::ofstream m_writer;
	std::vector<std::string> m_v_cols;

};

class rawtoroot{
public:
	rawtoroot(){};
	~rawtoroot(){};
	void open(std::string fout){}
	void close(){}
	void createTree(){}
	void addEvent(){}
private:
	TTree* m_tree;
};

int main(int /*argc*/, const char **argv) {
	eudaq::OptionParser op("EUDAQ Command Line DataConverter for WienerProducer", "2.0",
	                       "The Data Converter launcher of EUDAQ");
	eudaq::Option<std::string> file_input(op, "i", "input", "", "string",
	                                      "input file");
	eudaq::Option<std::string> file_output(op, "o", "output", "", "string",
	                                       "output file: .csv, .root");
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
  //  eudaq::FileWriterUP writer;
  std::ofstream writerCSV;
  std::string csv_str;
  rawtocsv tocsv;
  
  reader = eudaq::Factory<eudaq::FileReader>::MakeUnique(eudaq::str2hash(type_in), infile_path);

  if(!type_out.empty()){
    if (type_out=="csv"){
      writerCSV.open (outfile_path);
      /* first line for excel read csv file directly: sep=<delimeter> */
      writerCSV << "sep=,\n";
      tocsv.open(outfile_path);
    }
    
    else
	    return(1);
	    
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


    auto tag_map = ev->GetTags();
    /*
     * Register all tag-keys from the 1st event you read:
     */
    if (type_out == "csv"){
	    if (evtCounting==0 ){
		    /* add col title if first */
		    tocsv.addLineFirst(tag_map);
	    }
	    /*add line value for all evts */
	    tocsv.addLine(tag_map);
    }
    evtCounting++;    
  }

  
  std::cout<<"[Converter:info] # of Evt counting ==> "
	   << evtCounting <<"\n";
  return 0;
}



