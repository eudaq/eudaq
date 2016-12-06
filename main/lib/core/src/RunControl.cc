#include "RunControl.hh"
#include "TransportFactory.hh"
#include "Configuration.hh"
#include "BufferSerializer.hh"
#include "Exception.hh"
#include "Utils.hh"
#include "Logger.hh"

#include <iostream>
#include <ostream>
#include <fstream>

namespace eudaq {

  template class DLLEXPORT Factory<RunControl>;
  template DLLEXPORT
  std::map<uint32_t, typename Factory<RunControl>::UP_BASE (*)(const std::string&)>&
  Factory<RunControl>::Instance<const std::string&>();
  
  RunControl::RunControl(const std::string &listenaddress)
      : m_done(false), m_listening(true), m_runnumber(-1),
        m_idata((size_t)-1), m_ilog((size_t)-1), m_runsizelimit(0),
        m_runeventlimit(0), m_stopping(false),
        m_producerbusy(false) {
    if (listenaddress != "") {
      StartServer(listenaddress);
    }
  }

  void RunControl::StartServer(const std::string &listenaddress) {
    m_done = false;
    m_cmdserver.reset(TransportFactory::CreateServer(listenaddress));
    m_cmdserver->SetCallback(
        TransportCallback(this, &RunControl::CommandHandler));
    m_thread = std::thread(&RunControl::CommandThread, this);
    std::cout << "DEBUG: listenaddress=" << m_cmdserver->ConnectionString()
              << std::endl;
  }

  void RunControl::StopServer() {
    m_done = true;
    if(m_thread.joinable())
      m_thread.join();
  }

  RunControl::~RunControl() { StopServer(); }

  void RunControl::Configure(const Configuration &config) {
    SendCommand("CLEAR");
    mSleep(500);
    SendCommand("CONFIG", to_string(config));
    if (config.SetSection("RunControl")) {
      m_runsizelimit = config.Get("RunSizeLimit", 0LL);
      m_runeventlimit = config.Get("RunEventLimit", 0LL);
      auto m_nextconfigonrunchange =
          config.Get("NextConfigFileOnRunChange",
                     config.Get("NextConfigFileOnFileLimit", false));
    } else {
      m_runsizelimit = 0;
      m_runeventlimit = 0;
      // m_nextconfigonrunchange = false;
    }
  }

  void RunControl::Configure(const std::string &param, int geoid) {
    EUDAQ_INFO("Configuring (" + param + ")");
    std::ifstream file(param.c_str());
    if (file.is_open()) {
      Configuration config(file);
      config.Set("Name", param);
      if (geoid)
        config.Set("GeoID", to_string(geoid));
      Configure(config);
    } else {
      EUDAQ_ERROR("Unable to open file '" + param + "'");
    }
  }

  void RunControl::Reset() {
    EUDAQ_INFO("Resetting");
    m_listening = true;
    SendCommand("RESET");
  }

  void RunControl::GetStatus() { SendCommand("STATUS"); }

  void RunControl::StartRun(const std::string &msg) {
    m_listening = false;
    m_stopping = false;
    m_runnumber++;

    EUDAQ_INFO("Starting Run " + to_string(m_runnumber) + ": " + msg);
    SendCommand("CLEAR");
    mSleep(500);
    // give the data collectors time to prepare
    for (std::map<size_t, std::string>::iterator it = m_dataaddr.begin();
         it != m_dataaddr.end(); ++it) {
      SendReceiveCommand("PREPARE", to_string(m_runnumber),
                         GetConnection(it->first));
    }
    mSleep(1000);
    SendCommand("START", to_string(m_runnumber));
  }

  void RunControl::StopRun(bool listen) {
    
    EUDAQ_INFO("Stopping Run " + to_string(m_runnumber));
    SendCommand("STOP");
    m_stopping = true;
    m_listening = listen;
  }

  void RunControl::Terminate() {
    EUDAQ_INFO("Terminating connections");
    SendCommand("TERMINATE");
    mSleep(5000);
  }

  void RunControl::SendCommand(const std::string &cmd, const std::string &param,
                               const ConnectionInfo &id){
    std::unique_lock<std::mutex> lk(m_mtx_sendcmd);
    
    std::string packet(cmd);
    if (param.length() > 0) {
      packet += '\0' + param;
    }
    m_cmdserver->SendPacket(packet, id);
  }

