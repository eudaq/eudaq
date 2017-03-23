#include <list>
#include "FileNamer.hh"
#include "FileSerializer.hh"
#include "FileReader.hh"
#include "Event.hh"
#include "Logger.hh"
#include "Configuration.hh"

namespace eudaq {
  class RawFileReader;
  namespace{
    auto dummy2 = Factory<FileReader>::Register<RawFileReader, std::string&>(cstr2hash("raw"));
    auto dummy3 = Factory<FileReader>::Register<RawFileReader, std::string&&>(cstr2hash("raw"));
  }

  class RawFileReader : public FileReader {
  public:
    RawFileReader(const std::string& filename);
    EventSPC GetNextEvent()override;
  private:
    std::unique_ptr<FileDeserializer> m_des;
    std::string m_filename;
  };

  RawFileReader::RawFileReader(const std::string& filename): m_filename(filename){
    
  }

  EventSPC RawFileReader::GetNextEvent(){
    if(!m_des){
      m_des.reset(new FileDeserializer(m_filename));
      //TODO: check sucess or fail
    }
    EventUP ev;
    uint32_t id = -1;
    m_des->PreRead(id);
    if(id != -1)
      ev = Factory<eudaq::Event>::Create<Deserializer&>(id, *m_des);
    return std::move(ev);
  }

}
