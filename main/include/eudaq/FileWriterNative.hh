#ifndef EUDAQ_INCLUDED_FileWriterNative
#define EUDAQ_INCLUDED_FileWriterNative

#include "eudaq/FileWriter.hh"
#include "eudaq/FileSerializer.hh"

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

}

#endif // #ifndef EUDAQ_INCLUDED_FileWriterNative
