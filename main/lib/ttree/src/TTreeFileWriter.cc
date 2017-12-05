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
    std::unique_ptr<TFile> m_ttreewriter;
    std::string m_filepattern;
    uint32_t m_run_n;
    TFile *m_tfile; // book the pointer to a file (to store the otuput)
  };

  TTreeFileWriter::TTreeFileWriter(const std::string &patt){
    m_filepattern = patt;    
  }
  
  void TTreeFileWriter::WriteEvent(EventSPC ev) {
    uint32_t run_n = ev->GetRunN();
      if(!m_ttreewriter || m_run_n != run_n){
      try{
	std::time_t time_now = std::time(nullptr);
	char time_buff[13];
	time_buff[12] = 0;
	std::strftime(time_buff, sizeof(time_buff), "%y%m%d%H%M%S", std::localtime(&time_now));
	std::string time_str(time_buff);
	std::string foutput(FileNamer(m_filepattern).Set('X', ".root").Set('R', m_run_n).Set('D', time_str));
	m_tfile = new TFile(foutput.c_str(), "RECREATE");
	EUDAQ_INFO("Preparing the outputfile: " + foutput);
	m_ttreewriter.reset(m_tfile);
	m_run_n = run_n;
      } catch(const Exception &e){
      EUDAQ_THROW(std::string("Fail to open ROOT file")+e.what());
    }
    }
    if(!m_ttreewriter)
      EUDAQ_THROW("TTreeFileWriter: Attempt to write unopened file");
    TTreeEventSP ttree(new TTree);
    TTreeEventConverter::Convert(ev, ttree, GetConfiguration());
    ttree->Write();
    m_tfile->Write();
    //    m_tfile->Close();    
}
}
