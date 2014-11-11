#include "eudaq/FileReader.hh"
#include "eudaq/FileWriter.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Logger.hh"

#include <iostream>
#include "eudaq/MultiFileReader.hh"
#include "eudaq/readAndProcessDataTemplate.h"
#include "multiResender.h"


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
  eudaq::OptionFlag async(op, "a", "nosync", "Disables Synchronization with TLU events");
  eudaq::Option<std::string> level(op, "l", "log-level", "INFO", "level",
    "The minimum level for displaying log messages locally");
  op.ExtraHelpText("Available output types are: " + to_string(eudaq::FileWriterFactory::GetTypes(), ", "));
  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL(level.Value());

    ReadAndProcess<eudaq::multiResender> readProcess(!async.Value());
    readProcess.setEventsOfInterest(parsenumbers(events.Value()));
    for (size_t i = 0; i < op.NumArgs(); ++i) {

      readProcess.addFileReader(op.GetArg(i), ipat.Value());

    }
    readProcess.setWriter(new multiResender());


    readProcess.SetParameter(TAGNAME_RUNCONTROL, "127.0.0.1:44000");
    readProcess.StartRun();

    readProcess.process();
    readProcess.EndRun();

  }
  catch (...) {
    std::cout << "Time: " << (std::clock() - start) / (double)(CLOCKS_PER_SEC / 1000) << " ms" << std::endl;
    return op.HandleMainException();
  }
  std::cout << "Time: " << (std::clock() - start) / (double)(CLOCKS_PER_SEC / 1000) << " ms" << std::endl;

  return 0;
}
