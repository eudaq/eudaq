#if USE_ROOT

#include "eudaq/FileNamer.hh"
#include "eudaq/FileWriter.hh"
#include "eudaq/PluginManager.hh"
#include "eudaq/Logger.hh"

# include "TFile.h"
# include "TTree.h"
# include "TRandom.h"
# include "TString.h"

namespace eudaq {

  class FileWriterRoot : public FileWriter {
    public:
      FileWriterRoot(const std::string &);
      virtual void StartRun(unsigned);
      virtual void WriteEvent(const DetectorEvent &);
      virtual unsigned long long FileBytes() const;
      virtual ~FileWriterRoot();
    private:
      TFile * m_tfile; // book the pointer to a file (to store the otuput)
      TTree * m_ttree; // book the tree (to store the needed event info)
      // Book variables for the Event_to_TTree conversion 
      Int_t id_plane; // plane id, where the hit is 
      Int_t id_hit; // the hit id (within a plane)  
      Double_t  id_x; // the hit position along  X-axis  
      Double_t  id_y; // the hit position along  Y-axis  
      unsigned i_tlu; // a trigger id
      unsigned i_run; // a run  number 
      unsigned i_event; // an event number 
      unsigned long long int i_time_stamp; // the time stamp
  };

  namespace {
    static RegisterFileWriter<FileWriterRoot> reg("root");
  }

  FileWriterRoot::FileWriterRoot(const std::string & /*param*/)
    : m_tfile(0), m_ttree(0),
    id_plane(0), id_hit(0),
    id_x(0), id_y(0),
    i_tlu(0), i_run(0),
    i_event(0), i_time_stamp(0)
  {
  }

  void FileWriterRoot::StartRun(unsigned runnumber) {
    EUDAQ_INFO("Converting the inputfile into a TTree " );
    std::string foutput(FileNamer(m_filepattern).Set('X', ".root").Set('R', runnumber));
    EUDAQ_INFO("Preparing the outputfile: " + foutput);
    m_tfile = new TFile(foutput.c_str(), "RECREATE");
    m_ttree = new TTree("tree", "a simple Tree with simple variables");

    i_run = runnumber;
    i_event = 0;
    i_tlu = 0;

    m_ttree->Branch("id_plane", &id_plane, "id_plane/I");
    m_ttree->Branch("id_hit", &id_hit, "id_hit/I");
    m_ttree->Branch("id_x", &id_x, "id_x/D");
    m_ttree->Branch("id_y", &id_y, "id_y/D");
    m_ttree->Branch("i_time_stamp", &i_time_stamp, "i_time_stamp/LLU");
    m_ttree->Branch("i_tlu", &i_tlu, "i_tlu/I");
    m_ttree->Branch("i_run", &i_run, "i_run/I");
    m_ttree->Branch("i_event", &i_event, "i_event/I");
  }

  void FileWriterRoot::WriteEvent(const DetectorEvent & ev) {
    if (ev.IsBORE()) {
      eudaq::PluginManager::Initialize(ev);
      return;
    } else if (ev.IsEORE()) {
      m_ttree->Write();
    }
    StandardEvent sev = eudaq::PluginManager::ConvertToStandard(ev);
    for (size_t iplane = 0; iplane < sev.NumPlanes(); ++iplane) {

      const eudaq::StandardPlane & plane = sev.GetPlane(iplane);
      std::vector<double> cds = plane.GetPixels<double>();

      for (size_t ipix = 0; ipix < cds.size(); ++ipix) {

        //          if (ipix < 10) std::cout << ", " << plane.m_pix[0][ipix] << ";" << cds[ipix]

        id_plane = plane.ID();          
        id_hit = ipix;
        id_x = plane.GetX(ipix);
        id_y = plane.GetY(ipix);
        i_time_stamp =  sev.GetTimestamp();
        //          printf("%#x \n", i_time_stamp);  
        i_tlu = plane.TLUEvent();   
        i_run = sev.GetRunNumber();
        i_event = sev.GetEventNumber();                  
        m_ttree->Fill(); 
      }               
    }
  }

  FileWriterRoot::~FileWriterRoot() {
  }

  unsigned long long FileWriterRoot::FileBytes() const { return 0; }

}

#endif // USE_ROOT
