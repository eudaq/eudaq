#include "eudaq/FileNamer.hh"
#include "eudaq/FileWriter.hh"
#include "eudaq/Configuration.hh"
#include "eudaq/LCEventConverter.hh"
#include <ostream>
#include <ctime>
#include <iomanip>


#include "lcio.h"
#include "EVENT/LCIO.h"
#include "IO/ILCFactory.h"
#include "IO/LCWriter.h"
#include "IMPL/LCEventImpl.h"
#include "IMPL/LCCollectionVec.h"
#include "IMPL/LCTOOLS.h"

namespace eudaq {
  class LCFileWriter;

  namespace{
    auto dummy01 = Factory<FileWriter>::Register<LCFileWriter, std::string&>(cstr2hash("slcio"));
    auto dummy11 = Factory<FileWriter>::Register<LCFileWriter, std::string&&>(cstr2hash("slcio"));
  }

  class LCFileWriter : public FileWriter {
  public:
    LCFileWriter(const std::string &patt);
    void WriteEvent(EventSPC ev) override;
  private:
    std::unique_ptr<lcio::LCWriter> m_lcwriter;
    std::string m_filepattern;
    uint32_t m_run_n;
  };

  LCFileWriter::LCFileWriter(const std::string &patt){
    m_filepattern = patt;
  }

  void LCFileWriter::WriteEvent(EventSPC ev) {
    uint32_t run_n = ev->GetRunN();
    if(!m_lcwriter || m_run_n != run_n){
      try {
	m_lcwriter.reset(lcio::LCFactory::getInstance()->createLCWriter());
	std::time_t time_now = std::time(nullptr);
	char time_buff[13];
	time_buff[12] = 0;
	std::strftime(time_buff, sizeof(time_buff), "%y%m%d%H%M%S", std::localtime(&time_now));
	std::string time_str(time_buff);
	m_lcwriter->open(FileNamer(m_filepattern).Set('R', run_n).Set('D', time_str),
			 lcio::LCIO::WRITE_NEW);
	m_run_n = run_n;
      } catch (const lcio::IOException &e) {
	EUDAQ_THROW(std::string("Fail to open LCIO file")+e.what());
      }
    }
    if(!m_lcwriter)
      EUDAQ_THROW("LCFileWriter: Attempt to write unopened file");
    LCEventSP lcevent(new lcio::LCEventImpl);
    LCEventConverter::Convert(ev, lcevent, GetConfiguration());
    m_lcwriter->writeEvent(lcevent.get());
  }
}
