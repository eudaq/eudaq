#include "eudaq/DataReceiver.hh"
#include "eudaq/TransportServer.hh"
#include "eudaq/BufferSerializer.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Utils.hh"
#include <iostream>
#include <ostream>
#include <ctime>
#include <iomanip>
namespace eudaq {
  
  DataReceiver::DataReceiver()
    :m_is_listening(false),m_is_destructing(false){
    m_fut_deamon = std::async(std::launch::async, &DataReceiver::Deamon, this); 
  }

  DataReceiver::~DataReceiver(){
    m_is_destructing = true;
    if(m_fut_deamon.valid()){
      m_fut_deamon.get();
    }
  }

  void DataReceiver::OnConnect(ConnectionSPC id){
  }
  
  void DataReceiver::OnDisconnect(ConnectionSPC id){
  }
  
  void DataReceiver::OnReceive(ConnectionSPC id, EventSP ev){
  }
  
  void DataReceiver::DataHandler(TransportEvent &ev) {
    auto con = ev.id;
    switch (ev.etype) {
    case (TransportEvent::CONNECT):
      m_dataserver->SendPacket("OK EUDAQ DATA DataReceiver", *con, true);
      break;
    case (TransportEvent::DISCONNECT):
      con->SetState(0);
      EUDAQ_INFO("Disconnected: " + to_string(*con));
      for (size_t i = 0; i < m_vt_con.size(); ++i){
	if (m_vt_con[i] == con){
	  m_vt_con.erase(m_vt_con.begin() + i);
	  std::unique_lock<std::mutex> lk(m_mx_qu_ev);
	  m_qu_ev.push(std::make_pair<EventSP, ConnectionSPC>(nullptr, con));
	}
      }
      EUDAQ_THROW("Unrecognised connection id");
      break;
    case (TransportEvent::RECEIVE):
      if (con->GetState() == 0) { //unidentified connection
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
          con->SetType(part);
          i0 = i1 + 1;
          i1 = ev.packet.find(' ', i0);
          part = std::string(ev.packet, i0, i1 - i0);
          con->SetName(part);
        } while (false);
        m_dataserver->SendPacket("OK", *con, true);
        con->SetState(1); // successfully identified
	EUDAQ_INFO("Connection from " + to_string(*con));
	m_vt_con.push_back(con);
	std::unique_lock<std::mutex> lk(m_mx_qu_ev);
	m_qu_ev.push(std::make_pair<EventSP, ConnectionSPC>(nullptr, con));
      }
      else{ //identified connection  
	BufferSerializer ser(ev.packet.begin(), ev.packet.end());
	uint32_t id;
	ser.PreRead(id);
	auto ev_con = std::make_pair<EventSP, ConnectionSPC>
	  (Factory<Event>::MakeUnique<Deserializer&>(id, ser), con);
	std::unique_lock<std::mutex> lk(m_mx_qu_ev);
	m_qu_ev.push(ev_con);
	if(m_qu_ev.size() > 50000){
	  m_qu_ev.pop();
	  EUDAQ_WARN("Buffer of receving event is full.");
	}
	lk.unlock();
	m_cv_not_empty.notify_all();
      }
      break;
    default:
      std::cout << "Unknown:    " << *con << std::endl;
    }
  }

  bool DataReceiver::AsyncReceiving(){
    while (m_is_listening) {
      m_dataserver->Process(100000);
    }
    return 0;
  }

  bool DataReceiver::AsyncForwarding(){
    while(m_is_listening){
      std::unique_lock<std::mutex> lk(m_mx_qu_ev);
      if(m_qu_ev.empty()){
	m_cv_not_empty.wait(lk);
      }
      auto ev = m_qu_ev.front().first;
      auto con = m_qu_ev.front().second;
      m_qu_ev.pop();
      lk.unlock();
      if(ev){
	OnReceive(con, ev);
      }
      else{
	if(con->GetState())
	  OnConnect(con);
	else
	  OnDisconnect(con);	  
      }
    }
    return 0;
  }
  
  std::string DataReceiver::Listen(const std::string &addr){
    std::string listen = "tcp://0";
    if(addr.empty()){
      listen = addr;
    }

    std::unique_lock<std::mutex> lk_deamon(m_mx_deamon);
    m_is_listening = false;
    try{
      if(m_fut_async_rcv.valid()){
	m_fut_async_rcv.get();
      }
      if(m_fut_async_fwd.valid()){
	m_fut_async_fwd.get();
      }
    }
    catch(...){
      EUDAQ_WARN("connection execption from disconnetion");
    }
    
    std::unique_lock<std::mutex> lk(m_mx_qu_ev);    
    m_qu_ev = std::queue<std::pair<EventSP, ConnectionSPC>>();    
    lk.unlock();

    m_dataserver.reset(TransportServer::CreateServer(listen));
    m_dataserver->SetCallback(TransportCallback(this, &DataReceiver::DataHandler));

    m_is_listening = true;
    m_fut_async_rcv = std::async(std::launch::async, &DataReceiver::AsyncReceiving, this); 
    m_fut_async_fwd = std::async(std::launch::async, &DataReceiver::AsyncForwarding, this);
    return m_dataserver->ConnectionString();
  }

  bool DataReceiver::Deamon(){
    while(!m_is_destructing){
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
      std::chrono::milliseconds t(10);
      std::unique_lock<std::mutex> lk_deamon(m_mx_deamon);
      try{
	if(m_fut_async_rcv.valid()){
	  m_fut_async_rcv.wait_for(t);
	}
	if(m_fut_async_fwd.valid()){
	  m_fut_async_fwd.wait_for(t);
	}
      }
      catch(...){
	EUDAQ_WARN("connection execption from disconnetion");
      }
    }
  }
  
}
