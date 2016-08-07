#include <list>
#include "FileReader.hh"
#include "FileNamer.hh"
#include "PluginManager.hh"
#include "Event.hh"
#include "Logger.hh"
#include "FileSerializer.hh"
#include "Configuration.hh"

namespace eudaq {


  FileReader::FileReader(const std::string & file, const std::string & filepattern)
    : baseFileReader(FileNamer(filepattern).Set('X', ".raw").SetReplace('R', file)),
    m_des(Filename()),m_ver(1){
    uint32_t id;
    m_des.read(id);
    m_ev = Factory<eudaq::Event>::Create<Deserializer&>(id, m_des);
  }

  FileReader::FileReader(Parameter_ref param) :FileReader(param.Get(getKeyFileName(),""),param.Get(getKeyInputPattern(),""))
  {

  }

  FileReader::~FileReader() {

  }

  bool FileReader::NextEvent(size_t skip) {
    std::shared_ptr<eudaq::Event> ev = nullptr;


    bool result = m_des.ReadEvent(m_ver, ev, skip);
    if (ev) m_ev = ev;
    return result;
  }

  unsigned FileReader::RunNumber() const {
    return m_ev->GetRunNumber();
  }

  const Event & FileReader::GetEvent() const {
    return *m_ev;
  }

  const DetectorEvent & FileReader::GetDetectorEvent() const {
    return dynamic_cast<const DetectorEvent &>(*m_ev);
  }

  const StandardEvent & FileReader::GetStandardEvent() const {
    return dynamic_cast<const StandardEvent &>(*m_ev);
  }

  std::shared_ptr<eudaq::Event> FileReader::GetNextEvent(){

    if (!NextEvent()) {
      return nullptr;
    }

    return m_ev;


  }





  RegisterFileReader(FileReader, "raw");

}
