#include "Platform.hh"
#include "OptionParser.hh"
#include "Utils.hh"
#include "Logger.hh"
#include "RawDataEvent.hh"
#include "baseFileReader.hh"
#include "FileWriter.hh"
#include "EventSynchronisationDetectorEvents.hh"
#include "Processor_batch.hh"
#include "Processors.hh"
#include "Processor_inspector.hh"

#include <iostream>

using namespace eudaq;
using namespace Processors;
unsigned dbg = 0;
std::unique_ptr<eudaq::Option<std::string>>  add_Command_line_option_EventsOfInterest(eudaq::OptionParser & op) {

  return std::unique_ptr<eudaq::Option<std::string>>(new eudaq::Option<std::string>(op, "e", "events", "", "numbers", "Event numbers to process (eg. '1-10,99' default is all)"));


}

void add_merger(Processor_batch& batch, eudaq::OptionParser& op) {
  if (op.NumArgs() > 1 || EventSyncFactory::DefaultIsSet())
  {
    batch >> Processors::merger(EventSyncFactory::getDefaultSync());
  }
}
void add_files(Processor_batch& batch, eudaq::OptionParser& op) {
  for (size_t i = 0; i < op.NumArgs(); ++i)
  {
    batch >> Processors::fileReader(fileConfig(op.GetArg(i)));

  }
}
int main(int, char ** argv) {



  eudaq::OptionParser op("EUDAQ File Converter", "1.0", "", 1);

  auto events = add_Command_line_option_EventsOfInterest(op);


  FileWriterFactory::addComandLineOptions(op);
  op.ExtraHelpText(FileWriterFactory::Help_text());


  FileReaderFactory::addComandLineOptions(op);

  op.ExtraHelpText(FileReaderFactory::Help_text());

  EventSyncFactory::addComandLineOptions(op);

  eudaq::Option<std::string> level(op, "l", "log-level", "INFO", "level",
    "The minimum level for displaying log messages locally");

  std::clock_t    start = std::clock();
  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL(level.Value());

    Processor_batch batch;


    add_files(batch, op);
    add_merger(batch, op);
    batch >> Processors::ShowEventNR(1000)
      >> Processors::eventSelector(parsenumbers(events->Value()))
      >> Processors::fileWriter();

    batch.init();
    batch.run();
    batch.end();

  }
  catch (...) {
    std::cout << "Time: " << (std::clock() - start) / (double)(CLOCKS_PER_SEC / 1000) << " ms" << std::endl;
    return op.HandleMainException();
  }

  std::cout << "Time: " << (std::clock() - start) / (double)(CLOCKS_PER_SEC / 1000) << " ms" << std::endl;
  if (dbg > 0)std::cout << "almost done with Converter. exiting" << std::endl;
  return 0;
}
