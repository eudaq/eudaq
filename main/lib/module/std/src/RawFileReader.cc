#include <list>
#include "FileNamer.hh"
#include "FileSerializer.hh"
#include "FileReader.hh"
#include "PluginManager.hh"
#include "Event.hh"
#include "Logger.hh"
#include "Configuration.hh"

namespace eudaq {
  class RawFileReader;
  namespace{
    auto dummy0 = Factory<FileReader>::Register<RawFileReader, std::string&>(cstr2hash("RawFileReader"));
    auto dummy1 = Factory<FileReader>::Register<RawFileReader, std::string&&>(cstr2hash("RawFileReader"));
    auto dummy2 = Factory<FileReader>::Register<RawFileReader, std::string&>(cstr2hash("raw"));
    auto dummy3 = Factory<FileReader>::Register<RawFileReader, std::string&&>(cstr2hash("raw"));
  }

  class DLLEXPORT RawFileReader : public FileReader {
  public:
    RawFileReader(const std::string& filename);
    ~RawFileReader(){};
    
    unsigned RunNumber() const final override;
    bool NextEvent(size_t skip = 0)final override;
    EventSPC GetNextEvent()final override;
    void Interrupt() final override { m_des.Interrupt(); }

  private:
    FileDeserializer m_des;
    EventSP m_ev;
    unsigned m_ver;
  };

  RawFileReader::RawFileReader(const std::string& file):FileReader(file), m_des(Filename()),m_ver(1){
    uint32_t id;
    m_des.PreRead(id);
    m_ev = Factory<eudaq::Event>::Create<Deserializer&>(id, m_des);
  }

  bool RawFileReader::NextEvent(size_t skip) {
    EventSP ev = nullptr;
    bool result = m_des.ReadEvent(m_ver, ev, skip);
    if (ev) m_ev = ev;
    return result;
  }

  unsigned RawFileReader::RunNumber() const {
    return m_ev->GetRunNumber();
  }


  EventSPC RawFileReader::GetNextEvent(){
    if (!NextEvent()) {
      return nullptr;
    }
    return m_ev;

  }

}
