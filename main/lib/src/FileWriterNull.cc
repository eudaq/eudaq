#include "eudaq/FileWriter.hh"

namespace eudaq {

  class FileWriterNull : public FileWriter {
    public:
      FileWriterNull(const std::string &) {}
      virtual void StartRun(unsigned) {}
      virtual void WriteEvent(const DetectorEvent &) {}
      virtual unsigned long long FileBytes() const { return 0; }
  };

  namespace {
    RegisterFileWriter<FileWriterNull> reg("null");
  }

}
