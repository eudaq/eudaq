#include"Processor.hh"
#include"FileSerializer.hh"

using namespace eudaq;

class EventFileReaderPS:public Processor{
public:
  EventFileReaderPS(std::string cmd);
  ~EventFileReaderPS(){};
  void ProcessUserEvent(EventSPC ev) final override;
  void OpenFile(std::string filename);
  void ProduceEvent() final override;
  void ProcessUsrCmd(const std::string cmd_name, const std::string cmd_par) final override;
private:
  std::unique_ptr<FileDeserializer> m_des;    
};

namespace{
  auto dummy0 = Factory<Processor>::Register<EventFileReaderPS, std::string&>(eudaq::cstr2hash("EventFileReaderPS"));
  auto dummy1 = Factory<Processor>::Register<EventFileReaderPS, std::string&&>(eudaq::cstr2hash("EventFileReaderPS"));
}

EventFileReaderPS::EventFileReaderPS(std::string cmd)
  :Processor("EventFileReaderPS", ""){
  ProcessCmd(cmd);
}

void EventFileReaderPS::OpenFile(std::string filename){
  m_des.reset(new FileDeserializer(filename));  
}

void EventFileReaderPS::ProcessUserEvent(EventSPC ev){
  std::cout<<">>>>PSID="<<GetID()<<"  PSType="<<GetType()<<"  EVType="<<ev->GetSubType()<<"  EVNum="<<ev->GetEventNumber()<<std::endl;
  ForwardEvent(ev);
}

void EventFileReaderPS::ProduceEvent(){
  if (!m_des) EUDAQ_THROW("m_des is not created!");
  // while(1){
  for(uint32_t i=0; i<10; i++ ){
    uint32_t id;
    m_des->PreRead(id);
    EventSP ev = Factory<Event>::MakeShared<Deserializer&>(id, *m_des.get());
    // ProcessUserEvent(ev);
    Processing(ev);
  }
}

void EventFileReaderPS::ProcessUsrCmd(const std::string cmd_name, const std::string cmd_par){
  switch(cstr2hash(cmd_name.c_str())){
  case cstr2hash("FILE"):
    OpenFile(cmd_par);
    break;
  default:
    Processor::ProcessUsrCmd(cmd_name, cmd_par);
  }
}
