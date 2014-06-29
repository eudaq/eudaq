#include "eudaq/FileReader.hh"
#include "eudaq/FileWriter.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Logger.hh"

#include <iostream>
#include "eudaq/MultiFileReader.hh"
#include "eudaq/readAndProcessDataTemplate.h"

using namespace eudaq;
unsigned dbg = 0; 


int main(int, char ** argv) {
	std::clock_t    start;

	start = std::clock();
  eudaq::OptionParser op("EUDAQ File Converter", "1.0", "", 1);
  eudaq::Option<std::string> type(op, "t", "type", "native", "name", "Output file type");
  eudaq::Option<std::string> events(op, "e", "events", "", "numbers", "Event numbers to convert (eg. '1-10,99' default is all)");
  eudaq::Option<std::string> ipat(op, "i", "inpattern", "../data/run$6R.raw", "string", "Input filename pattern");
  eudaq::Option<std::string> opat(op, "o", "outpattern", "test$6R$X", "string", "Output filename pattern");
  eudaq::OptionFlag sync(op, "s", "synctlu", "Resynchronize subevents based on TLU event number");
  eudaq::Option<size_t> syncEvents(op, "n" ,"syncevents",1000,"size_t","Number of events that need to be synchronous before they are used");
  eudaq::Option<uint64_t> syncDelay(op, "d" ,"longDelay",20,"uint64_t","us time long time delay");
  eudaq::Option<std::string> level(op, "l", "log-level", "INFO", "level",
      "The minimum level for displaying log messages locally");
  op.ExtraHelpText("Available output types are: " + to_string(eudaq::FileWriterFactory::GetTypes(), ", "));
  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL(level.Value());

		 ReadAndProcess<eudaq::FileWriter> readProcess;
		 readProcess.setEventsOfInterest(parsenumbers(events.Value()));
    for (size_t i = 0; i < op.NumArgs(); ++i) {
	
		readProcess.addFileReader(op.GetArg(i), ipat.Value());

	}
	readProcess.setWriter(FileWriterFactory::Create(type.Value()));
  
	readProcess.SetParameter(TAGNAME_OUTPUTPATTER,opat.Value());

	  readProcess.StartRun();
		readProcess.process();
      readProcess.EndRun();
    
  } catch (...) {
	    std::cout << "Time: " << (std::clock() - start) / (double)(CLOCKS_PER_SEC / 1000) << " ms" << std::endl;
    return op.HandleMainException();
  }
    std::cout << "Time: " << (std::clock() - start) / (double)(CLOCKS_PER_SEC / 1000) << " ms" << std::endl;
  if(dbg>0)std::cout<< "almost done with Converter. exiting" << std::endl;
  return 0;
}
