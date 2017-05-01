#include "eudaq/FileSerializer.hh"
#include "eudaq/FileReader.hh"

class NativeFileReader : public eudaq::FileReader {
public:
  NativeFileReader(const std::string& filename);
  eudaq::EventSPC GetNextEvent()override;
private:
  std::unique_ptr<eudaq::FileDeserializer> m_des;
  std::string m_filename;
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::FileReader>::
    Register<NativeFileReader, std::string&>(eudaq::cstr2hash("native"));
  auto dummy1 = eudaq::Factory<eudaq::FileReader>::
    Register<NativeFileReader, std::string&&>(eudaq::cstr2hash("native"));
}

NativeFileReader::NativeFileReader(const std::string& filename)
  :m_filename(filename){    
}

eudaq::EventSPC NativeFileReader::GetNextEvent(){
  if(!m_des){
    m_des.reset(new eudaq::FileDeserializer(m_filename));
    //TODO: check sucess or fail
  }
  eudaq::EventUP ev;
  uint32_t id = -1;
  m_des->PreRead(id);
  if(id != -1)
    ev = eudaq::Factory<eudaq::Event>::
      Create<eudaq::Deserializer&>(id, *m_des);
  return std::move(ev);
}
