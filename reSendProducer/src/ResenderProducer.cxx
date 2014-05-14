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
	eudaq::OptionFlag sync(op, "s", "synctlu", "Resynchronize subevents based on TLU event number");
	eudaq::Option<size_t> syncEvents(op, "n" ,"syncevents",1000,"size_t","Number of events that need to be synchronous before they are used");
	eudaq::Option<unsigned long long> syncDelay(op, "d" ,"longDelay",20,"unsigned long long","us time long time delay");
	eudaq::Option<std::string> level(op, "l", "log-level", "INFO", "level",
		"The minimum level for displaying log messages locally");
	op.ExtraHelpText("Available output types are: " + to_string(eudaq::FileWriterFactory::GetTypes(), ", "));
	try {
		op.Parse(argv);
		EUDAQ_LOG_LEVEL(level.Value());

		ReadAndProcess<eudaq::multiResender> readProcess;
		readProcess.setEventsOfInterest(parsenumbers(events.Value()));
		for (size_t i = 0; i < op.NumArgs(); ++i) {

			readProcess.addFileReader(op.GetArg(i), ipat.Value());

		}
		readProcess.setWriter(new multiResender());

	
		readProcess.SetParameter(TAGNAME_RUNCONTROL,"127.0.0.1:44000");
		readProcess.StartRun();
		bool running=true;
		bool help = true;
// 		    do {
// 				if (help) {
// 					help = false;
// 				std::cout << "--- Commands ---\n"
// 					<< "s data Send StringEvent with 'data' as payload\n"
// 					<< "r size Send RawDataEvent with size bytes of random data (default=1k)\n"
// 					<< "l msg  Send log message\n"
// 					<< "o msg  Set status=OK\n"
// 					<< "w msg  Set status=WARN\n"
// 					<< "e msg  Set status=ERROR\n"
// 					<< "q      Quit\n"
// 					<< "?      \n"
// 					<< "----------------" << std::endl;
// 				}
// 
// 				
// 				running=readProcess.processNextEvent();
// 
// 			}while(running);
		readProcess.process();
		readProcess.EndRun();

	} catch (...) {
		std::cout << "Time: " << (std::clock() - start) / (double)(CLOCKS_PER_SEC / 1000) << " ms" << std::endl;
		return op.HandleMainException();
	}
	std::cout << "Time: " << (std::clock() - start) / (double)(CLOCKS_PER_SEC / 1000) << " ms" << std::endl;
	
	return 0;
}
