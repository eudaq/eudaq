#include "eudaq/FileNamer.hh"
#include "eudaq/FileWriter.hh"
#include "eudaq/FileSerializer.hh"
//#include "eudaq/Logger.hh"

namespace eudaq {

  class FileWriterNative : public FileWriter {
  public:
    FileWriterNative(const std::string &);
    virtual void StartRun(unsigned);
    virtual void WriteEvent(const DetectorEvent &);
    virtual unsigned long long FileBytes() const;
    virtual ~FileWriterNative();
  private:
    FileSerializer * m_ser;
  };

  namespace {
    static RegisterFileWriter<FileWriterNative> reg("native");
  }

  FileWriterNative::FileWriterNative(const std::string & /*param*/) : m_ser(0) {
    //EUDAQ_DEBUG("Constructing FileWriterNative(" + to_string(param) + ")");
  }

  void FileWriterNative::StartRun(unsigned runnumber) {
    delete m_ser;
    m_ser = new FileSerializer(FileNamer(m_filepattern).Set('X', ".raw").Set('R', runnumber));
  }

  void FileWriterNative::WriteEvent(const DetectorEvent & ev) {
    if (!m_ser) EUDAQ_THROW("FileWriterNative: Attempt to write unopened file");
    m_ser->write(ev);
    m_ser->Flush();
  }

  FileWriterNative::~FileWriterNative() {
    delete m_ser;
  }

  unsigned long long FileWriterNative::FileBytes() const { return m_ser ? m_ser->FileBytes() : 0; }

}
