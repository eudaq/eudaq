#include "FileNamer.hh"
#include "FileWriter.hh"
#include "FileSerializer.hh"

namespace eudaq {
  class RawFileWriter;

  namespace{
    auto dummy0 = Factory<FileWriter>::Register<RawFileWriter, std::string&>(cstr2hash("native"));
    auto dummy1 = Factory<FileWriter>::Register<RawFileWriter, std::string&&>(cstr2hash("native"));
    auto dummy2 = Factory<FileWriter>::Register<RawFileWriter, std::string&>(cstr2hash("RawFileWriter"));
    auto dummy3 = Factory<FileWriter>::Register<RawFileWriter, std::string&&>(cstr2hash("RawFileWriter"));
  }
  
  class RawFileWriter : public FileWriter {
  public:
    RawFileWriter(const std::string &);
    virtual void StartRun(uint32_t);
    virtual void WriteEvent(EventSPC ev);
    virtual uint64_t FileBytes() const;
    virtual ~RawFileWriter();

  private:
    std::unique_ptr<FileSerializer> m_ser;
    std::string m_filepattern;
  };

  RawFileWriter::RawFileWriter(const std::string & param){
    m_filepattern = param;
  }

  void RawFileWriter::StartRun(uint32_t runnumber) {
    m_ser.reset(new FileSerializer((FileNamer(m_filepattern).Set('X', ".raw").Set('R', runnumber))));
  }

  void RawFileWriter::WriteEvent(EventSPC ev) {
    if (!m_ser)
      EUDAQ_THROW("RawFileWriter: Attempt to write unopened file");
    m_ser->write(*(ev.get())); //TODO: Serializer accepts EventSPC
    m_ser->Flush();
  }
  
  RawFileWriter::~RawFileWriter() {
  }

  uint64_t RawFileWriter::FileBytes() const { return m_ser ? m_ser->FileBytes() : 0; }
}
