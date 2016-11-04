#include "eudaq/FileReader.hh"
#include "eudaq/FileWriter.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Logger.hh"

#include <iostream>
#include "eudaq/MultiFileReader.hh"

unsigned dbg = 0; 


int main(int, char ** argv) {
	std::clock_t    start;

	start = std::clock();
  eudaq::OptionParser op("EUDAQ File Converter", "1.0", "", 1);
  eudaq::Option<std::string> type(op, "t", "type", "native", "name", "Output file type");
  eudaq::Option<std::string> events(op, "e", "events", "", "numbers", "Event numbers to convert (eg. '1-10,99' default is all)");
  eudaq::Option<std::string> ipat(op, "i", "inpattern", "../data/run$6R.raw", "string", "Input filename pattern");
  eudaq::Option<std::string> opat(op, "o", "outpattern", "test$6R$X", "string", "Output filename pattern");
  eudaq::OptionFlag async(op, "a", "nosync", "Disables Synchronisation with TLU events");
  eudaq::Option<size_t> syncEvents(op, "n" ,"syncevents",1000,"size_t","Number of events that need to be synchronous before they are used");
  eudaq::Option<uint64_t> syncDelay(op, "d" ,"longDelay",20,"uint64_t","us time long time delay");
  eudaq::Option<std::string> level(op, "l", "log-level", "INFO", "level",
      "The minimum level for displaying log messages locally");
  op.ExtraHelpText("Available output types are: " + eudaq::to_string(eudaq::FileWriterFactory::GetTypes(), ", "));
  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL(level.Value());
    std::vector<unsigned> numbers = eudaq::parsenumbers(events.Value());
	std::sort(numbers.begin(),numbers.end());
		eudaq::multiFileReader reader(!async.Value());
    for (size_t i = 0; i < op.NumArgs(); ++i) {
	
      reader.addFileReader(op.GetArg(i), ipat.Value());
	}
      std::shared_ptr<eudaq::FileWriter> writer(eudaq::FileWriterFactory::Create(type.Value()));
      writer->SetFilePattern(opat.Value());
      writer->StartRun(reader.RunNumber());
	  int event_nr=0;
      do {
		  if (!numbers.empty()&&reader.GetDetectorEvent().GetEventNumber()>numbers.back())
		  {
			break;
		  }else if (reader.GetDetectorEvent().IsBORE() || reader.GetDetectorEvent().IsEORE() || numbers.empty() ||
				std::find(numbers.begin(), numbers.end(), reader.GetDetectorEvent().GetEventNumber()) != numbers.end()) {
			  writer->WriteEvent(reader.GetDetectorEvent());
			  if(dbg>0)std::cout<< "writing one more event" << std::endl;
			  ++event_nr;
			  if (event_nr%1000==0)
			  {
				  std::cout<<"Processing event "<< event_nr<<std::endl;
			  }
			}
      } while (reader.NextEvent());
      if(dbg>0)std::cout<< "no more events to read" << std::endl;
    
  } catch (...) {
	    std::cout << "Time: " << (std::clock() - start) / static_cast<double>(CLOCKS_PER_SEC / 1000) << " ms" << std::endl;
    return op.HandleMainException();
  }
    std::cout << "Time: " << (std::clock() - start) / static_cast<double>(CLOCKS_PER_SEC / 1000) << " ms" << std::endl;
  if(dbg>0)std::cout<< "almost done with Converter. exiting" << std::endl;
  return 0;
}
