/*
 * Author: Paul Schuetze
 *
 * Purpose: Splitting .raw files from eudaq into several files
 *
 *
 *
 */


#include "eudaq/FileReader.hh"
#include "eudaq/FileWriter.hh"
#include "eudaq/OptionParser.hh"

#include <iostream>
#include <string>
#include <fstream>

using namespace std;
using namespace eudaq;

void usage(){

  cout << "EURunsplitter run options: " << endl << endl;
  cout << "* -d (--directory) string\t\tDirectory containing raw data files" << endl;
  cout << "* -o (--oldrun) int\t\tRunnumber of run to be splitted" << endl;
  cout << "* -n (--newrun) int\t\tFirst runnumber for the created runs" << endl;
  cout << "  -e (--runsize) int\t\tNumber of events per new run" << endl;
  //  cout << "  -h (--help)   \t\tPrint this message" << endl;
  cout << "\n*: Mandatory.\n\n" << endl;
  exit(0);

}



int main( int argc, char ** argv ){
  
  
  OptionParser op("EUDAQ File Converter", "1.0", "", 0, 4);
  Option<std::string> pDir(op, "d", "directory", "rawdata/", "string", "Input data directory");
  Option<std::string> pOldRun(op, "o", "oldrun", "1", "size_t", "Run Number of run to split");
  Option<size_t> pNewRunStart(op, "n", "newrun", 0, "size_t", "Run Number of first output run");
  Option<size_t> pRunSize(op, "e", "runsize", 500000, "size_t", "Number of events per run");

  string origRunnr = "";
  string folder = "";
  unsigned firstNewRunnr;
  int evtsPerFile;

  try{
    op.Parse(argv);
    
    if(!pDir.IsSet() || !pOldRun.IsSet() || !pNewRunStart.IsSet())
      usage();
    
    folder = pDir.Value();
    origRunnr = pOldRun.Value();
    firstNewRunnr = pNewRunStart.Value();
    evtsPerFile = pRunSize.Value();
  }catch(...){
    cout << "Config parsing failed" << endl;
    exit(0);
  }

  // Init reader
  
  char buffer[100];  

  if(folder.at(folder.size()-1) == '/'){
    sprintf(buffer, "%srun$6R$X", folder.c_str());
  }else{
    sprintf(buffer, "%s/run$6R$X", folder.c_str());
  }
  
  string filename(buffer);
  
  FileReader * reader;
  reader = new FileReader(origRunnr, filename);
  

  // Opening logfile - always append logs
  ofstream log;
  log.open("split.log", std::ofstream::out | std::ofstream::app);
  
  
  // Init writer
  
  unsigned currentRunnr = firstNewRunnr;

  std::shared_ptr<FileWriter> writer;
  writer = std::shared_ptr<eudaq::FileWriter>(FileWriterFactory::Create(""));
  writer->SetFilePattern(filename);

  // Opening first new file
  
  writer->StartRun(firstNewRunnr);


  // Start Processing

  DetectorEvent boreevent = reader->GetDetectorEvent();

  if(!boreevent.IsBORE()){
    cout << "First event is not BORE. Damn it!" << endl;
    exit(1);
  }

  log << origRunnr << "\t" << firstNewRunnr;

  writer->WriteEvent(boreevent);
  
  DetectorEvent tempevent = boreevent;
  if(!reader->NextEvent()){
    cout << "No second event found. Exit." << endl;
    exit(1);
  };
  
  unsigned evtnr = 1;

  do{
    
    if(!(evtnr%evtsPerFile)){
      // Start new file
      currentRunnr++;
      
      cout << "\nStarting new file with Run Nr.: " << currentRunnr << endl << endl;
      
      writer->StartRun(currentRunnr);
      
      log << "\t" << currentRunnr;
      
      // Write old BOREevent first
      writer->WriteEvent(boreevent);
    }
    
    tempevent = reader->GetDetectorEvent();
    writer->WriteEvent(tempevent);
    
    if(!(evtnr%5000))cout << "Writing out event Nr. " << evtnr << " to Runnr. " << currentRunnr << endl;

    evtnr++;

  }while(reader->NextEvent());

  log << endl;
  log.close();

  return 0;

}
