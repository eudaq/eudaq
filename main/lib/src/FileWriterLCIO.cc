#if USE_LCIO
#include "eudaq/FileNamer.hh"
#include "eudaq/PluginManager.hh"
#include "eudaq/FileWriter.hh"

#include "IO/ILCFactory.h"
#include "IMPL/LCEventImpl.h"
#include "IMPL/LCCollectionVec.h"
#include "IMPL/LCRunHeaderImpl.h"
#include "EVENT/LCIO.h"
#include "Exceptions.h"
#include "IMPL/LCTOOLS.h"
#include "IO/LCWriter.h"
#include "lcio.h"
#include <iostream>
#include <time.h>

#if USE_EUTELESCOPE
#include "EUTelEventImpl.h"
#include "EUTELESCOPE.h"
#endif //USE_EUTELESCOPE

namespace eudaq {

  class FileWriterLCIO : public FileWriter {
    public:
      FileWriterLCIO(const std::string &);
      virtual void StartRun(unsigned);
    virtual void WriteEvent(const DetectorEvent &);
    virtual uint64_t FileBytes() const { return 0; }
      virtual ~FileWriterLCIO();

    private:
      lcio::LCWriter *m_lcwriter; /// The lcio writer
      bool m_fileopened; /// We have to keep track whether a file is open ourselves
  };

  namespace {
    static RegisterFileWriter<FileWriterLCIO> reg("lcio");
  }

  FileWriterLCIO::FileWriterLCIO(const std::string & /*param*/)
    : m_lcwriter(lcio::LCFactory::getInstance()->createLCWriter()), // get an LCWriter from the factory
    m_fileopened(false)
  {
  }

  void FileWriterLCIO::StartRun(unsigned runnumber) {
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

  void FileWriterLCIO::WriteEvent(const DetectorEvent & devent) {
   
    if (devent.IsBORE()) {
      PluginManager::Initialize(devent);

#if USE_EUTELESCOPE

      auto x = std::unique_ptr<lcio::LCRunHeader>(
          PluginManager::GetLCRunHeader(devent));

      m_lcwriter->writeRunHeader(x.get());
#endif
      return;
    }else if (devent.IsEORE()) {
#if USE_EUTELESCOPE
      std::cout << "Found a EORE, so adding an EORE to the LCIO file as well"
                << std::endl;
      auto lcioEvent = std::unique_ptr<eutelescope::EUTelEventImpl>(
          new eutelescope::EUTelEventImpl);
      lcioEvent->setEventType(eutelescope::kEORE);
      lcioEvent->setTimeStamp(devent.GetTimestamp());
      lcioEvent->setRunNumber(devent.GetRunNumber());
      lcioEvent->setEventNumber(devent.GetEventNumber());

      // sent the lcioEvent to the processor manager for further processing
      m_lcwriter->writeEvent(lcioEvent.get());
#endif //USE_EUTELESCOPE
      return;
    }
    
    auto lcevent =        std::unique_ptr<lcio::LCEvent>(PluginManager::ConvertToLCIO(devent));
    
    // only write non-empty events
    if (!lcevent->getCollectionNames()->empty()) {
      m_lcwriter->writeEvent(lcevent.get());
    }
  }
  
  FileWriterLCIO::~FileWriterLCIO() {
    // close an open file
    if (m_fileopened) {
      m_lcwriter->close();
    }
    
 }
 

}

#endif // USE_LCIO
