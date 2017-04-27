#include "Processor_batch.hh"
#include "Processors.hh"
#include "eudaq/baseFileReader.hh"
#include "eudaq/OptionParser.hh"
#include<thread>
#include<chrono>


using namespace eudaq;
using namespace Processors;
int main(int, char ** argv) {
  OptionParser op("EUDAQ File Sender", "1.0", "", 1);
  Option<std::string> dest(op, "d", 
    "destination", "tcp://127.0.0.1:44000", "string", "network Destination");
  try {
    op.Parse(argv);
    Processor_batch batch;
    batch >> fileReader(fileConfig(op.GetArg(0)))
	  >> ShowEventNR(1000) >> dataSender(dest.Value());
    batch.init();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    batch.run();
    batch.end();
  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}
