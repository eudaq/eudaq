#include "eudaq/FileWriterNative.hh"
#include "eudaq/FileNamer.hh"

namespace eudaq {

  namespace {
    static RegisterFileWriter<FileWriterNative> reg("native");
  }

  FileWriterNative::FileWriterNative(const std::string &) : m_ser(0) {}

  void FileWriterNative::StartRun(unsigned runnumber) {
    if (m_ser) delete m_ser;
    m_ser = new FileSerializer(FileNamer(m_filepattern).Set('X', ".raw").Set('R', runnumber));
  }

  void FileWriterNative::WriteEvent(const DetectorEvent & ev) {
    if (!m_ser) EUDAQ_THROW("FileWriterNative: Attempt to write unopened file");
    m_ser->write(ev);
    m_ser->Flush();
  }

  FileWriterNative::~FileWriterNative() {
    if (m_ser) delete m_ser;
  }

  unsigned long long FileWriterNative::FileBytes() const { return m_ser ? m_ser->FileBytes() : 0; }

}
