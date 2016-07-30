#include"EventFileReaderPS.hh"


using namespace eudaq;

INIT_CLASS(Processor, EventFileReaderPS, std::string );

EventFileReaderPS::EventFileReaderPS(std::string cmd)
  :Processor("EventFileReaderPS", ""){
  *this<<cmd;
}

void EventFileReaderPS::OpenFile(std::string filename){
  m_des.reset(new FileDeserializer(filename));  
}

void EventFileReaderPS::ProcessUserEvent(EVUP ev){
  std::cout<<">>>>PSID="<<GetID()<<"  PSType="<<GetType()<<"  EVType="<<ev->GetSubType()<<"  EVNum="<<ev->GetEventNumber()<<std::endl;
  ForwardEvent(std::move(ev));
}

void EventFileReaderPS::ProduceEvent(){
  if (!m_des) EUDAQ_THROW("m_des is not created!");
  while(1){
    EVUP ev(EventFactory::Create(*m_des.get()));//TODO: check if next event
    ProcessUserEvent(std::move(ev));
    // Processing(std::move(ev));
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
