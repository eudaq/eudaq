#include "eudaq/FileWriter.hh"
#include "eudaq/FileNamer.hh"
#include "eudaq/PluginManager.hh"
#include <iostream>

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

  namespace {
    static RegisterFileWriter<FileWriterText> reg("text");
  }

  FileWriterText::FileWriterText(const std::string & param)
    : m_file(0)
  {
    std::cout << "EUDAQ_DEBUG: This is FileWriterText::FileWriterText("<< param <<")"<< std::endl;
  }

  void FileWriterText::StartRun(unsigned runnumber) {
    std::cout << "EUDAQ_DEBUG: FileWriterText::StartRun("<< runnumber << ")" << std::endl;
    // close an open file
    if (m_file) {
      std::fclose(m_file);
      m_file = 0;
    }

    // open a new file
    std::string fname(FileNamer(m_filepattern).Set('X', ".txt").Set('R', runnumber));
    m_file = std::fopen(fname.c_str(), "wb");
    if (!m_file) EUDAQ_THROW("Error opening file: " + fname);
  }

  void FileWriterText::WriteEvent(const DetectorEvent & devent) {
    std::cout << "EUDAQ_DEBUG: FileWriterText::WriteEvent() processing event "
      <<  devent.GetRunNumber() <<"." << devent.GetEventNumber() << std::endl;

    //disentangle the detector event
    StandardEvent sevent(PluginManager::ConvertToStandard(devent));
    std::cout << "Event: " << sevent << std::endl;
  }

  FileWriterText::~FileWriterText() {
    if (m_file) {
      std::fclose(m_file);
      m_file = 0;
    }
  }

  unsigned long long FileWriterText::FileBytes() const { return std::ftell(m_file); }

}
