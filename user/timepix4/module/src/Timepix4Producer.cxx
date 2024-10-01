#include "eudaq/Producer.hh"
#include "eudaq/Configuration.hh"

#include <iostream>
#include <ostream>
#include <vector>
#include <unistd.h>
#include <iomanip>
#include <signal.h>
#ifdef WIN32
#include <Windows.h>
#else
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctime>
#endif
#include "Timepix4Producer.h"

//#define Tpx4_VERBOSE

class Timepix4Producer : public eudaq::Producer {
public:
  Timepix4Producer(const std::string name, const std::string &runcontrol);
  ~Timepix4Producer() override;
  void DoConfigure() override;
  void DoInitialise() override;
  void DoStartRun() override;
  void DoStopRun() override;
  void DoReset() override;
  void RunLoop() override;

  static const uint32_t m_id_factory = eudaq::cstr2hash("Timepix4Producer");
private:
  std::thread ts_thread;   // thread for 1 sec timestamps
  bool m_running = false;
  bool m_init = false;
  bool m_config = false;


  int m_supported_devices = 0;
  int m_active_devices;
  int m_spidrPort;
  int m_clientSocket;
  sockaddr_in6 m_serverAddress;
  string m_spidrIP, m_daqIP, m_xmlfileName, m_chipID;
  bool m_extRefClk, m_extT0;
  int m_xml_VTHRESH = 0;
  float m_temp;

  /** Return the binary representation of a char as std::string
   */
  template <typename T> std::string to_bit_string(const T data, int length=-1) {
    std::ostringstream stream;
    // if length is not defined, use a standard (full) one
    if (length<0 || length > std::numeric_limits<T>::digits) {
      length = std::numeric_limits<T>::digits;
      // std::numeric_limits<T>::digits does not include sign bits
      if (std::numeric_limits<T>::is_signed) {
        length++;
      }
    }
    while(--length >= 0) {
      stream << ((data >> length) & 1);
    }
    return stream.str();
  }

  template <typename T> std::string to_hex_string(const T i, int length=-1) {
    std::ostringstream stream;
    // if length is not defined, use a standard (full) one
    if (length<0 || length > ((std::numeric_limits<T>::digits+1)/4)) {
      length = (std::numeric_limits<T>::digits+1) / 4;
    }
    stream << std::hex << std::setfill('0') << std::setw(length)
           << static_cast<uint64_t>(i);
    return stream.str();
  }

  bool tokenize_ip_addr(const std::string addr, int *ip) {
    int j=0;
    int k=0;
    int l=0;
    for (int i=0; i<addr.size(); i++){
        if (addr[i] == '.' || i == addr.size()-1) {
            l = i-j;
            if (i == addr.size()-1) {
                l++;
            }
            ip[k] = std::stoi(addr.substr(j,l));
            j = i+1;
            if (++k>4) return false;
        }
    }
    if (k != 4) return false;
    else return true;
  }

};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::Producer>::
    Register<Timepix4Producer, const std::string&, const std::string&>(Timepix4Producer::m_id_factory);
}

Timepix4Producer::Timepix4Producer(const std::string name, const std::string &runcontrol)
: eudaq::Producer(name, runcontrol), m_running(false) {}

Timepix4Producer::~Timepix4Producer() {
  // force run stop
  m_running = false;
}

//----------------------------------------------------------
//  RESET
//----------------------------------------------------------
void Timepix4Producer::DoReset() {
  EUDAQ_USER("Timepix4Producer is going to be reset.");
  // stop the run, if it is still running
  m_running = false;
  // wait for other functions to finish


  m_init = false;
  m_config = false;

  EUDAQ_USER("Timepix4Producer was reset.");
} // DoReset()

