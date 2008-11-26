#include "eudaq/FileReader.hh"
#include "eudaq/FileNamer.hh"

namespace eudaq {

  FileReader::FileReader(const std::string & file, const std::string & filepattern)
    : m_des(FileNamer(filepattern).Set('X', ".raw").SetReplace('R', file)),
      m_ev(EventFactory::Create(m_des)) {
  }

  bool FileReader::NextEvent() {
    if (!m_des.HasData()) {
      return false;
    }
    m_ev = eudaq::EventFactory::Create(m_des);
    return true;
  }

  unsigned FileReader::RunNumber() const {
    return m_ev->GetRunNumber();
  }

  const DetectorEvent & FileReader::Event() const {
    return dynamic_cast<const DetectorEvent &>(*m_ev);
  }

}
