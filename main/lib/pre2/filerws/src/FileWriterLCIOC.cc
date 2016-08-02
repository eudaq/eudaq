#if USE_LCIO
#include "eudaq/FileNamer.hh"
#include "eudaq/PluginManager.hh"
#include "eudaq/FileWriter.hh"

#include "IO/ILCFactory.h"
#include "IMPL/LCEventImpl.h"
#include "IMPL/LCCollectionVec.h"
#include "EVENT/LCIO.h"
#include "Exceptions.h"
#include "IMPL/LCTOOLS.h"
#include "IO/LCWriter.h"
#include "lcio.h"

#include <iostream>
#include <time.h>

//#include "eudaq/DataCollector.hh"

namespace eudaq {

  class FileWriterLCIOC : public FileWriter {
    public:
      FileWriterLCIOC(const std::string &);
      virtual void StartRun(unsigned);
    virtual void WriteEvent(const DetectorEvent &);
    virtual uint64_t FileBytes() const { return 0; }
      virtual ~FileWriterLCIOC();

    private:
      lcio::LCWriter *m_lcwriter; /// The lcio writer
      bool m_fileopened; /// We have to keep track whether a file is open ourselves
    // int m_runnumber; 
  };

  registerFileWriter(FileWriterLCIOC, "calice");

  FileWriterLCIOC::FileWriterLCIOC(const std::string & /*param*/)
    : m_lcwriter(lcio::LCFactory::getInstance()->createLCWriter()), // get an LCWriter from the factory
    m_fileopened(false)
  {
    //EUDAQ_DEBUG("Constructing FileWriterLCIOC(" + to_string(param) + ")");
  }

  void FileWriterLCIOC::StartRun(unsigned runnumber) {
    // close an open file
    if (m_fileopened) {
      m_lcwriter->close();
      m_fileopened = false;
    }

    // open a new file
    try {
      time_t  ltime;
      struct tm *Tm;
      ltime=time(NULL);
      Tm=localtime(&ltime);

      char file_timestamp[25];
      sprintf(file_timestamp,"__%02dp%02dp%02d__%02dp%02dp%02d",
	     Tm->tm_mday,
	     Tm->tm_mon+1,
	     Tm->tm_year+1900,
	     Tm->tm_hour,
	     Tm->tm_min,
	     Tm->tm_sec);

      m_lcwriter->open(FileNamer(m_filepattern).Set('X',file_timestamp ).Set('R', runnumber),
          lcio::LCIO::WRITE_NEW) ;
      m_fileopened=true;
    } catch(const lcio::IOException & e) {
      std::cout << e.what() << std::endl ;
    }
  }

  void FileWriterLCIOC::WriteEvent(const DetectorEvent & devent) {
    
    // changed 19052015 std::cout << "FileWriterLCIOC: Send data to network." << std::endl;
    //    g_dc->SendEvent(devent);
    
    if (devent.IsBORE()) {
      PluginManager::Initialize(devent);
      return;
    }
    
    auto lcevent =        std::unique_ptr<lcio::LCEvent>(PluginManager::ConvertToLCIO(devent));
    
    // only write non-empty events
    if (!lcevent->getCollectionNames()->empty()) {
      m_lcwriter->writeEvent(lcevent.get());
    }
  }
  
  FileWriterLCIOC::~FileWriterLCIOC() {
    // close an open file
    if (m_fileopened) {
      m_lcwriter->close();
    }
    
 }
 

}

#endif // USE_LCIO
