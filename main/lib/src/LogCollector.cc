#include "eudaq/LogCollector.hh"
#include "eudaq/LogMessage.hh"
#include "eudaq/Logger.hh"
#include "eudaq/TransportFactory.hh"
#include "eudaq/BufferSerializer.hh"
#include <iostream>
#include <ostream>
#include <sstream>

namespace eudaq {

  namespace {

    void *LogCollector_thread(void *arg) {
      LogCollector *dc = static_cast<LogCollector *>(arg);
      dc->LogThread();
      return 0;
    }

  } // anonymous namespace

  LogCollector::LogCollector(const std::string &runcontrol,
                             const std::string &listenaddress)
      : CommandReceiver("LogCollector", "", runcontrol, false), m_done(false),
        m_listening(true),
        m_logserver(TransportFactory::CreateServer(listenaddress)),
        m_filename("../logs/" + Time::Current().Formatted("%Y-%m-%d.log")),
        m_file(m_filename.c_str(), std::ios_base::app) {
    if (!m_file.is_open())
      EUDAQ_THROWX(FileWriteException, "Unable to open log file (" +
                                           m_filename +
                                           ") is there a logs directory?");
    m_logserver->SetCallback(
        TransportCallback(this, &LogCollector::LogHandler));
    // pthread_attr_init(&m_threadattr);
    // pthread_create(&m_thread, &m_threadattr, LogCollector_thread, this);
    m_thread = std::unique_ptr<std::thread>(
        new std::thread(LogCollector_thread, this));
    std::cout << "###### listenaddress=" << m_logserver->ConnectionString()
              << std::endl
              << "       logfile=" << m_filename << std::endl;
    std::ostringstream os;
    os << "\n*** LogCollector started at " << Time::Current().Formatted()
       << " ***" << std::endl;
    m_file.write(os.str().c_str(), os.str().length());
    CommandReceiver::StartThread();
  }

  LogCollector::~LogCollector() {
    m_file << "*** LogCollector stopped at " << Time::Current().Formatted()
           << " ***" << std::endl;
    m_done = true;
    ///*if (m_thread)*/ pthread_join(m_thread, 0);
    m_thread->join();
    delete m_logserver;
  }

  void LogCollector::OnServer() {
    if (!m_logserver)
      EUDAQ_ERROR("Oops");
    // std::cout << "OnServer: " << m_logserver->ConnectionString() <<
    // std::endl;
    m_connectionstate.SetTag("_SERVER", m_logserver->ConnectionString());
  }

  void LogCollector::DoReceive(const LogMessage &ev) {
    std::ostringstream buf;
    ev.Write(buf);
    m_file.write(buf.str().c_str(), buf.str().length());
    m_file.flush();
    OnReceive(ev);
  }

  void LogCollector::LogHandler(TransportEvent &ev) {
    // std::cout << "LogHandler()" << std::endl;
    switch (ev.etype) {
    case (TransportEvent::CONNECT):
      // std::cout << "Connect:    " << ev.id << std::endl;
      if (m_listening) {
        m_logserver->SendPacket("OK EUDAQ LOG LogCollector", ev.id, true);
      } else {
        m_logserver->SendPacket("ERROR EUDAQ LOG Not accepting new connections",
                                ev.id, true);
        m_logserver->Close(ev.id);
      }
      break;
    case (TransportEvent::DISCONNECT):
      // std::cout << "Disconnect: " << ev.id << std::endl;
      OnDisconnect(ev.id);
      break;
    case (TransportEvent::RECEIVE):
      if (ev.id.GetState() == 0) { // waiting for identification
        // check packet
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
          if (part != "LOG")
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
        // std::cout << "client replied, sending OK" << std::endl;
        m_logserver->SendPacket("OK", ev.id, true);
        ev.id.SetState(1); // successfully identified
        OnConnect(ev.id);
      } else {
        // std::cout << "Receive:    " << ev.id << " " << ev.packet <<
        // std::endl;
        // for (size_t i = 0; i < 8 && i < ev.packet.size(); ++i) {
        //    std::cout << to_hex(ev.packet[i], 2) << ' ';
        //}
        // std::cout << ")" << std::endl;
        BufferSerializer ser(ev.packet.begin(), ev.packet.end());
        std::string src = ev.id.GetType();
        if (ev.id.GetName() != "")
          src += "." + ev.id.GetName();
        DoReceive(LogMessage(ser).SetSender(src));
      }
      break;
    default:
      std::cout << "Unknown:    " << ev.id << std::endl;
    }
  }

  void LogCollector::LogThread() {
    try {
      while (!m_done) {
        m_logserver->Process(100000);
      }
    } catch (const std::exception &e) {
      std::cout << "Error: Uncaught exception: " << e.what() << "\n"
                << "LogThread is dying..." << std::endl;
    } catch (...) {
      std::cout << "Error: Uncaught unrecognised exception: \n"
                << "LogThread is dying..." << std::endl;
    }
  }
}
