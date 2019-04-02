#include "eudaq/LogSender.hh"
#include "eudaq/LogMessage.hh"
#include "eudaq/TransportClient.hh"
#include "eudaq/Exception.hh"
#include "eudaq/BufferSerializer.hh"

namespace eudaq {

  LogSender::LogSender()
      : m_logclient(0), m_errlevel(Status::LVL_DEBUG),
        m_shownotconnected(false) {}

  void LogSender::Connect(const std::string &type, const std::string &name,
                          const std::string &server) {
      std::cout<<"-+-+-+-+-+-+-+-+-+-+-+-+-+-+-" << type <<", "<< name <<", "<< server <<std::endl;
    std::lock_guard<std::recursive_mutex> lk(m_mutex);
    if (isConnected) {
      return;
    }
    isConnected = true;
    m_shownotconnected = true;
    delete m_logclient;
    m_name = type + " " + name;
    m_logclient = TransportClient::CreateClient(server);

    std::string packet;
    if (!m_logclient->ReceivePacket(&packet, 1000000))
      EUDAQ_THROW("No response from LogCollector server");
    size_t i0 = 0, i1 = packet.find(' ');
    if (i1 == std::string::npos)
      EUDAQ_THROW("Invalid response from LogCollector server");
    std::string part(packet, i0, i1);
    if (part != "OK")
      EUDAQ_THROW("Invalid response from LogCollector server: " + packet);
    i0 = i1 + 1;
    i1 = packet.find(' ', i0);
    if (i1 == std::string::npos)
      EUDAQ_THROW("Invalid response from LogCollector server");
    part = std::string(packet, i0, i1 - i0);
    if (part != "EUDAQ")
      EUDAQ_THROW("Invalid response from LogCollector server, part=" + part);
    i0 = i1 + 1;
    i1 = packet.find(' ', i0);
    if (i1 == std::string::npos)
      EUDAQ_THROW("Invalid response from LogCollector server");
    part = std::string(packet, i0, i1 - i0);
    if (part != "LOG")
      EUDAQ_THROW("Invalid response from LogCollector server, part=" + part);
    i0 = i1 + 1;
    i1 = packet.find(' ', i0);
    part = std::string(packet, i0, i1 - i0);
    if (part != "LogCollector")
      EUDAQ_THROW("Invalid response from LogCollector server, part=" + part);

    m_logclient->SendPacket("OK EUDAQ LOG " + m_name);
    packet = "";
    if (!m_logclient->ReceivePacket(&packet, 1000000))
      EUDAQ_THROW("No response from LogCollector server");
    i1 = packet.find(' ');
    if (std::string(packet, 0, i1) != "OK")
      EUDAQ_THROW("Connection refused by LogCollector server: " + packet);
  }

  void LogSender::Disconnect() {
    std::lock_guard<std::recursive_mutex> lk(m_mutex);
    delete m_logclient;
    isConnected = false;
  }

  void LogSender::SendLogMessage(const LogMessage &msg) {

    SendLogMessage(msg, std::cout, std::cerr);
  }

  void LogSender::SendLogMessage(const LogMessage &msg, std::ostream &out,
                                 std::ostream &error_out) {
    std::cout <<"Log message sent: " <<msg.GetMessage()<<std::endl;
    std::lock_guard<std::recursive_mutex> lk(m_mutex);
    if (msg.GetLevel() >= m_level) {
      if (msg.GetLevel() >= m_errlevel) {
        if (m_name != "")
          error_out << "[" << m_name << "] ";
        error_out << msg << std::endl;
      } else {
        if (m_name != "")
          out << "[" << m_name << "] ";
        out << msg << std::endl;
      }
    }

    if (!m_logclient) {
        std::cout << "no log client found:" <<std::endl;

      if (m_shownotconnected)
        error_out << "### Log message triggered but Logger not connected ###\n";
    } else {
      BufferSerializer ser;
      msg.Serialize(ser);
      try {
        m_logclient->SendPacket(ser);
      } catch (const eudaq::Exception &e) {
        error_out << "Caught exception trying to log message '" << msg
                  << "': " << e.what() << std::endl;
        error_out << " -> will delete LogClient" << std::endl;
        delete m_logclient;
        m_logclient = 0;
      } catch (...) {
        error_out << "Caught exception trying to log message '" << msg << "'! "
                  << std::endl;
        error_out << " -> will delete LogClient" << std::endl;
        delete m_logclient;
        m_logclient = 0;
      }
      std::cout << "serialized:" <<std::endl;
    }
  }

  LogSender::~LogSender() { delete m_logclient; }
}
