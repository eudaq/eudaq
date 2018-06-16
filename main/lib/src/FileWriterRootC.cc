#ifdef ROOT_FOUND

#include "eudaq/FileNamer.hh"
#include "eudaq/FileWriter.hh"
#include "eudaq/PluginManager.hh"
#include "eudaq/Logger.hh"

//TODO This is an ugly workaround to re-enable windows compilation after change to VS2017. Somewhere a windows.h gets drawn in and the usual min/max macro problem occurs. Find the origin.
#undef min
#undef max
#include "TFile.h"
#include "TTree.h"
#include "TRandom.h"
#include "TString.h"

namespace eudaq {

  class FileWriterRootC : public FileWriter {
  public:
    FileWriterRootC(const std::string &);
    virtual void StartRun(unsigned);
    virtual void WriteEvent(const DetectorEvent &);
    virtual uint64_t FileBytes() const;
    virtual ~FileWriterRootC();

  private:
    TFile *m_tfile; // book the pointer to a file (to store the otuput)
    TTree *m_ttree; // book the tree (to store the needed event info)
    // Book variables for the Event_to_TTree conversion
    Int_t id_plane; // plane id, where the hit is

    unsigned i_tlu;                  // a trigger id
    unsigned i_run;                  // a run  number
    unsigned i_event;                // an event number
    uint64_t i_time_stamp, DUT_time; // the time stamp
  };

  namespace {
    static RegisterFileWriter<FileWriterRootC> reg("rootc");
  }

  FileWriterRootC::FileWriterRootC(const std::string & /*param*/)
      : m_tfile(0), m_ttree(0), id_plane(0), i_tlu(0), i_run(0), i_event(0),
        i_time_stamp(0) {}

  void FileWriterRootC::StartRun(unsigned runnumber) {
    EUDAQ_INFO("Converting the inputfile into a TTree ");
    std::string foutput(
        FileNamer(m_filepattern).Set('X', ".root").Set('R', runnumber));
    EUDAQ_INFO("Preparing the outputfile: " + foutput);
    m_tfile = new TFile(foutput.c_str(), "RECREATE");
    m_ttree = new TTree("tree", "a simple Tree with simple variables");

    i_run = runnumber;
    i_event = 0;
    i_tlu = 0;

    m_ttree->Branch("id_plane", &id_plane, "id_plane/I");
    m_ttree->Branch("i_time_stamp", &i_time_stamp, "i_time_stamp/LLU");
    m_ttree->Branch("DUT_time", &DUT_time, "DUT_time/LLU");
    m_ttree->Branch("i_tlu", &i_tlu, "i_tlu/I");
    m_ttree->Branch("i_run", &i_run, "i_run/I");
    m_ttree->Branch("i_event", &i_event, "i_event/I");
  }

  void FileWriterRootC::WriteEvent(const DetectorEvent &ev) {
    if (ev.IsBORE()) {
      eudaq::PluginManager::Initialize(ev);
      return;
    } else if (ev.IsEORE()) {
    }
    StandardEvent sev = eudaq::PluginManager::ConvertToStandard(ev);
    for (size_t iplane = 0; iplane < sev.NumPlanes(); ++iplane) {

      const eudaq::StandardPlane &plane = sev.GetPlane(iplane);
      std::vector<double> cds = plane.GetPixels<double>();

      DUT_time = sev.GetTag("DUT_time", (uint64_t)0);

      id_plane = plane.ID();
      i_time_stamp = sev.GetTimestamp();
      try {
        i_tlu = std::stoul(sev.GetTag("TLU_trigger", "15"));
      } catch (...) {
        std::cout << " error during converting "
                  << sev.GetTag("TLU_trigger", "15") << " to ull" << std::endl;
      }

      i_run = sev.GetRunNumber();
      i_event = sev.GetEventNumber();
      m_ttree->Fill();
    }
  }

  FileWriterRootC::~FileWriterRootC() {

    m_tfile->Close();
    delete m_tfile;
  }

  uint64_t FileWriterRootC::FileBytes() const { return 0; }
}

#endif // ROOT_FOUND
