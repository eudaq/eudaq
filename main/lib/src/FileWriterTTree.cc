#ifdef ROOT_FOUND

#include "eudaq/FileNamer.hh"
#include "eudaq/FileWriter.hh"
#include "eudaq/PluginManager.hh"
#include "eudaq/Logger.hh"

#include "TFile.h"
#include "TTree.h"
#include "TRandom.h"
#include "TString.h"
#include <memory>



namespace eudaq {


class rootEvent {
public:
  rootEvent(TTree* outTtree) {
    outTtree->Branch("event_nr", &m_event_nr);
    outTtree->Branch("x", &m_x);
    outTtree->Branch("y", &m_y);
    outTtree->Branch("ID", &m_id);
    outTtree->Branch("Charge", &m_charge);

  }

  void reset() {
    m_charge.clear();
    m_id.clear();
    m_x.clear();
    m_y.clear();
  }

  void setEventNr(int eventNR) {
    m_event_nr = eventNR;
  }

  void push_hit(Double_t x, Double_t y, Double_t charge, Double_t ID_) {
    m_x.push_back(x);
    m_y.push_back(y);
    m_charge.push_back(charge);
    m_id.push_back(ID_);
  }
private:
  std::vector<Double_t> m_id;
  std::vector<Double_t> m_x;
  std::vector<Double_t> m_charge;
  std::vector<Double_t> m_y;
  int m_event_nr = 0;
};

class FileWriterTTree : public FileWriter {
public:
  FileWriterTTree(const std::string &);
  virtual void StartRun(unsigned) override;
  virtual void WriteEvent(const DetectorEvent &) override;
  virtual uint64_t FileBytes() const override;
  virtual ~FileWriterTTree();
private:
  TFile * m_tfile =nullptr; // book the pointer to a file (to store the otuput)
  TTree * outTtreem_ttree; // book the tree (to store the needed event info)
  std::string m_fileName;
  std::shared_ptr<rootEvent> m_event;


};
registerFileWriter(FileWriterTTree, "ttree");
FileWriterTTree::FileWriterTTree(const std::string & name):m_fileName(name) {
#ifdef _DEBUG
#ifdef WIN32

 // EUDAQ_THROW("debug mode is not supported in windows for this producer");
#endif // WIN32
#endif // _DEBUG

}


void FileWriterTTree::StartRun(unsigned runnumber) {
  EUDAQ_INFO("Converting the inputfile into a TTree ");
  std::string foutput(FileNamer(m_filepattern).Set('X', ".root").Set('R', runnumber));
  EUDAQ_INFO("Preparing the outputfile: " + foutput);

  m_tfile = new TFile(foutput.c_str(), "RECREATE");
  outTtreem_ttree = new TTree("szData", "a simple Tree with simple variables");
  outTtreem_ttree->SetDirectory(m_tfile->GetDirectory("/"));
  m_event = std::make_shared<rootEvent>(outTtreem_ttree);

}

void FileWriterTTree::WriteEvent(const DetectorEvent &dev) {


  if (dev.IsBORE()) {
    eudaq::PluginManager::Initialize(dev);
    
    return;
  } else if (dev.IsEORE()) {
    return;
  }
  StandardEvent sev = eudaq::PluginManager::ConvertToStandard(dev);
  m_event->reset();

  for (size_t iplane = 0; iplane < sev.NumPlanes(); ++iplane) {

    const auto & plane = sev.GetPlane(iplane);
    auto cds = plane.GetPixels<double>();

    for (size_t ipix = 0; ipix < cds.size(); ++ipix) {


      m_event->push_hit(
        plane.GetX(ipix),
        plane.GetY(ipix),
        plane.GetPixel(ipix),
        plane.ID()
        );
    }
  }
 
  outTtreem_ttree->Fill();

}

uint64_t FileWriterTTree::FileBytes() const {
  return 0;
}

FileWriterTTree::~FileWriterTTree() {
 
 if (m_tfile)
 {
   m_tfile->Write();
  m_tfile->Close();
  delete m_tfile;
  m_tfile = nullptr;
 }
 
}

}



#endif // ROOT_FOUND
