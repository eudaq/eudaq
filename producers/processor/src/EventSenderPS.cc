#include"EventSenderPS.hh"

using namespace eudaq;

namespace{
  static RegisterDerived<Processor, typename std::string, EventSenderPS, uint32_t> reg_EXAMPLEPS("EventSenderPS");
}



EventSenderPS::EventSenderPS(uint32_t psid)
  :Processor("EventSenderPS", psid){
  InsertEventType(Event::str2id("_RAW"));
}

void EventSenderPS::ProcessUserEvent(EVUP ev){
  if (!m_sender) EUDAQ_THROW("DataSender is not created!");
  m_sender->SendEvent(*ev);
}

void EventSenderPS::Connect(std::string type, std::string name, std::string server){
  m_sender.reset(new DataSender(type, name));
  m_sender->Connect(server); //tcp://ipaddress:portnum
}
