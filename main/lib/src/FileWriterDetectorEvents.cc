#include "eudaq/FileWriter.hh"
#include "eudaq/FileNamer.hh"
#include "eudaq/PluginManager.hh"
#include <iostream>
#include <fstream>

namespace eudaq {

  class FileWriterTextdetector : public FileWriter {
  public:
    FileWriterTextdetector(const std::string &);
    virtual void StartRun(unsigned);
    virtual void WriteEvent(const DetectorEvent &);
    virtual uint64_t FileBytes() const;
    virtual ~FileWriterTextdetector();

  private:
    //  std::FILE * m_file;
    std::ofstream *m_out;
    bool firstEvent;
    uint64_t DUT_start_time;
  };

  namespace {
    static RegisterFileWriter<FileWriterTextdetector> reg("detector");
  }

  FileWriterTextdetector::FileWriterTextdetector(const std::string &param)
      : firstEvent(false), m_out(nullptr), DUT_start_time(0) {
    std::cout
        << "EUDAQ_DEBUG: This is FileWriterTextdetector::FileWriterTextCompact("
        << param << ")" << std::endl;
  }

  void FileWriterTextdetector::StartRun(unsigned runnumber) {
    std::cout << "EUDAQ_DEBUG: FileWriterTextdetector::StartRun(" << runnumber
              << ")" << std::endl;
    // close an open file
    if (m_out) {
      m_out->close();
      m_out = nullptr;
    }

    // open a new file
    std::string fname(
        FileNamer(m_filepattern).Set('X', ".txt").Set('R', runnumber));
    m_out = new std::ofstream(fname.c_str());

    if (!m_out)
      EUDAQ_THROW("Error opening file: " + fname);
  }

  void FileWriterTextdetector::WriteEvent(const DetectorEvent &devent) {

    if (devent.IsBORE()) {
      eudaq::PluginManager::Initialize(devent);
      firstEvent = true;
      return;
    } else if (devent.IsEORE()) {
      return;
    }

    *m_out << "================================================================"
              "==============" << std::endl;
    for (size_t i = 0; i < devent.NumEvents(); ++i) {
      auto raw = std::dynamic_pointer_cast<RawDataEvent>(devent.GetEventPtr(i));

      *m_out << devent.GetEvent(i)->GetRunNumber() << ";      ";
      *m_out << devent.GetEvent(i)->GetEventNumber() << ";       ";
      *m_out << devent.GetEvent(i)->get_id() << ";         ";
      *m_out << Event::id2str(devent.GetEvent(i)->get_id()) << ";       ";

      if (raw) {
        *m_out << raw->GetSubType() << "      ";
      } else {
        *m_out << "           ";
      }

      *m_out << std::endl;
    }
    *m_out << "================================================================"
              "==============" << std::endl;
  }

  FileWriterTextdetector::~FileWriterTextdetector() {
    if (m_out) {
      m_out->close();

      m_out = nullptr;
    }
  }

  uint64_t FileWriterTextdetector::FileBytes() const { return m_out->tellp(); }
}
