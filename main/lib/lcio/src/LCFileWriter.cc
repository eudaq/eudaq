#include "eudaq/FileNamer.hh"
#include "eudaq/FileWriter.hh"
#include "eudaq/Configuration.hh"
#include "eudaq/LCEventConverter.hh"


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
    auto dummy0 = Factory<FileWriter>::Register<LCFileWriter, std::string&>(cstr2hash("lcio"));
    auto dummy1 = Factory<FileWriter>::Register<LCFileWriter, std::string&&>(cstr2hash("lcio"));
    auto dummy01 = Factory<FileWriter>::Register<LCFileWriter, std::string&>(cstr2hash("slcio"));
    auto dummy11 = Factory<FileWriter>::Register<LCFileWriter, std::string&&>(cstr2hash("slcio"));
    auto dummy2 = Factory<FileWriter>::Register<LCFileWriter, std::string&>(cstr2hash("LCFileWriter"));
    auto dummy3 = Factory<FileWriter>::Register<LCFileWriter, std::string&&>(cstr2hash("LCFileWriter"));
  }

  class LCFileWriter : public FileWriter {
  public:
    LCFileWriter(const std::string &);
    void StartRun(uint32_t) override;
    void WriteEvent(EventSPC ev) override;
    uint64_t FileBytes() const override {return -1;};
  private:
    std::unique_ptr<lcio::LCWriter> m_lcwriter;
    std::string m_filepattern;
    Configuration m_conf;
  };

  LCFileWriter::LCFileWriter(const std::string &param){
    m_filepattern = param;
  }

  void LCFileWriter::StartRun(unsigned runnumber) {
    try {
      m_lcwriter.reset(lcio::LCFactory::getInstance()->createLCWriter());
      m_lcwriter->open(FileNamer(m_filepattern).Set('R', runnumber),
		       lcio::LCIO::WRITE_NEW); 
    } catch (const lcio::IOException &e) {
      std::cout << e.what() << std::endl;
      /// FIXME Error message to run control and logger
    }
  }

  void LCFileWriter::WriteEvent(EventSPC ev) {
    if (!m_lcwriter)
      EUDAQ_THROW("LCFileWriter: Attempt to write unopened file");
    LCEventSP lcevent(new lcio::LCEventImpl);
    LCEventConverter::Convert(ev, lcevent, m_conf);
    m_lcwriter->writeEvent(lcevent.get());
  }

}
