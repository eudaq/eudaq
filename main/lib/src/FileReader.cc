#include "eudaq/FileReader.hh"
#include "eudaq/FileNamer.hh"
#include "eudaq/PluginManager.hh"
#include "eudaq/Event.hh"
#include "eudaq/Logger.hh"
#include <list>
#include "eudaq/FileSerializer.hh"
#include "eudaq/Configuration.hh"

namespace eudaq {

/* The FileReader Class is specifically designed to accept settings from the .conf files and to read in the data as collected from the DataCollector. The FileReader class has two defining constants, the file and the file pattern.
*/
  FileReader::FileReader(const std::string &file,
                         const std::string &filepattern)
      : m_filename(
            FileNamer(filepattern).Set('X', ".raw").SetReplace('R', file)),
        m_des(m_filename), m_ev(EventFactory::Create(m_des)), m_ver(1) {

    // 		m_ev->SetTag("longTimeDelay",longTimeDelay);
    // 		m_ev->SetTag("NumberOfEvents",syncEvents);

    eudaq::Configuration conf(GetDetectorEvent().GetTag("CONFIG"));
    conf.SetSection("EventStruct");

    //       if (synctriggerid) {
    //
    // 	// saves this information in the BOR event. the DataConverterPlugins can
    // extract this information during initializing.
    // 		m_sync =std::make_shared<eudaq::SyncBase>(GetDetectorEvent());
    //
    //
    //       }
  }
  //   FileReader::FileReader(FileReader&& fileR):m_filename(fileR.Filename()),
  // 	  m_des(m_filename){
  //
  //
  //   }
  FileReader::~FileReader() {}

  bool FileReader::NextEvent(size_t skip) {
    std::shared_ptr<eudaq::Event> ev = nullptr;

    bool result = m_des.ReadEvent(m_ver, ev, skip);
    if (ev)
      m_ev = ev;
    return result;
  }

  unsigned FileReader::RunNumber() const { return m_ev->GetRunNumber(); }

  const Event &FileReader::GetEvent() const { return *m_ev; }

  const DetectorEvent &FileReader::GetDetectorEvent() const {
    return dynamic_cast<const DetectorEvent &>(*m_ev);
  }

  const StandardEvent &FileReader::GetStandardEvent() const {
    return dynamic_cast<const StandardEvent &>(*m_ev);
  }
}