//----------------------------------------------------------
//  INIT
//----------------------------------------------------------
void Timepix4Producer::DoInitialise() {
  bool serious_error = false;
  if (m_init || m_config || m_running) {
    EUDAQ_WARN("Timepix4Producer: Initializing while it is already in initialized state. Performing reset first...");
    DoReset();
  }

  auto config = GetInitConfiguration();

  EUDAQ_USER("Timepix4Producer initializing with configuration: " + config->Name());


  // SPIDR IP & PORT
  m_spidrIP  = config->Get( "SPIDR_IP", "127.0.0.1" );
  int ip[4];
  if (!tokenize_ip_addr(m_spidrIP, ip) ) {
      EUDAQ_ERROR("Incorrect SPIDR IP address: " + m_spidrIP);
  }
  m_spidrPort = config->Get( "SPIDR_Port", 50051 );

  m_clientSocket = socket(AF_INET6, SOCK_STREAM, 0);

  m_serverAddress.sin6_family = AF_INET6;
  m_serverAddress.sin6_addr = in6addr_any;
  m_serverAddress.sin6_port = htons(m_spidrPort);

  serious_error = connect(m_clientSocket, (struct sockaddr*)&m_serverAddress, sizeof(m_serverAddress));
  // Open a control connection to SPIDR-Tpx4 module

  if (serious_error) {
    EUDAQ_THROW("Timepix4Producer: Could not establish socket connection to TPX4 slow control. Make sure tpx4sc is running");
    return;
  }
  m_init = true;
  EUDAQ_USER("Timepix4Producer Init done");
} // DoInitialise()

//----------------------------------------------------------
//  CONFIGURE
//----------------------------------------------------------
void Timepix4Producer::DoConfigure() {
  bool serious_error = false;
  if (!m_init) {
    EUDAQ_THROW("DoConfigure: Trying to configure an uninitialized module. This will not work.");
    return;
  }
  if (m_running) {
    EUDAQ_WARN("DoConfigure: Trying to configure a running module. Trying to stop it first.");
    m_running = false;
  }

  auto config = GetConfiguration();
  EUDAQ_USER("Timepix4Producer configuring: " + config->Name());

  // Configuration file values are accessible as config->Get(name, default)
  config->Print();

  // sleep for 1 second, to make sure the TLU clock is already present
  sleep (1);
  EUDAQ_INFO("external_T0 = " + (m_extT0 ? std::string("true") : std::string("false")));
  if (serious_error) {
    EUDAQ_THROW("Timepix4Producer: There were major errors during configuration. See the log.");
    return;
  }
  m_config = true;
  // Also display something for us
  EUDAQ_USER("Timepix4Producer configured. Ready to start run. ");
} // DoConfigure()

//----------------------------------------------------------
//  START RUN
//----------------------------------------------------------
void Timepix4Producer::DoStartRun() {
  if (!(m_init && m_config)){
    m_running = false;
    EUDAQ_THROW("DoStartRun: Timepix4 producer is not configured properly. I can not start the run.");
    return;
  }
  if (m_running) {
    m_running = false;
    EUDAQ_WARN("DoStartRun: Timepix4 producer is already running. I'm trying to stop if first. This might however create a mess in the runs.");
  }
  m_running = true;

  EUDAQ_USER("Timepix4Producer started run.");
} // DoStartRun()

//----------------------------------------------------------
//  RUN LOOP
//----------------------------------------------------------
void Timepix4Producer::RunLoop() {
  int start_attempts = 0;
  while (!(m_running)) {
    if (++start_attempts > 3) {
      EUDAQ_THROW( "RunLoop: DoStartRun() was not executed properly before calling RunLoop(). Aborting run loop." );
      return;
    }
    EUDAQ_WARN( "RunLoop: Waiting to finish DoStartRun(). Attempt no. " + std::to_string(start_attempts));
    sleep(1);
  }

  EUDAQ_USER("Timepix4Producer starting run loop...");


  while(m_running) {
    }
    // Get a sample of pixel data packets, with timeout in ms

  EUDAQ_USER("Timepix4Producer exiting run loop.");
} // RunLoop()

//----------------------------------------------------------
//  STOP RUN
//----------------------------------------------------------
void Timepix4Producer::DoStopRun() {
  EUDAQ_USER("Timepix4Producer stopping run...");

  m_running = false;
  EUDAQ_USER("Timepix4Producer stopped run.");
} // DoStopRun()
