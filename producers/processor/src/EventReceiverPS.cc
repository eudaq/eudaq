#include"EventReceiverPS.hh"
#include"TransportFactory.hh"

using namespace eudaq;

INIT_CLASS(Processor, EventReceiverPS, std::string );

EventReceiverPS::EventReceiverPS(std::string cmd)
  :Processor("EventReceiverPS", ""){
  *this<<cmd;
}

void EventReceiverPS::ProcessUserEvent(EVUP ev){
  std::cout<<">>>>PSID="<<GetID()<<"  PSType="<<GetType()<<"  EVType="<<ev->GetSubType()<<"  EVNum="<<ev->GetEventNumber()<<std::endl;
  ForwardEvent(std::move(ev));
}

void EventReceiverPS::ProduceEvent(){
  try {
    while (1) {
      m_server->Process(100000);
    }
  } catch (const std::exception &e) {
    std::cout << "Error: Uncaught exception: " << e.what() << "\n"
	      << "DataThread is dying..." << std::endl;
  } catch (...) {
    std::cout << "Error: Uncaught unrecognised exception: \n"
	      << "DataThread is dying..." << std::endl;
  }
}

void EventReceiverPS::SetServer(std::string listenaddress){
  m_server.reset(TransportFactory::CreateServer(listenaddress)); //tcp://portnum
  m_server->SetCallback(TransportCallback(this, &EventReceiverPS::DataHandler));
}


void EventReceiverPS::DataHandler(TransportEvent &ev) {

  std::cout<< ">>>>>>>>>>>>>>>>>>>>>datahandler"<<std::endl;
  switch (ev.etype) {
  case (TransportEvent::CONNECT):
    m_server->SendPacket("OK EUDAQ DATA DataCollector", ev.id, true);
    break;
  case (TransportEvent::DISCONNECT):
    //TODO:: OnDisconnect(ev.id);
    break;
  case (TransportEvent::RECEIVE):
    if (ev.id.GetState() == 0) { // waiting for identification
      do {
	size_t i0 = 0, i1 = ev.packet.find(' ');
	if (i1 == std::string::npos)
	  break;
	std::string part(ev.packet, i0, i1);
	if (part != "OK")
	  break;
	i0 = i1 + 1;
	i1 = ev.packet.find(' ', i0);
	if (i1 == std::string::npos)
	  break;
	part = std::string(ev.packet, i0, i1 - i0);
	if (part != "EUDAQ")
	  break;
	i0 = i1 + 1;
	i1 = ev.packet.find(' ', i0);
	if (i1 == std::string::npos)
	  break;
	part = std::string(ev.packet, i0, i1 - i0);
	if (part != "DATA")
	  break;
	i0 = i1 + 1;
	i1 = ev.packet.find(' ', i0);
	part = std::string(ev.packet, i0, i1 - i0);
	ev.id.SetType(part);
	i0 = i1 + 1;
	i1 = ev.packet.find(' ', i0);
	part = std::string(ev.packet, i0, i1 - i0);
	ev.id.SetName(part);
      } while (false);
      m_server->SendPacket("OK", ev.id, true);
      ev.id.SetState(1); // successfully identified
      //TODO:: OnConnect(ev.id);
    } else {
      std::cout<< ">>>>>>>>>> reveived"<<std::endl;;
      BufferSerializer ser(ev.packet.begin(), ev.packet.end());
      EVUP event(EventFactory::Create(ser));
      Processing(std::move(event));
    }
    break;
  default:
    std::cout << "Unknown:    " << ev.id << std::endl;
  }
}


void EventReceiverPS::ProcessUsrCmd(const std::string cmd_name, const std::string cmd_par){
  switch(cstr2hash(cmd_name.c_str())){
  case cstr2hash("SETSERVER"):{
    SetServer(cmd_par);
    break;
  }
  default:
    Processor::ProcessUsrCmd(cmd_name, cmd_par);
  }
}
