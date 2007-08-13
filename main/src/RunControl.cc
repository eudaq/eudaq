#include "eudaq/RunControl.hh"
#include "eudaq/TransportFactory.hh"
#include "eudaq/Configuration.hh"
#include "eudaq/BufferSerializer.hh"
#include "eudaq/Exception.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Logger.hh"

#include <iostream>
#include <ostream>
#include <fstream>

namespace eudaq {

  namespace {

    void * RunControl_thread(void * arg) {
      RunControl * rc = static_cast<RunControl *>(arg);
      rc->CommandThread();
      return 0;
    }

  } // anonymous namespace

  RunControl::RunControl(const std::string & listenaddress)
    : m_done(false),
      m_listening(true),
      m_runnumber(0),
      m_transport(0),
      m_idata((size_t)-1),
      m_ilog((size_t)-1)
  {
    if (listenaddress != "") {
      StartServer(listenaddress);
    }
  }

  void RunControl::StartServer(const std::string & listenaddress) {
    m_done = false;
    m_transport = TransportFactory::CreateServer(listenaddress);
    m_transport->SetCallback(TransportCallback(this, &RunControl::CommandHandler));
    pthread_attr_init(&m_threadattr);
    pthread_create(&m_thread, &m_threadattr, RunControl_thread, this);
    std::cout << "DEBUG: listenaddress=" << m_transport->ConnectionString() << std::endl;
  }

  void RunControl::StopServer() {
    m_done = true;
    /*if (m_thread)*/ pthread_join(m_thread, 0);
    delete m_transport;
  }

  RunControl::~RunControl() {
    StopServer();
  }

  void RunControl::Configure(const std::string & param) {
    std::string filename = "conf/" + (param == "" ? "default" : param) + ".conf";
    EUDAQ_EXTRA("Loading configuration from: " + filename);
    std::ifstream file(filename.c_str());
    if (file.is_open()) {
      Configuration config(file);
      SendCommand("CONFIG", to_string(config));
    } else {
      EUDAQ_ERROR("Unable to open file '" + filename + "'");
    }
  }

  void RunControl::Reset() {
    EUDAQ_INFO("Resetting");
    m_listening = true;
    SendCommand("RESET");
  }

  void RunControl::GetStatus() {
    SendCommand("STATUS");
  }

  void RunControl::StartRun(const std::string & msg) {
    m_listening = false;
    m_runnumber++;
    //std::string packet;
    EUDAQ_INFO("Starting Run " + to_string(m_runnumber) + ": " + msg);
    if (m_idata != (size_t)-1) {
      SendCommand("PREPARE", to_string(m_runnumber), m_transport->GetConnection(m_idata));
      mSleep(50);
    }
//       EUDAQ_ERROR("No response from DataCollector");
//       m_runnumber--;
//       return;
//     }
//     EUDAQ_EXTRA("DataCollector responded " + packet);
    SendCommand("START", to_string(m_runnumber));
  }

  void RunControl::StopRun() {
    EUDAQ_INFO("Stopping Run " + to_string(m_runnumber));
    SendCommand("STOP");
    m_listening = true;
  }

  void RunControl::Terminate() {
    EUDAQ_INFO("Terminating connections");
    SendCommand("TERMINATE");
  }

  void RunControl::SendCommand(const std::string & cmd, const std::string & param,
                               const ConnectionInfo & id) {
    std::string packet(cmd);
    if (param.length() > 0) {
      packet += '\0' + param;
    }
    m_transport->SendPacket(packet, id);
  }

  void RunControl::CommandThread() {
    while (!m_done) {
      m_transport->Process(100000);
    }
  }

