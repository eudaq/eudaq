#include <list>
#include "RawFileReader.hh"
#include "FileNamer.hh"
#include "PluginManager.hh"
#include "Event.hh"
#include "Logger.hh"
#include "FileSerializer.hh"
#include "Configuration.hh"

namespace eudaq {
  namespace{
    auto dummy0 = Factory<FileReader>::Register<RawFileReader, std::string&>(cstr2hash("RawFileReader"));
    auto dummy1 = Factory<FileReader>::Register<RawFileReader, std::string&&>(cstr2hash("RawFileReader"));
  }
  
  RawFileReader::RawFileReader(const std::string & file, const std::string & filepattern)
    : FileReader(FileNamer(filepattern).Set('X', ".raw").SetReplace('R', file)),
    m_des(Filename()),m_ver(1){
    uint32_t id;
    m_des.PreRead(id);
    m_ev = Factory<eudaq::Event>::Create<Deserializer&>(id, m_des);
  }

  RawFileReader::RawFileReader(Parameter_ref param) :RawFileReader(param.Get(getKeyFileName(),""),param.Get(getKeyInputPattern(),""))
  {

  }

  RawFileReader::~RawFileReader() {

  }

  bool RawFileReader::NextEvent(size_t skip) {
    std::shared_ptr<eudaq::Event> ev = nullptr;


    bool result = m_des.ReadEvent(m_ver, ev, skip);
    if (ev) m_ev = ev;
    return result;
  }

  unsigned RawFileReader::RunNumber() const {
    return m_ev->GetRunNumber();
  }

  const Event & RawFileReader::GetEvent() const {
    return *m_ev;
  }

  const DetectorEvent & RawFileReader::GetDetectorEvent() const {
    return dynamic_cast<const DetectorEvent &>(*m_ev);
  }

  const StandardEvent & RawFileReader::GetStandardEvent() const {
    return dynamic_cast<const StandardEvent &>(*m_ev);
  }

  std::shared_ptr<eudaq::Event> RawFileReader::GetNextEvent(){

    if (!NextEvent()) {
      return nullptr;
    }

    return m_ev;


  }

  RegisterFileReader(RawFileReader, "raw");

}