  std::string RunControl::SendReceiveCommand(const std::string &cmd,
                                             const std::string &param,
                                             const ConnectionInfo &id) {
    std::unique_lock<std::mutex> lk(m_mtx_sendcmd);
    mSleep(500); // make sure there are no pending replies //TODO: it is unreliable.
    std::string packet(cmd);
    if (param.length() > 0) {
      packet += '\0' + param;
    }
    std::string result;
    m_cmdserver->SendReceivePacket(packet, &result, id);
    return result;
  }

  void RunControl::CommandThread() {
    while (!m_done) {
      m_cmdserver->Process(100000);
    }
  }

  void RunControl::CommandHandler(TransportEvent &ev) {
    switch (ev.etype) {
    case (TransportEvent::CONNECT):
      std::cout << "Connect:    " << ev.id << std::endl;
      if (m_listening) {
        m_cmdserver->SendPacket("OK EUDAQ CMD RunControl", ev.id, true);
      } else {
        m_cmdserver->SendPacket("ERROR EUDAQ CMD Not accepting new connections",
                                ev.id, true);
        m_cmdserver->Close(ev.id);
      }
      break;
    case (TransportEvent::DISCONNECT):
      OnDisconnect(ev.id);
      if (m_idata != (size_t)-1 && ev.id.Matches(GetConnection(m_idata)))
        m_idata = (size_t)-1;
      if (m_ilog != (size_t)-1 && ev.id.Matches(GetConnection(m_ilog)))
        m_ilog = (size_t)-1;
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
          if (part != "CMD")
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

        m_cmdserver->SendPacket("OK", ev.id, true);
        ev.id.SetState(1); // successfully identified
        if (ev.id.GetType() == "LogCollector") {
          InitLog(ev.id);
        } else if (ev.id.GetType() == "DataCollector") {
          InitData(ev.id);
        } else {
          InitOther(ev.id);
        }
        OnConnect(ev.id);
      }
      else {
        BufferSerializer ser(ev.packet.begin(), ev.packet.end());
        std::shared_ptr<Status> status(new Status(ser));
        if (status->GetLevel() == Status::LVL_BUSY && ev.id.GetState() == 1) {
          ev.id.SetState(2);
        } else if (status->GetLevel() != Status::LVL_BUSY &&
                   ev.id.GetState() == 2) {
          ev.id.SetState(1);
        }
        bool busy = false;
        for (size_t i = 0; i < m_cmdserver->NumConnections(); ++i) {
          if (m_cmdserver->GetConnection(i).GetState() == 2) {
            busy = true;
            break;
          }
        }
        m_producerbusy = busy;
        if (from_string(status->GetTag("RUN"), m_runnumber) == m_runnumber) {
          // We ignore status messages that are marked with a previous run
          // number
          OnReceive(ev.id, status);
        }
      }
      break;
    default:
      std::cout << "Unknown:    " << ev.id << std::endl;
    }
  }

  void RunControl::InitData(const ConnectionInfo &id){
    // default DC already connected?
    if (m_idata != (size_t)-1) {
      if (id.GetName() == "") {
        EUDAQ_WARN("Default DataCollector already connected but additional "
                   "(unamed) DataCollector found");
        std::cout << "Default DataCollector already connected but additional "
                     "(unamed) DataCollector found" << std::endl;
      } else {
        EUDAQ_INFO("Additional DataCollector connecting: " + id.GetName());
      }
    }

    // this DC is default DC if unnamed or no other DC is yet connected
    bool isDefaultDC = false;
    if (id.GetName() == "" || m_idata == (size_t)-1) {
      isDefaultDC = true;
    }


    // search all connections for this particular DC (and remember other DCs
    // found)
    std::vector<std::string> otherDC; // holds names of other DCs found
                                      // (important for announcing this DC)
    size_t thisDCIndex = (size_t)-1;
    for (size_t i = 0; i < NumConnections(); ++i) {
      if (GetConnection(i).GetType() == "DataCollector") {
        if (GetConnection(i).GetName() == id.GetName()) {
          // found this DC
          if (thisDCIndex != (size_t)-1) {
            EUDAQ_WARN(
                "Multiple DataCollectors with the same name are connected!");
          }
          thisDCIndex = i;
        } else {
          // found other DC -- important when announcing DC later
          otherDC.push_back(GetConnection(i).GetName());
        }
      }
    }
    if (thisDCIndex == (size_t)-1)
      EUDAQ_THROW("DataCollector connection problem");

    // no name indicates default DC -- init global var
    if (isDefaultDC)
      m_idata = thisDCIndex;
    
    Status status =
      m_cmdserver->SendReceivePacket<Status>("SERVER", id, 1000000);
    std::string dsAddrReported = status.GetTag("_SERVER");
    if (dsAddrReported == "")
      EUDAQ_THROW("Invalid response from DataCollector");

    // determine data server remote IP ADDRESS from actual connection origin
    std::string dataip = "tcp://127.0.0.1";
    // strip off the port number
    size_t pos = id.GetRemote().find(":");
    if (pos != std::string::npos) {
      dataip = "tcp://";
      dataip += std::string(id.GetRemote(), 0, pos);
    }

    // determine data server remote PORT from number reported by data server
    std::string dataport = "44001";
    // strip off the protocol and IP
    pos = dsAddrReported.find("://");
    if (pos != std::string::npos)
      pos = dsAddrReported.find(":", pos + 3);
    else
      pos = dsAddrReported.find(":");
    if (pos != std::string::npos) {
      dataport = std::string(dsAddrReported, pos + 1);
    }

    // combine IP from connection with port number reported by data server
    std::string thisDataAddr = dataip;
    thisDataAddr += ":";
    thisDataAddr += dataport;
    std::cout
        << "DataServer responded: full server address determined to be  = '"
        << thisDataAddr << "'" << std::endl;

    std::pair<std::map<size_t, std::string>::iterator, bool> ret;
    ret = m_dataaddr.insert(
        std::pair<size_t, std::string>(thisDCIndex, thisDataAddr));
    if (ret.second == false) {
      std::cout << "Connection with DataCollector number " << thisDCIndex
                << " already existed" << std::endl;
      std::cout << " with a remote address of " << ret.first->second
                << std::endl;
    }

    for (size_t i = 0; i < NumConnections(); ++i) {
      std::string name_con =  GetConnection(i).GetName();
      if (isDefaultDC) {
        // check that we do not override a previously connected DC
        bool matches = false;
        for (std::vector<std::string>::iterator it = otherDC.begin();
             it != otherDC.end(); ++it) {
	  std::string name_otherDC = *it;
          if (name_otherDC.find(name_con)!=std::string::npos) {
            matches = true;
          }
        }
        if (!matches) {
          // announce default DC to any connected command receiver not
          // named the same as previously found DCs
          SendCommand("DATA", thisDataAddr, GetConnection(i));
        }
      } else {
	std::string name_this = id.GetName();
        if (name_this.find(name_con) != std::string::npos) {
          // announce named DC to specific producer
          SendCommand("DATA", thisDataAddr, GetConnection(i));
        }
      }
    }
    // announce log collector to DC
    if (m_ilog != (size_t)-1) {
      SendCommand("LOG", m_logaddr, id);
    }
  }

  void RunControl::InitLog(const ConnectionInfo &id) {
    if (m_ilog != (size_t)-1)
      EUDAQ_WARN("Log collector already connected");

    for (size_t i = 0; i < NumConnections(); ++i) {
      if (GetConnection(i).GetType() == "LogCollector") {
        m_ilog = i;
      }
    }
    if (m_ilog == (size_t)-1)
      EUDAQ_THROW("No LogCollector is connected");
    eudaq::Status status =
        m_cmdserver->SendReceivePacket<eudaq::Status>("SERVER", id, 1000000);

    m_logaddr = status.GetTag("_SERVER");
    std::cout << "LogServer responded: " << m_logaddr << std::endl;
    if (m_logaddr == "")
      EUDAQ_THROW("Invalid response from LogCollector");
    EUDAQ_LOG_CONNECT("RunControl", "", m_logaddr);
    SendCommand("LOG", m_logaddr);

    // loop over data collectors and announce LC
    for (std::map<size_t, std::string>::iterator it = m_dataaddr.begin();
         it != m_dataaddr.end(); ++it) {
      SendCommand("DATA", it->second, id);
    }
  }

  void RunControl::InitOther(const ConnectionInfo &id) {
    std::string packet;

    if (m_ilog != (size_t)-1) {
      SendCommand("LOG", m_logaddr, id);
    }

    std::string name_this = id.GetName();
    // search for applicable DC
    bool foundDC = false;
    for (std::map<size_t, std::string>::iterator it = m_dataaddr.begin();
         it != m_dataaddr.end(); ++it) {
      std::string name_dc = GetConnection(it->first).GetName();
      if (name_dc.find(name_this)!=std::string::npos) {
        foundDC = true;
        SendCommand("DATA", it->second, id);
      }
    }
    // if not found, use default DC
    if (!foundDC && m_idata != (size_t)-1) {
      SendCommand("DATA", m_dataaddr[m_idata], id);
    }
  }
}
