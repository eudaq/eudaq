#include "eudaq/FileWriter.hh"
#include "eudaq/FileNamer.hh"
#include "eudaq/PluginManager.hh"
#include <iostream>
#include <fstream>

namespace eudaq {

  class FileWriterTextCompact : public FileWriter {
  public:
    FileWriterTextCompact(const std::string &);
    virtual void StartRun(unsigned);
    virtual void WriteEvent(const DetectorEvent &);
    virtual uint64_t FileBytes() const;
    virtual ~FileWriterTextCompact();

  private:
    //  std::FILE * m_file;
    std::ofstream *m_out;
    bool firstEvent;
    uint64_t DUT_start_time;
  };

  namespace {
    static RegisterFileWriter<FileWriterTextCompact> reg("textc");
  }

  FileWriterTextCompact::FileWriterTextCompact(const std::string &param)
      : firstEvent(false), m_out(nullptr), DUT_start_time(0) {
    std::cout
        << "EUDAQ_DEBUG: This is FileWriterTextCompact::FileWriterTextCompact("
        << param << ")" << std::endl;
  }

  void FileWriterTextCompact::StartRun(unsigned runnumber) {
    std::cout << "EUDAQ_DEBUG: FileWriterTextCompact::StartRun(" << runnumber
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

  void FileWriterTextCompact::WriteEvent(const DetectorEvent &devent) {

    if (devent.IsBORE()) {
      eudaq::PluginManager::Initialize(devent);
      firstEvent = true;
      return;
    } else if (devent.IsEORE()) {
      return;
    }
    StandardEvent sev = eudaq::PluginManager::ConvertToStandard(devent);

    if (firstEvent) {
      if (sev.NumPlanes() == 0) // only TLU Events
      {

        *m_out << "i_time_stamp;  TLU_trigger" << std::endl;

      } else {

        *m_out << "i_time_stamp; id_plane; i_tlu; i_event; DUT_time_stamp; "
               << std::endl;
      }

      DUT_start_time = sev.GetTag("DUT_time", (uint64_t)0);
      firstEvent = false;
    }
    if (sev.NumPlanes() == 0) // only TLU Events
    {

      *m_out << sev.GetTimestamp() << "; " << sev.GetTag("TLU_trigger")
             << std::endl;
    }
    for (size_t iplane = 0; iplane < sev.NumPlanes(); ++iplane) {

      const eudaq::StandardPlane &plane = sev.GetPlane(iplane);
      std::vector<double> cds = plane.GetPixels<double>();

      //          if (ipix < 10) std::cout << ", " << plane.m_pix[0][ipix] <<
      //          ";" << cds[ipix]
      *m_out << sev.GetTimestamp() / 384066 << "; ";
      *m_out << sev.GetTag("TLU_trigger") << "; ";
      *m_out << plane.ID() << "; ";
      *m_out << plane.TLUEvent() << "; ";
      *m_out << sev.GetEventNumber() << "; ";
      *m_out << sev.GetTag("DUT_time", (uint64_t)0) << "; ";
      // std::string dummy=sev.GetTag("TLU_input");
      //		*m_out<<std::stoi(sev.GetTag("TLU_input"))<<"; ";
      *m_out << std::endl;
    }
  }

  FileWriterTextCompact::~FileWriterTextCompact() {
    if (m_out) {
      m_out->close();

      m_out = nullptr;
    }
  }

  uint64_t FileWriterTextCompact::FileBytes() const { return m_out->tellp(); }
}
