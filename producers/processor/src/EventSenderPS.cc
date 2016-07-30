#include"EventSenderPS.hh"

#include<sstream>

using namespace eudaq;

INIT_CLASS(Processor, EventSenderPS, std::string );

EventSenderPS::EventSenderPS(std::string cmd)
  :Processor("EventSenderPS", ""){
  *this<<cmd;
}

void EventSenderPS::ProcessUserEvent(EVUP ev){
  std::cout<<">>>>PSID="<<GetID()<<"  PSType="<<GetType()<<"  EVType="<<ev->GetSubType()<<"  EVNum="<<ev->GetEventNumber()<<std::endl;
  if (!m_sender){
    EUDAQ_THROW("DataSender is not created!");
  }
  // else
  m_sender->SendEvent(*ev);
}

void EventSenderPS::Connect(std::string type, std::string name, std::string server){
  m_sender.reset(new DataSender(type, name));
  m_sender->Connect(server); //tcp://ipaddress:portnum
}

void EventSenderPS::ProcessUsrCmd(const std::string cmd_name, const std::string cmd_par){
  switch(cstr2hash(cmd_name.c_str())){
  case cstr2hash("CONNECT"):{
    std::stringstream ss(cmd_par);
    std::string type_str;
    std::string name_str;
    std::string server_str;
    getline(ss, type_str, ',');
    getline(ss, name_str, ',');
    getline(ss, server_str, ',');
    Connect(type_str, name_str, server_str);
    break;
  }
  default:
    std::cout<<"unkonw user cmd"<<std::endl;
  }
}
