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
      : CommandReceiver("LogCollector", "", runcontrol), m_exit(false),
        m_logserver(TransportFactory::CreateServer(listenaddress)),
        m_filename(logdirectory + "/" + Time::Current().Formatted("%Y-%m-%d.log")),
        m_file(m_filename.c_str(), std::ios_base::app) {
    if (!m_file.is_open())
      EUDAQ_THROWX(FileWriteException, "Unable to open log file (" +
		   m_filename + ") is there a logs directory?");
    else std::cout << "LogCollector opened \"" << m_filename << "\" for logging." << std::endl;
    m_logserver->SetCallback(TransportCallback(this, &LogCollector::LogHandler));
    std::ostringstream os;
    os << "\n*** LogCollector started at " << Time::Current().Formatted()
       << " ***" << std::endl;
    m_file.write(os.str().c_str(), os.str().length());
  }

  LogCollector::~LogCollector() {
    m_file << "*** LogCollector stopped at " << Time::Current().Formatted()
           << " ***" << std::endl;
    CloseLogCollector();
  }

  void LogCollector::OnTerminate(){
    CloseLogCollector();
    DoTerminate(); 
  }
  
  void LogCollector::OnServer() {
    if (!m_logserver)
      EUDAQ_ERROR("Oops");
    std::string addr_server = m_logserver->ConnectionString();
    std::string addr_cmd = GetCommandRecieverAddress();
    if(addr_server.find("tcp://") == 0 && addr_cmd.find("tcp://") == 0){
      addr_server = addr_cmd.substr(0, addr_cmd.find_last_of(":")) +
	addr_server.substr(addr_server.find_last_of(":"));
    }
    SetStatusTag("_SERVER", addr_server);
  }

  void LogCollector::LogHandler(TransportEvent &ev) {
    auto con = ev.id;
    switch (ev.etype) {
    case (TransportEvent::CONNECT):
      m_logserver->SendPacket("OK EUDAQ LOG LogCollector", *con, true);
      break;
    case (TransportEvent::DISCONNECT):
      // std::cout << "Disconnect: " << ev.id << std::endl;
      DoDisconnect(con);
      break;
    case (TransportEvent::RECEIVE):
      if (con->GetState() == 0) { // waiting for identification
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
          con->SetType(part);
          i0 = i1 + 1;
          i1 = ev.packet.find(' ', i0);
          part = std::string(ev.packet, i0, i1 - i0);
          con->SetName(part);
        } while (false);
        m_logserver->SendPacket("OK", *con, true);
        con->SetState(1); // successfully identified
        DoConnect(con);
      } else {
        BufferSerializer ser(ev.packet.begin(), ev.packet.end());
        std::string src = con->GetType();
        if (con->GetName() != "")
          src += "." + con->GetName();
	
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
      std::cout << "Unknown:    " << *con << std::endl;
    }
  }

  void LogCollector::LogThread(){
    try {
      // TODO: create m_logserver here instead of inside constructor
      while (!m_exit) {
        m_logserver->Process(100000);
      }
      // TODO: send disconnect event
    } catch (const std::exception &e) {
      std::cout << "Error: Uncaught exception: " << e.what() << "\n"
                << "LogThread is dying..." << std::endl;
    } catch (...) {
      std::cout << "Error: Uncaught unrecognised exception: \n"
                << "LogThread is dying..." << std::endl;
    }
  }

  void LogCollector::StartLogCollector(){
    if(m_exit){
      EUDAQ_THROW("LogCollector can not be restarted after exit. (TODO)");
    }
    m_thd_server = std::thread(&LogCollector::LogThread, this);
    std::cout << "###### listenaddress=" << m_logserver->ConnectionString()
              << std::endl
              << "       logfile=" << m_filename << std::endl;
  }

  void LogCollector::CloseLogCollector(){
    m_exit = true;
    if(m_thd_server.joinable())
      m_thd_server.join();
  }
  
  void LogCollector::Exec(){
    StartLogCollector(); //TODO: Start it OnServer
    StartCommandReceiver();
    while(IsActiveCommandReceiver() || IsActiveLogCollector()){
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
  }
}
