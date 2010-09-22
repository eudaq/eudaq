#include "eudaq/FileNamer.hh"
#include "eudaq/FileWriter.hh"
#include "eudaq/FileSerializer.hh"
#include "eudaq/PluginManager.hh"
//#include "eudaq/Logger.hh"

namespace eudaq {

  class FileWriterStandard : public FileWriter {
  public:
    FileWriterStandard(const std::string &);
    virtual void StartRun(unsigned);
    virtual void WriteEvent(const DetectorEvent &);
    virtual unsigned long long FileBytes() const;
    virtual ~FileWriterStandard();
  private:
    FileSerializer * m_ser;
  };

  namespace {
    static RegisterFileWriter<FileWriterStandard> reg("standard");
  }

  FileWriterStandard::FileWriterStandard(const std::string & /*param*/) : m_ser(0) {
    //EUDAQ_DEBUG("Constructing FileWriterStandard(" + to_string(param) + ")");
  }

  void FileWriterStandard::StartRun(unsigned runnumber) {
    delete m_ser;
    m_ser = new FileSerializer(FileNamer(m_filepattern).Set('X', ".std").Set('R', runnumber));
  }

  void FileWriterStandard::WriteEvent(const DetectorEvent & ev) {
    if (!m_ser) EUDAQ_THROW("FileWriterStandard: Attempt to write unopened file");
    if (ev.IsBORE()) PluginManager::Initialize(ev);
    m_ser->write(PluginManager::ConvertToStandard(ev));
    m_ser->Flush();
  }

  FileWriterStandard::~FileWriterStandard() {
    delete m_ser;
  }

  unsigned long long FileWriterStandard::FileBytes() const { return m_ser ? m_ser->FileBytes() : 0; }

}
