#include "eudaq/FileDeserializer.hh"
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
    if(!m_des)
      EUDAQ_THROW("unable to open file: " + m_filename);
  }
  eudaq::EventUP ev;
  uint32_t id;
  
  if(m_des->HasData()){
    m_des->PreRead(id);
    ev = eudaq::Factory<eudaq::Event>::
      Create<eudaq::Deserializer&>(id, *m_des);
    return std::move(ev);
  }  else  return nullptr;
  
}