  void RunControl::CommandHandler(TransportEvent & ev) {
    //std::cout << "Event: ";
    switch (ev.etype) {
    case (TransportEvent::CONNECT):
      std::cout << "Connect:    " << ev.id << std::endl;
      if (m_listening) {
        m_transport->SendPacket("OK EUDAQ CMD RunControl", ev.id, true);
      } else {
        m_transport->SendPacket("ERROR EUDAQ CMD Not accepting new connections", ev.id, true);
        m_transport->Close(ev.id);
      }
      break;
    case (TransportEvent::DISCONNECT):
      //std::cout << "Disconnection: " << ev.id << std::endl;
      OnDisconnect(ev.id);
      if (m_idata != (size_t)-1 && ev.id.Matches(GetConnection(m_idata))) m_idata = (size_t)-1;
      if (m_ilog  != (size_t)-1 && ev.id.Matches(GetConnection(m_ilog)))  m_ilog  = (size_t)-1;
      break;
    case (TransportEvent::RECEIVE):
      //std::cout << "Receive: " << ev.packet << std::endl;
      if (ev.id.GetState() == 0) { // waiting for identification
        // check packet
        do {
          size_t i0 = 0, i1 = ev.packet.find(' ');
          if (i1 == std::string::npos) break;
          std::string part(ev.packet, i0, i1);
          if (part != "OK") break;
          i0 = i1+1;
          i1 = ev.packet.find(' ', i0);
          if (i1 == std::string::npos) break;
          part = std::string(ev.packet, i0, i1-i0);
          if (part != "EUDAQ") break;
          i0 = i1+1;
          i1 = ev.packet.find(' ', i0);
          if (i1 == std::string::npos) break;
          part = std::string(ev.packet, i0, i1-i0);
          if (part != "CMD") break;
          i0 = i1+1;
          i1 = ev.packet.find(' ', i0);
          part = std::string(ev.packet, i0, i1-i0);
          ev.id.SetType(part);
          i0 = i1+1;
          i1 = ev.packet.find(' ', i0);
          part = std::string(ev.packet, i0, i1-i0);
          ev.id.SetName(part);
        } while(false);
        //std::cout << "client replied, sending OK" << std::endl;
        m_transport->SendPacket("OK", ev.id, true);
        ev.id.SetState(1); // successfully identified
        if (ev.id.GetType() == "LogCollector") {
          InitLog(ev.id);
        } else if (ev.id.GetType() == "DataCollector") {
          InitData(ev.id);
        } else {
          InitOther(ev.id);
        }
        OnConnect(ev.id);
      } else {
        BufferSerializer ser(ev.packet.begin(), ev.packet.end());
        counted_ptr<Status> status(new Status(ser));
        OnReceive(ev.id, status);
        //std::cout << "Receive:    " << ev.id << " \'" << ev.packet << "\'" << std::endl;
      }
      break;
    default:
      std::cout << "Unknown:    " << ev.id << std::endl;
    }
  }

  void RunControl::InitData(const ConnectionInfo & id) {
    if (m_idata != (size_t)-1) EUDAQ_WARN("Data collector already connected");

    eudaq::Status status = m_transport->SendReceivePacket<eudaq::Status>("GETRUN", id, 1000000);
    std::string part = status.GetTag("_RUN");

    if (part == "") EUDAQ_THROW("Bad response from DataCollector");
    m_runnumber = from_string(part, 0);
    std::cout << "DataServer responded: RunNumber = " << m_runnumber << std::endl;

    for (size_t i = 0; i < NumConnections(); ++i) {
      if (GetConnection(i).GetType() == "DataCollector") {
        m_idata = i;
      }
    }
    if (m_idata == (size_t)-1) EUDAQ_THROW("No DataCollector is connected");

    status = m_transport->SendReceivePacket<eudaq::Status>("SERVER", id, 1000000);
    m_dataaddr = status.GetTag("_SERVER");
    std::cout << "DataServer responded: " << m_dataaddr << std::endl;
    if (m_dataaddr == "") EUDAQ_THROW("Invalid response from DataCollector");
    SendCommand("DATA", m_dataaddr);

    if (m_ilog != (size_t)-1) {
      SendCommand("LOG", m_logaddr, id);
    }
  }

  void RunControl::InitLog(const ConnectionInfo & id) {
    if (m_ilog != (size_t)-1) EUDAQ_WARN("Log collector already connected");

    for (size_t i = 0; i < NumConnections(); ++i) {
      if (GetConnection(i).GetType() == "LogCollector") {
        m_ilog = i;
      }
    }
    if (m_ilog == (size_t)-1) EUDAQ_THROW("No LogCollector is connected");
    eudaq::Status status = m_transport->SendReceivePacket<eudaq::Status>("SERVER", id, 1000000);

    m_logaddr = status.GetTag("_SERVER");
    std::cout << "LogServer responded: " << m_logaddr << std::endl;
    if (m_logaddr == "") EUDAQ_THROW("Invalid response from LogCollector");
    EUDAQ_LOG_CONNECT("RunControl", "", m_logaddr);
    SendCommand("LOG", m_logaddr);

    if (m_idata != (size_t)-1) {
      SendCommand("DATA", m_dataaddr, id);
    }
  }

  void RunControl::InitOther(const ConnectionInfo & id) {
    std::string packet;

    if (m_ilog != (size_t)-1) {
      SendCommand("LOG", m_logaddr, id);
    }

    if (m_idata != (size_t)-1) {
      SendCommand("DATA", m_dataaddr, id);
    }
  }

}
