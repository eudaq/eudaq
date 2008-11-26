#include "eudaq/FileReader.hh"
#include "eudaq/FileWriter.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/counted_ptr.hh"
#include "eudaq/Utils.hh"

#include <iostream>

using namespace eudaq;

int main(int, char ** argv) {
  eudaq::OptionParser op("EUDAQ File Converter", "1.0", "", 1);
  eudaq::Option<std::string> type(op, "t", "type", "native", "name", "Output file type");
  eudaq::Option<std::string> ipat(op, "i", "inpattern", "../data/run$6R.raw", "string", "Input filename pattern");
  eudaq::Option<std::string> opat(op, "o", "outpattern", "test$6R$X", "string", "Output filename pattern");
  try {
    op.Parse(argv);
    for (size_t i = 0; i < op.NumArgs(); ++i) {
      eudaq::FileReader reader(op.GetArg(i), ipat.Value());
      counted_ptr<eudaq::FileWriter> writer(FileWriterFactory::Create(type.Value()));
      writer->SetFilePattern(opat.Value());
      writer->StartRun(reader.RunNumber());
      do {
        writer->WriteEvent(reader.Event());
      } while (reader.NextEvent());
    }
  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}
