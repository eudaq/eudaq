#include "eudaq/FileWriter.hh"
#include "eudaq/FileNamer.hh"
#include "eudaq/PluginManager.hh"
#include <iostream>
#include <fstream>

#if ((defined WIN32) && (defined __CINT__))
typedef unsigned long long uint64_t typedef long long
    int64_t typedef unsigned int uint32_t typedef int int32_t
#else
#include <cstdint>
#endif

        uint64_t timediff = 0;
uint64_t timediff1 = (uint64_t)-1;
namespace eudaq {

  class FileWriterText : public FileWriter {
  public:
    FileWriterText(const std::string &);
    virtual void StartRun(unsigned);
    virtual void WriteEvent(const DetectorEvent &);
    virtual uint64_t FileBytes() const;
    virtual ~FileWriterText();

  private:
    std::ofstream *m_out;
    bool firstEvent;
  };

  namespace {
    static RegisterFileWriter<FileWriterText> reg("text");
  }

  FileWriterText::FileWriterText(const std::string &param)
      : firstEvent(false), m_out(nullptr) {
    std::cout << "EUDAQ_DEBUG: This is FileWriterText::FileWriterText(" << param
              << ")" << std::endl;
  }

  void FileWriterText::StartRun(unsigned runnumber) {
    std::cout << "EUDAQ_DEBUG: FileWriterText::StartRun(" << runnumber << ")"
              << std::endl;
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

  void FileWriterText::WriteEvent(const DetectorEvent &devent) {

#ifndef FILEWRITEROLD

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

        *m_out << "i_time_stamp;  TLU_Trigger_input; id_plane; id_hit; id_x; "
                  "id_y; i_tlu; i_run; i_event; DUT_time_stamp" << std::endl;
      }

      firstEvent = false;
    }
    if (sev.NumPlanes() == 0) // only TLU Events
    {

      *m_out << sev.GetTimestamp() / 384066 << ";  "
             << sev.GetTag("TLU_trigger") << std::endl;
    }
    for (size_t iplane = 0; iplane < sev.NumPlanes(); ++iplane) {

      const eudaq::StandardPlane &plane = sev.GetPlane(iplane);
      std::vector<double> cds = plane.GetPixels<double>();

      for (size_t ipix = 0; ipix < cds.size(); ++ipix) {

        if (plane.GetX(ipix) > 0) {

          //          if (ipix < 10) std::cout << ", " << plane.m_pix[0][ipix]
          //          << ";" << cds[ipix]
          *m_out << sev.GetTimestamp() / 384066 << "; ";
          *m_out << sev.GetTag("TLU_trigger") << "; ";
          *m_out << plane.ID() << "; ";
          *m_out << ipix << "; ";
          *m_out << plane.GetX(ipix) << "; ";
          *m_out << plane.GetY(ipix) << "; ";
          *m_out << plane.TLUEvent() << "; ";
          *m_out << sev.GetRunNumber() << "; ";
          *m_out << sev.GetEventNumber() << "; ";

          *m_out << std::endl;
        }
      }
    }

#else

    std::cout << "EUDAQ_DEBUG: FileWriterText::WriteEvent() processing event "
              << devent.GetRunNumber() << "." << devent.GetEventNumber()
              << std::endl;

    // disentangle the detector event
    StandardEvent sevent(PluginManager::ConvertToStandard(devent));
    std::cout << "Event: " << sevent << std::endl;
// sevent.Print(std::cout);
#endif
  }

  FileWriterText::~FileWriterText() {
    if (m_out) {
      m_out->close();

      m_out = nullptr;
    }
  }

  uint64_t FileWriterText::FileBytes() const { return m_out->tellp(); }
}
