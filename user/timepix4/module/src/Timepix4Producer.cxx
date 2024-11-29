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
  std::string SendMessage(const char* message);
  
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
  int m_threshold = 1000;
  
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
  close(m_clientSocket);

  string response = SendMessage("reset\n");
  EUDAQ_USER("Slow control response: " + response);

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
  m_spidrIP  = config->Get( "SPIDR_IP", "192.168.1.10" );
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

  string response = SendMessage("get_devid\n");
  EUDAQ_USER("Init done for deviceID: " + response);

  m_init = true;
  //  EUDAQ_USER("Timepix4Producer Init done");

} // DoInitialise()

//----------------------------------------------------------
//  CONFIGURE
//----------------------------------------------------------
void Timepix4Producer::DoConfigure() {
  ssize_t bitRecv;
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
  int threshold = config->Get( "threshold", m_threshold );

  // sleep for 1 second, to make sure the TLU clock is already present
  sleep(1);
  m_config = true;
  std::string message = "configure\n";
  bitRecv = send(m_clientSocket, message.c_str(), message.size(),0);
  std::cout << "AAAAAAAAAAA " <<  bitRecv << std::endl;
  std::cout << message << std::endl;
  if (bitRecv == -1) {
    EUDAQ_THROW("Timepix4Producer: Could not configure device.");
    return;
  }

  string response = SendMessage("configure\n");
  EUDAQ_USER("Configure message received response: " + response);

  string setthreshold = string("set_threshold ") + to_string(threshold) + string("\n");
  response = SendMessage(setthreshold.c_str());
  EUDAQ_USER("Threshold command received response: " + response);

  m_config = true;
  // Also display something for us
  if (bitRecv == -1) {
    EUDAQ_THROW("Timepix4Producer: Could not set threshold.");
    return;
  }
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

  string response = SendMessage("start_run\n");
  EUDAQ_USER("Timepix4Producer start command received response: " + response);



  /* this should do things like ((based on TPX3 producer))
    - restart timers (T0 sync?!)
    - selecting data driven readout
    - open the shutter
  */
  
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

  /* here I need to get the DATA from the SPIDR and put it into events
     Question: where exactly does the magic happen that starts the sdaq ?
   */
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

  string response = SendMessage("stop_run\n");
  EUDAQ_USER("Stop command response: " + response);

  m_running = false;
  EUDAQ_USER("Timepix4Producer stopped run.");
} // DoStopRun()



//----------------------------------------------------------
// communicating witht the slow control
//----------------------------------------------------------
// TO DO: add error handling!
std::string Timepix4Producer::SendMessage(const char* message) {
  int returnbytes =  send(m_clientSocket, message, strlen(message), 0);
  // for the future: change to debug instead of user
  EUDAQ_USER("Sent " + string(message) + " command, got confirmation for number of sent bytes = " + to_string(returnbytes));

  // Receive a response
  char buffer[1024];
  recv(m_clientSocket, buffer, sizeof(buffer), 0);
  string response=buffer;
  response=response.substr(0, response.find("\n"));

  return response;
}

