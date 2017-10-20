#include "eudaq/LogCollector.hh"
#include "eudaq/LogMessage.hh"
#include "eudaq/Logger.hh"
#include "eudaq/TransportServer.hh"
#include "eudaq/BufferSerializer.hh"
#include <iostream>
#include <ostream>
#include <sstream>

namespace eudaq {

  template class DLLEXPORT Factory<LogCollector>;
  template DLLEXPORT
  std::map<uint32_t, typename Factory<LogCollector>::UP_BASE (*)
	   (const std::string&, const std::string&)>&
  Factory<LogCollector>::Instance<const std::string&, const std::string&>();
  
  LogCollector::LogCollector(const std::string &name, const std::string &runcontrol)
    : CommandReceiver("LogCollector", name, runcontrol), m_exit(false){
  }
  
  LogCollector::~LogCollector(){
    CloseLogCollector();
  }

  void LogCollector::OnInitialise(){
    auto conf = GetConfiguration();
    try{
      DoInitialise();
      CommandReceiver::OnInitialise();
    }catch (const Exception &e) {
      std::string msg = "Error when init by " + conf->Name() + ": " + e.what();
      EUDAQ_ERROR(msg);
      SetStatus(Status::STATE_ERROR, msg);
    }
  }

  
  void LogCollector::OnTerminate(){
    CloseLogCollector();
    DoTerminate();
    CommandReceiver::OnTerminate();
    std::exit(0);
  }
  
  void LogCollector::LogHandler(TransportEvent &ev) {
    auto con = ev.id;
    switch (ev.etype) {
    case (TransportEvent::CONNECT):
      m_logserver->SendPacket("OK EUDAQ LOG LogCollector", *con, true);
      break;
    case (TransportEvent::DISCONNECT):
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
	// m_file.write(buf.str().c_str(), buf.str().length());
	// m_file.flush();
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
    if(m_log_addr.empty()){
      m_log_addr = "tcp://0";
    }
    m_logserver.reset(TransportServer::CreateServer(m_log_addr)),
    m_logserver->SetCallback(TransportCallback(this, &LogCollector::LogHandler));
    
    m_thd_server = std::thread(&LogCollector::LogThread, this);
    m_log_addr = m_logserver->ConnectionString();
    SetStatusTag("_SERVER", m_log_addr);
  }

  void LogCollector::CloseLogCollector(){
    m_exit = true;
    if(m_thd_server.joinable())
      m_thd_server.join();
  }
  
  void LogCollector::Exec(){
    StartLogCollector(); //TODO: Start it OnServer
    Connect();
    while(IsConnected() || IsActiveLogCollector()){
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
  }
}
