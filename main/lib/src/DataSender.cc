#include "eudaq/Event.hh"
#include "eudaq/AidaPacket.hh"
#include "eudaq/TransportClient.hh"
#include "eudaq/TransportFactory.hh"
#include "eudaq/Exception.hh"
#include "eudaq/BufferSerializer.hh"
#include "eudaq/Logger.hh"
#include "eudaq/DataSender.hh"

namespace eudaq {

  DataSender::DataSender(const std::string & type, const std::string & name)
    : m_type(type),
    m_name(name),
    m_dataclient(0),
    m_packetCounter(0) {}

  void DataSender::Connect(const std::string & server) {
    delete m_dataclient;
    m_dataclient = TransportFactory::CreateClient(server);

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
    if (part != "DataCollector" )
    	EUDAQ_THROW("Invalid response from DataCollector server, part=" + part);
    if (part != "DataCollector" ) EUDAQ_THROW("Invalid response from DataCollector server, part=" + part);

    m_dataclient->SendPacket("OK EUDAQ DATA " + m_type + " " + m_name);
    packet = "";
    if (!m_dataclient->ReceivePacket(&packet, 1000000)) EUDAQ_THROW("No response from DataCollector server");
    i1 = packet.find(' ');
    if (std::string(packet, 0, i1) != "OK") EUDAQ_THROW("Connection refused by DataCollector server: " + packet);
  }

  void DataSender::SendEvent(const Event &ev) {
    if (!m_dataclient) EUDAQ_THROW("Transport not connected error");
    //EUDAQ_DEBUG("Serializing event");
    BufferSerializer ser;
    ev.Serialize(ser);
    //EUDAQ_DEBUG("Sending event");
    m_packetCounter += 1;
    m_dataclient->SendPacket(ser);
    //EUDAQ_DEBUG("Sent event");
  }


  void DataSender::SendPacket( AidaPacket &packet ) {
	  m_packetCounter += 1;
	  if ( packet.GetPacketNumber() == 0 )
		  packet.SetPacketNumber( m_packetCounter );
//    EUDAQ_DEBUG("Serializing packet");
	  BufferSerializer ser;
	  packet.Serialize(ser);
//    EUDAQ_DEBUG("Sending packet");
	  m_dataclient->SendPacket(ser);
//    EUDAQ_DEBUG("Sent packet");
  }


  DataSender::~DataSender() {
    delete m_dataclient;
  }

}
