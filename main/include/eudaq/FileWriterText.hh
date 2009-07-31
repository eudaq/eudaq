#ifndef EUDAQ_INCLUDED_FileWriterText
#define EUDAQ_INCLUDED_FileWriterText

#include "eudaq/FileWriter.hh"
#include <cstdio>

namespace eudaq {

  class FileWriterText : public FileWriter {
  public:
    FileWriterText(const std::string &);
    virtual void StartRun(unsigned);
    virtual void WriteEvent(const DetectorEvent &);
    virtual unsigned long long FileBytes() const;
    virtual ~FileWriterText();
  private:
    std::FILE * m_file;
  };

}

#endif // EUDAQ_INCLUDED_FileWriterText
