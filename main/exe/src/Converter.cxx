#include "eudaq/FileWriter.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Logger.hh"

#include "eudaq/MultiFileReader.hh"
#include "eudaq/readAndProcessDataTemplate.h"
#include <iostream>
#include "eudaq/EventSynchronisationDetectorEvents.hh"

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

    eudaq::Option<std::string> level(op, "l", "log-level", "INFO", "level",
                                   "The minimum level for displaying log messages locally");
  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL(level.Value());

    ReadAndProcess<eudaq::FileWriter> readProcess;
    readProcess.setEventsOfInterest(parsenumbers(events->Value()));
    
    readProcess.addFileReader(FileReaderFactory::create(op));
    readProcess.setWriter(FileWriterFactory::Create());

    readProcess.StartRun();
    readProcess.process();
    readProcess.EndRun();

  }
  catch (...) {
    std::cout << "Time: " << (std::clock() - start) / (double) (CLOCKS_PER_SEC / 1000) << " ms" << std::endl;
    return op.HandleMainException();
  }
  std::cout << "Time: " << (std::clock() - start) / (double) (CLOCKS_PER_SEC / 1000) << " ms" << std::endl;
  if (dbg>0)std::cout << "almost done with Converter. exiting" << std::endl;
  return 0;
}
