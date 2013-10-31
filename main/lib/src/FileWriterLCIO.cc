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

namespace eudaq {

  class FileWriterLCIO : public FileWriter {
    public:
      FileWriterLCIO(const std::string &);
      virtual void StartRun(unsigned);
      virtual void WriteEvent(const DetectorEvent &);
      virtual unsigned long long FileBytes() const { return 0; }
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
    //EUDAQ_DEBUG("Constructing FileWriterLCIO(" + to_string(param) + ")");
  }

  void FileWriterLCIO::StartRun(unsigned runnumber) {
    // close an open file
    if (m_fileopened) {
      m_lcwriter->close();
      m_fileopened = false;
    }

    // open a new file
    try {
      m_lcwriter->open(FileNamer(m_filepattern).Set('X', ".slcio").Set('R', runnumber),
          lcio::LCIO::WRITE_NEW) ;
      m_fileopened=true;
    } catch(const lcio::IOException & e) {
      std::cout << e.what() << std::endl ;
      ///FIXME Error message to run control and logger
    }
  }

  void FileWriterLCIO::WriteEvent(const DetectorEvent & devent) {
    if (devent.IsBORE()) {
      PluginManager::Initialize(devent);
      return;
    }
    std::cout << "EUDAQ_DEBUG: FileWriterLCIO::WriteEvent() processing event "
      <<  devent.GetRunNumber () <<"." << devent.GetEventNumber () << std::endl;

    lcio::LCEvent * lcevent = PluginManager::ConvertToLCIO(devent);

    // only write non-empty events
    if (!lcevent->getCollectionNames()->empty()) {
      // std::cout << " FileWriterLCIO::WriteEvent() : doing the actual writing : " <<std::flush;
      m_lcwriter->writeEvent(lcevent);
      // std::cout << " done" <<std::endl;
    }
    delete lcevent;
  }

  FileWriterLCIO::~FileWriterLCIO() {
    // close an open file
    if (m_fileopened) {
      m_lcwriter->close();
    }
  }

}

#endif // USE_LCIO
