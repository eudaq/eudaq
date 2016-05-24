#include "eudaq/FileNamer.hh"
#include "eudaq/FileWriter.hh"
#include "eudaq/FileSerializer.hh"
#include "eudaq/BufferSerializer.hh"
#include "eudaq/Event.hh"

namespace eudaq {

  class FileWriterNative2 : public FileWriter {
  public:
    FileWriterNative2(const std::string &);
    virtual void StartRun(unsigned);
    virtual void WriteEvent(const DetectorEvent &);
    virtual uint64_t FileBytes() const;
    virtual ~FileWriterNative2();

  private:
    BufferSerializer m_buf;
    FileSerializer *m_ser;
  };

  

  registerFileWriter(FileWriterNative2, "native2");
  
  
  FileWriterNative2::FileWriterNative2(const std::string & /*param*/) : m_ser(0) {
    //EUDAQ_DEBUG("Constructing FileWriterNative(" + to_string(param) + ")");
  }

  void FileWriterNative2::StartRun(unsigned runnumber) {
    delete m_ser;
    m_ser = new FileSerializer(
        FileNamer(m_filepattern).Set('X', ".raw").Set('R', runnumber));
    unsigned versiontag = Event::str2id("VER2");
    m_ser->write(versiontag);
  }

  void FileWriterNative2::WriteEvent(const DetectorEvent &ev) {
    if (!m_ser)
      EUDAQ_THROW("FileWriterNative: Attempt to write unopened file");
    m_buf.clear();
    m_buf.write(ev);
    m_ser->write(m_buf);
    m_ser->Flush();
  }

  FileWriterNative2::~FileWriterNative2() { delete m_ser; }

  uint64_t FileWriterNative2::FileBytes() const {
    return m_ser ? m_ser->FileBytes() : 0;
  }
}
