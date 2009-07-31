#include "eudaq/FileReader.hh"
#include "eudaq/FileNamer.hh"
#include "eudaq/PluginManager.hh"
#include "eudaq/Event.hh"
#include "eudaq/Logger.hh"

namespace eudaq {

  FileReader::FileReader(const std::string & file, const std::string & filepattern)
    : m_filename(FileNamer(filepattern).Set('X', ".raw").SetReplace('R', file)),
      m_des(m_filename),
      m_ev(EventFactory::Create(m_des)),
      m_ver(1) {
    //unsigned versiontag = m_des.peek<unsigned>();
    //if (versiontag == Event::str2id("VER2")) {
    //  m_ver = 2;
    //  m_des.read(versiontag);
    //} else if (versiontag != Event::str2id("_DET")) {
    //  EUDAQ_WARN("Unrecognised native file (tag=" + Event::id2str(versiontag) + "), assuming version 1");
    //}
    //EUDAQ_INFO("FileReader, version = " + to_string(m_ver));
    //NextEvent();
  }

  bool FileReader::NextEvent(size_t skip) {
    if (!m_des.HasData()) {
      return false;
    }
    if (m_ver < 2) {
      for (size_t i = 0; i <= skip; ++i) {
        if (!m_des.HasData()) break;
        m_ev = EventFactory::Create(m_des);
      }
      return true;
    } else {
      BufferSerializer buf;
      for (size_t i = 0; i <= skip; ++i) {
        if (!m_des.HasData()) break;
        m_des.read(buf);
      }
      m_ev = eudaq::EventFactory::Create(buf);
      return true;
    }
  }

  unsigned FileReader::RunNumber() const {
    return m_ev->GetRunNumber();
  }

  const DetectorEvent & FileReader::Event() const {
    return dynamic_cast<const DetectorEvent &>(*m_ev);
  }

//   const StandardEvent & FileReader::GetStandardEvent() const {
//     if (!m_sev) {
//       counted_ptr<StandardEvent> sevent(new StandardEvent);
//       const DetectorEvent & dev = GetDetectorEvent();
//       for (size_t i = 0; i < dev.NumEvents(); ++i) {
//         const eudaq::Event * subevent = dev.GetEvent(i);

//         try {
//           const DataConverterPlugin * converterplugin = PluginManager::GetInstance().GetPlugin(subevent->GetType());
//           converterplugin->GetStandardSubEvent(*sevent, *subevent);
//           //std::fprintf(m_file, "Event %d %d\n", devent.GetEventNumber(), standardevent->m_x.size());
//         } catch(eudaq::Exception & e) {
//           //std::cout <<  e.what() << std::endl;
//           std::cout <<  "FileWriterText::WriteEvent(): Ignoring event type "
//                     <<  subevent->GetType() << std::endl;
//           continue;
//         }
//       }
//       m_sev = sevent;
//     }
//     return *m_sev;
//   }

}
