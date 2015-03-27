#ifdef WIN32
#include <Windows4Root.h>
#endif

#include "eudaq/FileReader.hh"
#include "eudaq/FileWriter.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Logger.hh"

#include <iostream>
#include "../inc/makeCorrelations.h"
#include "eudaq/PluginManager.hh"
#include "eudaq/MultiFileReader.hh"
#include "eudaq/readAndProcessDataTemplate.h"

using namespace eudaq;
unsigned dbg = 0; 


int main(int, char ** argv) {
	std::clock_t    start;

	start = std::clock();
  eudaq::OptionParser op("EUDAQ File Converter", "1.0", "", 1);

  auto events = ReadAndProcess<eudaq::FileWriter>::add_Command_line_option_EventsOfInterest(op);


  FileWriterFactory::addComandLineOptions(op);
  op.ExtraHelpText(FileWriterFactory::Help_text());


  FileReaderFactory::addComandLineOptions(op);

  op.ExtraHelpText(FileReaderFactory::Help_text());

  

  EventSyncFactory::addComandLineOptions(op);
  eudaq::Option<std::string> confFile(op, "c", "confFile", "../conf/offlinemonconf.xml", "string", "load the file that contains all the information about the correlations plots");
  eudaq::Option<std::string> level(op, "l", "log-level", "INFO", "level",
                                   "The minimum level for displaying log messages locally");

  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL(level.Value());
    ReadAndProcess<mCorrelations> readProcess;
  readProcess.setEventsOfInterest(parsenumbers(events->Value()));

  readProcess.addFileReader(FileReaderFactory::create(op));
  readProcess.setWriter(std::unique_ptr<  mCorrelations>(new   mCorrelations()));
  readProcess.SetParameter("conf", confFile.Value());
  readProcess.StartRun();
  readProcess.process();
  readProcess.EndRun();



     
    
  } catch (...) {
	    std::cout << "Time: " << (std::clock() - start) / (double)(CLOCKS_PER_SEC / 1000) << " ms" << std::endl;
    return op.HandleMainException();
  }
    std::cout << "Time: " << (std::clock() - start) / (double)(CLOCKS_PER_SEC / 1000) << " ms" << std::endl;

  return 0;
}
