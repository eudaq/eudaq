#include "Processor.hh"
#include "DataSender.hh"

#include<sstream>

using namespace eudaq;

class EventSenderPS: public Processor{
public:
  EventSenderPS();
  void ProcessEvent(EventSPC ev) final override;
  void ProcessCommand(const std::string& cmd_name, const std::string& cmd_par) final override;
  void Connect(std::string type, std::string name, std::string server);

private:
  std::unique_ptr<DataSender> m_sender;
};

namespace{
  auto dummy0 = Factory<Processor>::Register<EventSenderPS>(eudaq::cstr2hash("EventSenderPS"));
}

EventSenderPS::EventSenderPS()
  :Processor("EventSenderPS"){
}

void EventSenderPS::ProcessEvent(EventSPC ev){
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

void EventSenderPS::ProcessCommand(const std::string& cmd_name, const std::string& cmd_par){
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
