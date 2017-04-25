#include "Processor_batch.hh"
#include "Processors.hh"
#include "eudaq/baseFileReader.hh"
#include "eudaq/OptionParser.hh"

using namespace eudaq;
using namespace Processors;
int main(int, char ** argv) {
  eudaq::OptionParser op("EUDAQ File Sender", "1.0", "", 0);
  eudaq::Option<std::string> port_(op, "p", "port", "tcp://44000", "string", "network port");
  try {
    op.Parse(argv);
    Processor_batch batch;
    batch >> dataReciver(port_.Value())
      >> ShowEventNR(1000) >> waitForEORE();
    batch.init(); batch.run(); batch.end();
  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}
