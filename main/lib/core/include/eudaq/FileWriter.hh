#ifndef EUDAQ_INCLUDED_FileWriter
#define EUDAQ_INCLUDED_FileWriter

#include "eudaq/Factory.hh"
#include "eudaq/Event.hh"
#include "eudaq/Configuration.hh"

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

  //----------DOC-MARK-----BEG*DEC-----DOC-MARK----------
  class DLLEXPORT FileWriter {
  public:
    FileWriter();
    virtual ~FileWriter() {}
    void SetConfiguration(ConfigurationSPC c) {m_conf = c;};
    ConfigurationSPC GetConfiguration() const {return m_conf;};
    virtual void WriteEvent(EventSPC ) {};
    virtual uint64_t FileBytes() const {return 0;};
    static FileWriterSP Make(std::string type, std::string path);
  private:
    ConfigurationSPC m_conf;
  };
  //----------DOC-MARK-----END*DEC-----DOC-MARK----------
}

#endif // EUDAQ_INCLUDED_FileWriter
