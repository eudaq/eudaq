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
    :m_is_listening(false),m_is_destructing(false), m_last_addr("tcp://0"){
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
      EUDAQ_INFO("DataReceiver:: Disconnected from " + to_string(*con));
      for (size_t i = 0; i < m_vt_con.size(); ++i){
	if (m_vt_con[i] == con){
	  m_vt_con.erase(m_vt_con.begin() + i);
	  std::unique_lock<std::mutex> lk(m_mx_qu_ev);
	  m_qu_ev.push(std::make_pair<EventSP, ConnectionSPC>(nullptr, con));
	  lk.unlock();
	  m_cv_not_empty.notify_all();
	  break;
	}
      }
      EUDAQ_THROW("DataReceiver:: Unrecognised Connection");
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
	EUDAQ_INFO("DataReceiver:: Connection from " + to_string(*con));
	m_vt_con.push_back(con);
	std::unique_lock<std::mutex> lk(m_mx_qu_ev);
	m_qu_ev.push(std::make_pair<EventSP, ConnectionSPC>(nullptr, con));
	lk.unlock();
	m_cv_not_empty.notify_all();
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
	  EUDAQ_WARN("DataReceiver:: Buffer of receving event is full.");
	}
	m_cv_not_empty.notify_all();
      }
      break;
    default:
      EUDAQ_WARN("DataReceiver:: Unknown TransportEvent");
    }
  }

  bool DataReceiver::AsyncReceiving(){
    while (m_is_listening){
      m_dataserver->Process(100000);
    }
    return 0;
  }

  bool DataReceiver::AsyncForwarding(){
    while(m_is_listening){
      std::unique_lock<std::mutex> lk(m_mx_qu_ev);
      if(m_qu_ev.empty()){
	while(m_cv_not_empty.wait_for(lk, std::chrono::seconds(1))
	      ==std::cv_status::timeout){
	  if(!m_is_listening){
	    return 0;
	  }
	}
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
	else{
	  OnDisconnect(con);
	}
      }
    }
    return 0;
  }
  
  std::string DataReceiver::Listen(const std::string &addr){
    std::string this_addr = m_last_addr;
    if(!addr.empty()){
      this_addr = addr;
    }
    if(m_dataserver){
      EUDAQ_THROW("DataReceiver:: Last server did not closed sucessfully");
    }

    auto dataserver = TransportServer::CreateServer(this_addr);
    dataserver->SetCallback(TransportCallback(this, &DataReceiver::DataHandler));
    
    m_last_addr = dataserver->ConnectionString();
    m_dataserver.reset(dataserver);
    m_is_listening = true;
    m_fut_async_rcv = std::async(std::launch::async, &DataReceiver::AsyncReceiving, this); 
    m_fut_async_fwd = std::async(std::launch::async, &DataReceiver::AsyncForwarding, this);
    return m_last_addr;
  }

  void DataReceiver::StopListen(){
    m_is_listening = false;
    auto tp_stop = std::chrono::steady_clock::now();    
    while( m_fut_async_rcv.valid() || m_fut_async_fwd.valid()){
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      if((std::chrono::steady_clock::now()-tp_stop) > std::chrono::seconds(10)){
	EUDAQ_THROW("DataReceiver:: Unable to stop the data receving/forwarding threads");
      }
    }
  }
  
  bool DataReceiver::Deamon(){
    while(!m_is_destructing){
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
      std::chrono::milliseconds t(10);
      std::unique_lock<std::mutex> lk_deamon(m_mx_deamon);
      if(m_is_listening){
	try{
	  if(m_fut_async_rcv.valid() &&
	     m_fut_async_rcv.wait_for(t)!=std::future_status::timeout){
	    m_fut_async_rcv.get();
	  }
	  if(m_fut_async_fwd.valid() &&
	     m_fut_async_fwd.wait_for(t)!=std::future_status::timeout){
	    m_fut_async_fwd.get();
	  }
	}
	catch(...){
	  EUDAQ_WARN("DataReceiver:: Connection execption at listening time");
	}
      }
      else{ //stop-start circle 
	try{
	  if(m_fut_async_rcv.valid()){
	    m_fut_async_rcv.get();
	  }
	  if(m_fut_async_fwd.valid()){
	    m_fut_async_fwd.get();
	  }
	  if(!m_qu_ev.empty()){
	    EUDAQ_WARN("DataReceiver:: Data buffer is not empty during the stopping");
	    m_qu_ev = std::queue<std::pair<EventSP, ConnectionSPC>>();
	  }
	  if(m_dataserver)
	    m_dataserver.reset();
	}
	catch(...){
	  EUDAQ_WARN("DataReceiver:: Connection execption from disconnetion");
	}
      }
    }    
    try{
      std::unique_lock<std::mutex> lk_deamon(m_mx_deamon);
      m_is_listening = false;
      if(m_fut_async_rcv.valid()){
	m_fut_async_rcv.get();
      }
      if(m_fut_async_fwd.valid()){
	m_fut_async_fwd.get();
      }
      if(!m_qu_ev.empty()){
	EUDAQ_WARN("DataReceiver:: Data buffer is not empty during the exiting");
	m_qu_ev = std::queue<std::pair<EventSP, ConnectionSPC>>();
      }
      if(m_dataserver)
	m_dataserver.reset();
    }
    catch(...){
      EUDAQ_WARN("DataReceiver:: Connection execption from disconnetion");
    }
  }
}
