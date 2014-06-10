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

    class PseudoMutex {
      public:
        typedef bool type;
        PseudoMutex(type & flag) : m_flag(flag) {
          while (m_flag) {
            mSleep(10);
          };
          m_flag = true;
        }
        ~PseudoMutex() { m_flag = false; }
      private:
        type & m_flag;
    };

    void * RunControl_thread(void * arg) {
      RunControl * rc = static_cast<RunControl *>(arg);
      rc->CommandThread();
      return 0;
    }

  } // anonymous namespace

  RunControl::RunControl(const std::string & listenaddress)
    : m_done(false),
    m_listening(true),
    m_runnumber(-1),
    m_cmdserver(0),
    m_idata((size_t)-1),
    m_ilog((size_t)-1),
    m_runsizelimit(0),
    m_stopping(false),
    m_busy(false),
    m_producerbusy(false)
  {
    if (listenaddress != "") {
      StartServer(listenaddress);
    }
  }

  void RunControl::StartServer(const std::string & listenaddress) {
    m_done = false;
    m_cmdserver = TransportFactory::CreateServer(listenaddress);
    m_cmdserver->SetCallback(TransportCallback(this, &RunControl::CommandHandler));
	m_thread=std::unique_ptr<std::thread>(new std::thread(RunControl_thread, this));
    //pthread_attr_init(&m_threadattr);
    //pthread_create(&m_thread, &m_threadattr, RunControl_thread, this);
    std::cout << "DEBUG: listenaddress=" << m_cmdserver->ConnectionString() << std::endl;
  }

  void RunControl::StopServer() {
    m_done = true;
    ///*if (m_thread)*/ pthread_join(m_thread, 0);
	m_thread->join();
    delete m_cmdserver;
  }

  RunControl::~RunControl() {
    StopServer();
  }

  void RunControl::Configure(const Configuration & config) {
    SendCommand("CLEAR");
    mSleep(500);
    SendCommand("CONFIG", to_string(config));
    if (config.SetSection("RunControl")) {
      m_runsizelimit = config.Get("RunSizeLimit", 0LL);
    } else {
      m_runsizelimit = 0;
    }
  }

  void RunControl::Configure(const std::string & param, int geoid) {
    std::string filename = "../conf/" + (param == "" ? "default" : param) + ".conf";
    EUDAQ_INFO("Configuring (" + param + ")");
    //EUDAQ_EXTRA("Loading configuration from: " + filename);
    std::ifstream file(filename.c_str());
    if (file.is_open()) {
      Configuration config(file);
      config.Set("Name", param);
      if (geoid) config.Set("GeoID", to_string(geoid));
      Configure(config);
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
    m_stopping = false;
    m_runnumber++;
    //std::string packet;
    EUDAQ_INFO("Starting Run " + to_string(m_runnumber) + ": " + msg);
    SendCommand("CLEAR");
    mSleep(500);
    // give the data collectors time to prepare
    for (std::map<size_t,std::string>::iterator it=m_dataaddr.begin(); it!=m_dataaddr.end(); ++it){
	SendReceiveCommand("PREPARE", to_string(m_runnumber), GetConnection(it->first));
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

  void RunControl::SendCommand(const std::string & cmd, const std::string & param,
      const ConnectionInfo & id) {
    PseudoMutex m(m_busy);
    std::string packet(cmd);
    if (param.length() > 0) {
      packet += '\0' + param;
    }
    m_cmdserver->SendPacket(packet, id);
  }

  std::string RunControl::SendReceiveCommand(const std::string & cmd, const std::string & param,
      const ConnectionInfo & id) {
    PseudoMutex m(m_busy);
    mSleep(500); // make sure there are no pending replies
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

  void RunControl::CommandHandler(TransportEvent & ev) {
    //std::cout << "Event: ";
    switch (ev.etype) {
      case (TransportEvent::CONNECT):
        std::cout << "Connect:    " << ev.id << std::endl;
        if (m_listening) {
          m_cmdserver->SendPacket("OK EUDAQ CMD RunControl", ev.id, true);
        } else {
          m_cmdserver->SendPacket("ERROR EUDAQ CMD Not accepting new connections", ev.id, true);
          m_cmdserver->Close(ev.id);
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
        } else {
          BufferSerializer ser(ev.packet.begin(), ev.packet.end());
          std::shared_ptr<Status> status(new Status(ser));
          if (status->GetLevel() == Status::LVL_BUSY && ev.id.GetState() == 1) {
            ev.id.SetState(2);
          } else if (status->GetLevel() != Status::LVL_BUSY && ev.id.GetState() == 2) {
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
            // We ignore status messages that are marked with a previous run number
            OnReceive(ev.id, status);
          }
          //std::cout << "Receive:    " << ev.id << " \'" << ev.packet << "\'" << std::endl;
        }
        break;
      default:
        std::cout << "Unknown:    " << ev.id << std::endl;
    }
  }

  void RunControl::InitData(const ConnectionInfo & id) {
    // default DC already connected?
    if (m_idata != (size_t)-1) {
      if (id.GetName() == ""){
	EUDAQ_WARN("Default DataCollector already connected but additional (unamed) DataCollector found");
	std::cout << "Default DataCollector already connected but additional (unamed) DataCollector found" << std::endl;
      } else {
	EUDAQ_INFO("Additional DataCollector connecting: "+id.GetName());
      }
    }

    // this DC is default DC if unnamed or no other DC is yet connected
    bool isDefaultDC = false;
    if (id.GetName() == "" || m_idata == (size_t)-1){
      isDefaultDC = true;
      std::cout << "New default DataCollector with name '" << id.GetName() << "' is connecting" << std::endl;
    } else {
      std::cout << "Additional DataCollector with name '" << id.GetName() << "' is connecting" << std::endl;
    }

    eudaq::Status status = m_cmdserver->SendReceivePacket<eudaq::Status>("GETRUN", id, 1000000);
    std::string part = status.GetTag("_RUN");

    if (part == "") EUDAQ_THROW("Bad response from DataCollector");

    // verify and initialize (correct) run number
    unsigned thisRunNumber = from_string(part, 0);
    std::cout << "DataServer responded: RunNumber = " << thisRunNumber << std::endl;

    if (m_runnumber >=0){
      // we have set the run number value before
      if (m_runnumber != static_cast<int32_t>(thisRunNumber)){
	EUDAQ_THROW("DataCollector run number mismatch! Previously received run number does not match run number reported by newly connected DataCollector "+id.GetName());
      }
    } else {
      // set the run number value to the one reported by the data collector
      m_runnumber = thisRunNumber;
    }

    // search all connections for this particular DC (and remember other DCs found)
    std::vector<std::string> otherDC; // holds names of other DCs found (important for announcing this DC)
    size_t thisDCIndex = (size_t) -1;
    for (size_t i = 0; i < NumConnections(); ++i) {
      if (GetConnection(i).GetType() == "DataCollector") {
	if (GetConnection(i).GetName() == id.GetName()){
	  // found this DC
	  if (thisDCIndex != (size_t) -1){
	    EUDAQ_WARN("Multiple DataCollectors with the same name are connected!");
	  }
	  thisDCIndex = i;
	}else{
	  // found other DC -- important when announcing DC later
	  otherDC.push_back(GetConnection(i).GetName());
	}
      }
    }
    if (thisDCIndex == (size_t)-1) EUDAQ_THROW("DataCollector connection problem");
  
    // no name indicates default DC -- init global var
    if (isDefaultDC) m_idata = thisDCIndex;

    status = m_cmdserver->SendReceivePacket<eudaq::Status>("SERVER", id, 1000000);
    std::string dsAddrReported = status.GetTag("_SERVER");
    if (dsAddrReported == "") EUDAQ_THROW("Invalid response from DataCollector");

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
      pos = dsAddrReported.find(":",pos+3);
    else
      pos = dsAddrReported.find(":");
    if (pos != std::string::npos) {
      dataport = std::string(dsAddrReported, pos+1);
    }

    // combine IP from connection with port number reported by data server
    std::string thisDataAddr = dataip;  thisDataAddr += ":"; thisDataAddr += dataport;
    std::cout << "DataServer responded: full server address determined to be  = '" << thisDataAddr << "'" << std::endl;

    std::pair<std::map<size_t,std::string>::iterator,bool> ret;
    ret = m_dataaddr.insert ( std::pair<size_t, std::string>(thisDCIndex,thisDataAddr) );
    if (ret.second==false) {
      std::cout << "Connection with DataCollector number " << thisDCIndex << " already existed" << std::endl;
      std::cout << " with a remote address of " << ret.first->second << std::endl;
    }

    for (size_t i = 0; i < NumConnections(); ++i) {
      if (isDefaultDC){
	// check that we do not override a previously connected DC
	bool matches = false;
	for (std::vector<std::string>::iterator it = otherDC.begin() ; it != otherDC.end(); ++it){
	  if (GetConnection(i).GetName() == *it){
	    matches = true;
	  }
	}
	if (!matches){
	  // announce default DC to any connected command receiver not
	  // named the same as previously found DCs
	  SendCommand("DATA", thisDataAddr,GetConnection(i));
	}
      } else {
	if (GetConnection(i).GetName() == id.GetName()) {
	  // announce named DC to specific producer
	  SendCommand("DATA", thisDataAddr,GetConnection(i));
	}
      }
    }
    // announce log collector to DC
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
    eudaq::Status status = m_cmdserver->SendReceivePacket<eudaq::Status>("SERVER", id, 1000000);

    m_logaddr = status.GetTag("_SERVER");
    std::cout << "LogServer responded: " << m_logaddr << std::endl;
    if (m_logaddr == "") EUDAQ_THROW("Invalid response from LogCollector");
    EUDAQ_LOG_CONNECT("RunControl", "", m_logaddr);
    SendCommand("LOG", m_logaddr);

    // loop over data collectors and announce LC
    for (std::map<size_t,std::string>::iterator it=m_dataaddr.begin(); it!=m_dataaddr.end(); ++it){
      SendCommand("DATA", it->second, id);
    }
  }

  void RunControl::InitOther(const ConnectionInfo & id) {
    std::string packet;

    if (m_ilog != (size_t)-1) {
      SendCommand("LOG", m_logaddr, id);
    }

    // search for applicable DC
    bool foundDC = false;
    for (std::map<size_t,std::string>::iterator it=m_dataaddr.begin(); it!=m_dataaddr.end(); ++it){
      if (GetConnection(it->first).GetName() == id.GetName()){
	foundDC= true;
	SendCommand("DATA", it->second, id);
      }
    }
    // if not found, use default DC
    if (!foundDC && m_idata != (size_t)-1) {
      SendCommand("DATA", m_dataaddr[m_idata], id);
    }
  }

}
