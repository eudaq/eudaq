#include "eudaq/Event.hh"
#include "eudaq/TransportClient.hh"
#include "eudaq/Exception.hh"
#include "eudaq/BufferSerializer.hh"
#include "eudaq/Logger.hh"
#include "eudaq/DataSender.hh"

namespace eudaq {

  DataSender::DataSender(const std::string & type, const std::string & name)
    : m_type(type),
    m_name(name),
    m_packetCounter(0) {}


  DataSender::~DataSender(){
    if(m_fut_async.valid()){
      m_fut_async.get();
    }
  }
  
  void DataSender::Connect(const std::string & server) {
    m_is_connected = false;
    try{
      if(m_fut_async.valid()){
	m_fut_async.get();
	//previous connection is closed.
      }
    }
    catch(...){
      EUDAQ_WARN("connection execption from disconnetion");
    }
    
    std::unique_lock<std::mutex> lk(m_mx_qu_ev);    
    m_qu_ev = std::queue<EventSPC>();    
    lk.unlock();
    m_dataclient.reset(TransportClient::CreateClient(server));
    std::string packet;
    if (!m_dataclient->ReceivePacket(&packet, 1000000)) EUDAQ_THROW("No response from DataCollector server");
    size_t i0 = 0, i1 = packet.find(' ');
    if (i1 == std::string::npos) EUDAQ_THROW("Invalid response from DataCollector server");
    std::string part(packet, i0, i1);
    if (part != "OK") EUDAQ_THROW("Invalid response from DataCollector server: " + packet);
    i0 = i1+1;
    i1 = packet.find(' ', i0);
    if (i1 == std::string::npos) EUDAQ_THROW("Invalid response from DataCollector server");
    part = std::string(packet, i0, i1-i0);
    if (part != "EUDAQ") EUDAQ_THROW("Invalid response from DataCollector server, part=" + part);
    i0 = i1+1;
    i1 = packet.find(' ', i0);
    if (i1 == std::string::npos) EUDAQ_THROW("Invalid response from DataCollector server");
    part = std::string(packet, i0, i1-i0);
    if (part != "DATA") EUDAQ_THROW("Invalid response from DataCollector server, part=" + part);
    i0 = i1+1;
    i1 = packet.find(' ', i0);
    part = std::string(packet, i0, i1-i0);
    if (part != "DataReceiver" && part != "DataCollector" && part != "Monitor" )
      EUDAQ_THROW("Invalid response from DataCollector server, part=" + part);

    m_dataclient->SendPacket("OK EUDAQ DATA " + m_type + " " + m_name);
    packet = "";
    if (!m_dataclient->ReceivePacket(&packet, 1000000)) EUDAQ_THROW("No response from DataCollector server");
    i1 = packet.find(' ');
    if (std::string(packet, 0, i1) != "OK") EUDAQ_THROW("Connection refused by DataCollector server: " + packet);

    m_is_connected = true;
    m_fut_async = std::async(std::launch::async, &DataSender::AsyncSending, this);
  }

  void DataSender::SendEvent(EventSPC ev){
    if (!m_dataclient)
      EUDAQ_THROW("Transport not connected error");
    std::unique_lock<std::mutex> lk(m_mx_qu_ev);
    m_qu_ev.push(ev);
    if(m_qu_ev.size() > 50000){
      m_qu_ev.pop();
      EUDAQ_WARN("Buffer of sending event is full.");
    }
    lk.unlock();
    m_cv_not_empty.notify_all();
  }

  bool DataSender::AsyncSending(){
    while(m_is_connected){//TODO: 
      std::unique_lock<std::mutex> lk(m_mx_qu_ev);
      if(m_qu_ev.empty()){
	m_cv_not_empty.wait(lk);
      }
      auto ev = m_qu_ev.front();
      m_qu_ev.pop();
      lk.unlock();
      BufferSerializer ser;
      ev->Serialize(ser);
      m_packetCounter += 1;
      m_dataclient->SendPacket(ser);
    }
  }
  
}
