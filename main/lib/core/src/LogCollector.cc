#include "eudaq/LogCollector.hh"
#include "eudaq/LogMessage.hh"
#include "eudaq/Logger.hh"
#include "eudaq/TransportFactory.hh"
#include "eudaq/BufferSerializer.hh"
#include <iostream>
#include <ostream>
#include <sstream>

namespace eudaq {

  template class DLLEXPORT Factory<LogCollector>;
  template DLLEXPORT
  std::map<uint32_t, typename Factory<LogCollector>::UP_BASE (*)
	   (const std::string&, const std::string&,
	    const std::string&, const int&)>&
  Factory<LogCollector>::Instance<const std::string&, const std::string&,
				  const std::string&, const int&>();//TODO: check const int&
  
  LogCollector::LogCollector(const std::string &runcontrol,
                             const std::string &listenaddress,
			     const std::string &logdirectory)
      : CommandReceiver("LogCollector", "", runcontrol), m_done(false),
        m_listening(true),
        m_logserver(TransportFactory::CreateServer(listenaddress)),
        m_filename(logdirectory + "/" + Time::Current().Formatted("%Y-%m-%d.log")),
        m_file(m_filename.c_str(), std::ios_base::app) {
    if (!m_file.is_open())
      EUDAQ_THROWX(FileWriteException, "Unable to open log file (" +
		   m_filename +
		   ") is there a logs directory?");
    else std::cout << "LogCollector opened \"" << m_filename << "\" for logging." << std::endl;
    m_logserver->SetCallback(
        TransportCallback(this, &LogCollector::LogHandler));
    m_thread = std::thread(&LogCollector::LogThread, this);
    std::cout << "###### listenaddress=" << m_logserver->ConnectionString()
              << std::endl
              << "       logfile=" << m_filename << std::endl;
    std::ostringstream os;
    os << "\n*** LogCollector started at " << Time::Current().Formatted()
       << " ***" << std::endl;
    m_file.write(os.str().c_str(), os.str().length());

  }

  LogCollector::~LogCollector() {
    m_file << "*** LogCollector stopped at " << Time::Current().Formatted()
           << " ***" << std::endl;
    m_done = true;
    if(m_thread.joinable())
      m_thread.join();
  }

  void LogCollector::OnTerminate(){
    m_done = true;
    DoTerminate();
    
  }
  
  void LogCollector::OnServer() {
    if (!m_logserver)
      EUDAQ_ERROR("Oops");
    SetStatusTag("_SERVER", m_logserver->ConnectionString());
  }

  void LogCollector::LogHandler(TransportEvent &ev) {
    switch (ev.etype) {
    case (TransportEvent::CONNECT):
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
      DoDisconnect(ev.id);
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
        m_logserver->SendPacket("OK", ev.id, true);
        ev.id.SetState(1); // successfully identified
        DoConnect(ev.id);
      } else {
        BufferSerializer ser(ev.packet.begin(), ev.packet.end());
        std::string src = ev.id.GetType();
        if (ev.id.GetName() != "")
          src += "." + ev.id.GetName();
	
	std::ostringstream buf;
	LogMessage logmesg(ser);
	logmesg.SetSender(src);
	logmesg.Write(buf);
	m_file.write(buf.str().c_str(), buf.str().length());
	m_file.flush();
	DoReceive(logmesg);
      }
      break;
    default:
      std::cout << "Unknown:    " << ev.id << std::endl;
    }
  }

  void LogCollector::LogThread(){
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


  void LogCollector::Exec(){
    try {
      while (!m_done){
	Process();
	//TODO: sleep here is needed.
      }
    } catch (const std::exception &e) {
      std::cout <<"LogCollector::Exec() Error: Uncaught exception: " <<e.what() <<std::endl;
    } catch (...) {
      std::cout <<"LogCollector::Exec() Error: Uncaught unrecognised exception" <<std::endl;
    }
  }
}
