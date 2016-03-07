#include "eudaq/FileWriter.hh"
#include "eudaq/PluginManager.hh"

namespace eudaq {

  class FileWriterNull : public FileWriter {
  public:
    FileWriterNull(const std::string &) {}
    virtual void StartRun(unsigned) {}
    virtual void WriteEvent(const DetectorEvent &devent) {
      if (devent.IsBORE()) {
        eudaq::PluginManager::Initialize(devent);
      }
    }
    virtual uint64_t FileBytes() const { return 0; }
  };

  namespace {
    RegisterFileWriter<FileWriterNull> reg("null");
  }
}
