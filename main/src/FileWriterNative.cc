#include "eudaq/FileWriterNative.hh"

namespace eudaq {

  namespace {
    static RegisterFileWriter<FileWriterNative> reg("NATIVE");
  }

  FileWriterNative::FileWriterNative(const std::string &) : m_ser(0) {}

  void FileWriterNative::StartRun(unsigned runnumber) {
    if (m_ser) delete m_ser;
    m_ser = new FileSerializer(GenFilename(runnumber, ".raw"));
  }

  void FileWriterNative::WriteEvent(const DetectorEvent & ev) {
    if (!m_ser) EUDAQ_THROW("FileWriterNative: Attempt to write unopened file");
    m_ser->write(ev);
  }

  FileWriterNative::~FileWriterNative() {
    if (m_ser) delete m_ser;
  }

  unsigned long long FileWriterNative::FileBytes() const { return m_ser ? m_ser->FileBytes() : 0; }

}
