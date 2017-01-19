#include "FileNamer.hh"
#include "FileWriter.hh"
#include "FileSerializer.hh"

namespace eudaq {
  class RawFileWriter;

  namespace{
    auto dummy0 = Factory<FileWriter>::Register<RawFileWriter, std::string&>(cstr2hash("native"));
    auto dummy1 = Factory<FileWriter>::Register<RawFileWriter, std::string&&>(cstr2hash("native"));
    auto dummy10 = Factory<FileWriter>::Register<RawFileWriter, std::string&>(cstr2hash("raw"));
    auto dummy11 = Factory<FileWriter>::Register<RawFileWriter, std::string&&>(cstr2hash("raw"));
  }
  
  class RawFileWriter : public FileWriter {
  public:
    RawFileWriter(const std::string &patt);
    void WriteEvent(EventSPC ev) override;
    uint64_t FileBytes() const override;
  private:
    std::unique_ptr<FileSerializer> m_ser;
    std::string m_filepattern;
    uint32_t m_run_n;
  };

  RawFileWriter::RawFileWriter(const std::string &patt){
    m_filepattern = patt;
  }
  
  void RawFileWriter::WriteEvent(EventSPC ev) {
    uint32_t run_n = ev->GetRunN();
    if(!m_ser || m_run_n != run_n){
      m_ser.reset(new FileSerializer((FileNamer(m_filepattern).Set('X', ".raw").Set('R', run_n))));
      m_run_n = run_n;
    }
    if(!m_ser)
      EUDAQ_THROW("RawFileWriter: Attempt to write unopened file");
    m_ser->write(*(ev.get())); //TODO: Serializer accepts EventSPC
    m_ser->Flush();
  }
  

  uint64_t RawFileWriter::FileBytes() const { return m_ser ? m_ser->FileBytes() : 0; }
}
