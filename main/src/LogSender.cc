#include "eudaq/LogSender.hh"
#include "eudaq/LogMessage.hh"
#include "eudaq/TransportFactory.hh"
#include "eudaq/Exception.hh"
#include "eudaq/BufferSerializer.hh"

namespace eudaq {

  LogSender::LogSender() :
    m_transport(0), m_shownotconnected(false) {}

  void LogSender::Connect(const std::string & type, const std::string & name, const std::string & server) {
    m_shownotconnected = true;
    delete m_transport;
    m_name = type + " " + name;
    m_transport = TransportFactory::CreateClient(server);

    std::string packet;
    if (!m_transport->ReceivePacket(&packet, 1000000)) EUDAQ_THROW("No response from LogCollector server");
    size_t i0 = 0, i1 = packet.find(' ');
    if (i1 == std::string::npos) EUDAQ_THROW("Invalid response from LogCollector server");
    std::string part(packet, i0, i1);
    if (part != "OK") EUDAQ_THROW("Invalid response from LogCollector server: " + packet);
    i0 = i1+1;
    i1 = packet.find(' ', i0);
    if (i1 == std::string::npos) EUDAQ_THROW("Invalid response from LogCollector server");
    part = std::string(packet, i0, i1-i0);
    if (part != "EUDAQ") EUDAQ_THROW("Invalid response from LogCollector server, part=" + part);
    i0 = i1+1;
    i1 = packet.find(' ', i0);
    if (i1 == std::string::npos) EUDAQ_THROW("Invalid response from LogCollector server");
    part = std::string(packet, i0, i1-i0);
    if (part != "LOG") EUDAQ_THROW("Invalid response from LogCollector server, part=" + part);
    i0 = i1+1;
    i1 = packet.find(' ', i0);
    part = std::string(packet, i0, i1-i0);
    if (part != "LogCollector" ) EUDAQ_THROW("Invalid response from LogCollector server, part=" + part);

    m_transport->SendPacket("OK EUDAQ LOG " + m_name);
    packet = "";
    if (!m_transport->ReceivePacket(&packet, 1000000)) EUDAQ_THROW("No response from LogCollector server");
    i1 = packet.find(' ');
    if (std::string(packet, 0, i1) != "OK") EUDAQ_THROW("Connection refused by LogCollector server: " + packet);
  }

  void LogSender::SendLogMessage(const LogMessage & msg, bool show) {
    //std::cout << "Sending: " << msg << std::endl;
    if (!m_transport) {
      if (m_shownotconnected) std::cerr << "### Logger not connected ###\n";
    } else {
      BufferSerializer ser;
      msg.Serialize(ser);
      m_transport->SendPacket(ser);
    }
    if (msg.GetLevel() >= m_level && show) {
      if (m_name != "") std::cerr << "[" << m_name << "] ";
      std::cerr << msg << std::endl;
    }
  }

  LogSender::~LogSender() {
    delete m_transport;
  }

}
