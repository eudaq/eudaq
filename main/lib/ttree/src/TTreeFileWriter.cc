#include "eudaq/FileNamer.hh"
#include "eudaq/FileWriter.hh"
#include "eudaq/Configuration.hh"
#include "eudaq/TTreeEventConverter.hh"
#include <ostream>
#include <ctime>
#include <iomanip>


#include "TFile.h"
#include "TTree.h"
#include "TRandom.h"
#include "TString.h"


namespace eudaq {
  class TTreeFileWriter;
  namespace{
    auto dummy01 = Factory<FileWriter>::Register<TTreeFileWriter, std::string&>(cstr2hash("root"));
    auto dummy11 = Factory<FileWriter>::Register<TTreeFileWriter, std::string&&>(cstr2hash("root"));
  }

  class TTreeFileWriter : public FileWriter {
  public:
    TTreeFileWriter(const std::string &patt);
    void WriteEvent(EventSPC ev) override;
  private:
    std::unique_ptr<TTreeFileWriter> m_ttreewriter;
    std::string m_filepattern;
    uint32_t m_run_n;
    TFile *m_tfile; // book the pointer to a file (to store the otuput)
  };

  TTreeFileWriter::TTreeFileWriter(const std::string &patt){
    m_filepattern = patt;    
  }
  
  void TTreeFileWriter::WriteEvent(EventSPC ev) {
    std::string foutput(FileNamer(m_filepattern).Set('X', ".root").Set('R', m_run_n));
    EUDAQ_INFO("Preparing the outputfile: " + foutput);
    m_tfile = new TFile(foutput.c_str(), "RECREATE");
    
  }
}
