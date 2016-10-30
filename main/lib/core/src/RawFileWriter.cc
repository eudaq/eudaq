#include "FileNamer.hh"
#include "FileWriter.hh"
#include "FileSerializer.hh"
#include "PluginManager.hh"

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
    virtual void StartRun(unsigned);
    virtual void WriteEvent(const DetectorEvent &);
    virtual uint64_t FileBytes() const;
    virtual ~RawFileWriter();

  private:
    FileSerializer *m_ser;
  };

  RawFileWriter::RawFileWriter(const std::string & param) : m_ser(0){
    //EUDAQ_DEBUG("Constructing RawFileWriter(" + to_string(param) + ")");
  }

  void RawFileWriter::StartRun(unsigned runnumber) {
    delete m_ser;
    m_ser = new FileSerializer(
        FileNamer(m_filepattern).Set('X', ".raw").Set('R', runnumber));
  }

  void RawFileWriter::WriteEvent(const DetectorEvent &ev) {
    if (!m_ser)
      EUDAQ_THROW("RawFileWriter: Attempt to write unopened file");
    if (ev.IsBORE()) {
      eudaq::PluginManager::Initialize(ev);
    }
    if (!m_ser) EUDAQ_THROW("RawFileWriter: Attempt to write unopened file");
    m_ser->write(ev);
    m_ser->Flush();
  }
  
  RawFileWriter::~RawFileWriter() {
    delete m_ser;
  }

  uint64_t RawFileWriter::FileBytes() const { return m_ser ? m_ser->FileBytes() : 0; }
}
