#include"Processor.hh"
#include"FileSerializer.hh"

using namespace eudaq;

class EventFileReaderPS:public Processor{
public:
  EventFileReaderPS();
  ~EventFileReaderPS() {};
  void ProduceEvent() final override;
  void ProcessCommand(const std::string& cmd_name, const std::string& cmd_par) final override;
  void OpenFile(const std::string& filename);
private:
  std::unique_ptr<FileDeserializer> m_des;    
};

namespace{
  auto dummy0 = Factory<Processor>::Register<EventFileReaderPS>(eudaq::cstr2hash("EventFileReaderPS"));
}

EventFileReaderPS::EventFileReaderPS()
  :Processor("EventFileReaderPS"){
}

void EventFileReaderPS::OpenFile(const std::string& filename){
  m_des.reset(new FileDeserializer(filename));  
}

void EventFileReaderPS::ProduceEvent(){
  if (!m_des) EUDAQ_THROW("m_des is not created!");
  // while(1){
  for(uint32_t i=0; i<10; i++ ){
    uint32_t id;
    m_des->PreRead(id);
    EventSP ev = Factory<Event>::MakeShared<Deserializer&>(id, *m_des.get());
    RegisterEvent(std::move(ev));
  }
}

void EventFileReaderPS::ProcessCommand(const std::string& cmd_name, const std::string& cmd_par){
  switch(cstr2hash(cmd_name.c_str())){
  case cstr2hash("FILE"):
    OpenFile(cmd_par);
    break;
  default:
    break;
  }
}
