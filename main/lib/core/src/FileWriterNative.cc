#include "FileNamer.hh"
#include "FileWriter.hh"
#include "FileSerializer.hh"
#include "PluginManager.hh"

namespace eudaq {
  class FileWriterNative;

  namespace{
    auto dummy0 = Factory<FileWriter>::Register<FileWriterNative, std::string&>(cstr2hash("native"));
    auto dummy1 = Factory<FileWriter>::Register<FileWriterNative, std::string&&>(cstr2hash("native"));
  }
  
  class FileWriterNative : public FileWriter {
  public:
    FileWriterNative(const std::string &);
    virtual void StartRun(unsigned);
    virtual void WriteEvent(const DetectorEvent &);
    virtual uint64_t FileBytes() const;
    virtual ~FileWriterNative();

  private:
    FileSerializer *m_ser;
  };

  FileWriterNative::FileWriterNative(const std::string & param) : m_ser(0){
    //EUDAQ_DEBUG("Constructing FileWriterNative(" + to_string(param) + ")");
  }

  void FileWriterNative::StartRun(unsigned runnumber) {
    delete m_ser;
    m_ser = new FileSerializer(
        FileNamer(m_filepattern).Set('X', ".raw").Set('R', runnumber));
  }

  void FileWriterNative::WriteEvent(const DetectorEvent &ev) {
    if (!m_ser)
      EUDAQ_THROW("FileWriterNative: Attempt to write unopened file");
    if (ev.IsBORE()) {
      eudaq::PluginManager::Initialize(ev);
    }
    if (!m_ser) EUDAQ_THROW("FileWriterNative: Attempt to write unopened file");
    m_ser->write(ev);
    m_ser->Flush();
  }
  

  FileWriterNative::~FileWriterNative() {
    delete m_ser;
  }

  uint64_t FileWriterNative::FileBytes() const { return m_ser ? m_ser->FileBytes() : 0; }
}
