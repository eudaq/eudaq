#ifndef EUDAQ_INCLUDED_FileWriter
#define EUDAQ_INCLUDED_FileWriter

#include "DetectorEvent.hh"
#include "Factory.hh"

#include <vector>
#include <string>
#include <memory>

namespace eudaq {
  class FileWriter;
  
#ifndef EUDAQ_CORE_EXPORTS
  extern template class DLLEXPORT Factory<FileWriter>;
  extern template DLLEXPORT
  std::map<uint32_t, typename Factory<FileWriter>::UP_BASE (*)(std::string&)>&
  Factory<FileWriter>::Instance<std::string&>();
  extern template DLLEXPORT
  std::map<uint32_t, typename Factory<FileWriter>::UP_BASE (*)(std::string&&)>&
  Factory<FileWriter>::Instance<std::string&&>();
#endif
  
  using FileWriterUP = Factory<FileWriter>::UP_BASE;
  using FileWriterSP = Factory<FileWriter>::SP_BASE;
  using FileWriter_up = FileWriterUP;

  class DLLEXPORT FileWriter {
  public:
    FileWriter();
    virtual void StartRun(unsigned runnumber) = 0;
    virtual void WriteBaseEvent(const Event&);
    virtual void WriteEvent(const DetectorEvent &) = 0;
    virtual uint64_t FileBytes() const = 0;
    void SetFilePattern(const std::string &p) { m_filepattern = p; }
    virtual ~FileWriter() {}

  protected:
    std::string m_filepattern;
  };

}

#endif // EUDAQ_INCLUDED_FileWriter
