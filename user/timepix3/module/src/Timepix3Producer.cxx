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
#include <ctime>
#endif
#include "Timepix3Config.h"

//#define TPX3_VERBOSE

class Timepix3Producer : public eudaq::Producer {
public:
  Timepix3Producer(const std::string name, const std::string &runcontrol);
  ~Timepix3Producer() override;
  void DoConfigure() override;
  void DoInitialise() override;
  void DoStartRun() override;
  void DoStopRun() override;
  void DoReset() override;
  void DoTerminate() override;
  void RunLoop() override;

  static const uint32_t m_id_factory = eudaq::cstr2hash("Timepix3Producer");
private:
  bool m_running;
  double getTpx3Temperature();

  unsigned m_ev;
  int m_spidrPort;
  int device_nr = 0;
  int supported_devices = 0;
  int m_active_devices;
  string m_spidrIP, m_daqIP, m_xmlfileName, m_time, m_chipID;
  Timepix3Config *myTimepix3Config;
  SpidrController *spidrctrl;
  SpidrDaq *spidrdaq;
  bool init_done = false;
  bool conf_done = false;
  int m_xml_VTHRESH;
  float m_temp;
  std::stringstream convstream;

  /** Return the binary representation of a char as std::string
   */
  template <typename T> std::string to_bit_string(const T data) {
    std::ostringstream stream;
    for(int i = std::numeric_limits<T>::digits - 1; i >= 0; i--) {
      stream << ((data >> i) & 1);
    }
    return stream.str();
  }

  template <typename T> std::string to_hex_string(const T i) {
    std::ostringstream stream;
    stream << std::hex << std::showbase << std::setfill('0') << std::setw(std::numeric_limits<T>::digits / 4) << std::hex
           << static_cast<uint64_t>(i);
    return stream.str();
  }

    /** Helper function to return a printed list of an integer vector, used to shield
   *  debug code from being executed if debug level is not sufficient
   */
  template <typename T> std::string listVector(std::map<T, T> vec, std::string separator = ", ") {
    std::stringstream os;
    for(auto it : vec) {
      os << to_hex_string(it.first) << ": ";
      os << static_cast<uint64_t>(it.second);
      os << separator;
    }
    return os.str();
  };
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::Producer>::
    Register<Timepix3Producer, const std::string&, const std::string&>(Timepix3Producer::m_id_factory);
}

Timepix3Producer::Timepix3Producer(const std::string name, const std::string &runcontrol)
: eudaq::Producer(name, runcontrol), m_ev(0), m_running(false) {
  myTimepix3Config = new Timepix3Config();
}

Timepix3Producer::~Timepix3Producer() {}

void Timepix3Producer::DoReset() {
  if(init_done) {
    spidrctrl->closeShutter();
    std::cout << "Deleting spidrctrl instance... ";
    delete spidrctrl;
    std::cout << " ...DONE" << std::endl;
    init_done = false;
  }
  if(conf_done) {
    std::cout << "Deleting spidrdaq instance... ";
    delete spidrdaq;
    std::cout << " ...DONE" << std::endl;
    conf_done = false;
  }
}

void Timepix3Producer::DoInitialise() {
  auto config = GetInitConfiguration();

  std::cout << "Initializing: " << config->Name() << std::endl;

  // SPIDR-TPX3 IP & PORT
  m_spidrIP  = config->Get( "SPIDR_IP", "192.168.100.10" );
  int ip[4];
  vector<TString> ipstr = tokenise( m_spidrIP, ".");
  for (int i = 0; i < ipstr.size(); i++ ) ip[i] = ipstr[i].Atoi();
  m_spidrPort = config->Get( "SPIDR_Port", 50000 );

  //EUDAQ_USER("SPIDR address: " + std::to_string(ip[0]) + "." + std::to_string(ip[1]) + "." + std::to_string(ip[2]) + "." + std::to_string(ip[3]) + ":" + std::to_string(m_spidrPort));

  // Open a control connection to SPIDR-TPX3 module
  // with address 192.168.100.10, default port number 50000
  spidrctrl = new SpidrController( ip[0], ip[1], ip[2], ip[3], m_spidrPort );

  // Are we connected to the SPIDR?
  if (!spidrctrl->isConnected()) {
    EUDAQ_ERROR("Connection to SPIDR failed at: " + spidrctrl->ipAddressString() + "; status: " + spidrctrl->connectionStateString() + ", " + spidrctrl->connectionErrString());
  } else {
    EUDAQ_USER("SPIDR is connected at " + spidrctrl->ipAddressString() + "; status: " + spidrctrl->connectionStateString());
    EUDAQ_USER("SPIDR Class:    " + spidrctrl->versionToString( spidrctrl->classVersion() ));

    int firmwVersion, softwVersion = 0;
    if( spidrctrl->getFirmwVersion( &firmwVersion ) ) {
      EUDAQ_USER("SPIDR Firmware: " + spidrctrl->versionToString( firmwVersion ));
    }

    if( spidrctrl->getSoftwVersion( &softwVersion ) ) {
      EUDAQ_USER("SPIDR Software: " + spidrctrl->versionToString( softwVersion ));
    }
  }
  init_done = true;
}


// This gets called whenever the DAQ is configured
void Timepix3Producer::DoConfigure() {
  auto config = GetConfiguration();

  std::cout << "Configuring: " << config->Name() << std::endl;

  // Do any configuration of the hardware here
  // Configuration file values are accessible as config->Get(name, default)

  // Timepix3 XML config
  m_xmlfileName = config->Get( "XMLConfig", "" );
  myTimepix3Config->ReadXMLConfig( m_xmlfileName );
  cout << "Configuration file created on: " << myTimepix3Config->getTime() << endl;

  // set whether external clock (TLU) is used or device runs standalone
  bool cfg_extRefClk = config->Get("external_clock", true);
  if (!spidrctrl->setExtRefClk(cfg_extRefClk)) {
    EUDAQ_ERROR("setExtRefClk: " + spidrctrl->errorString());
  } else {
    EUDAQ_USER("setExtRefClk = " + (cfg_extRefClk ? std::string("true") : std::string("false")));
  }

  // reset
  int errstat;
  convstream.str(std::string());
  if( !spidrctrl->reset( &errstat ) ) {
    convstream << std::hex << errstat;
    EUDAQ_ERROR("reset: ERROR (" + convstream.str() + ")");
  } else {
    convstream << std::hex << errstat << std::dec;
    EUDAQ_USER("reset: OK (" + convstream.str() + ")");
  }

  // determine number of devices, does not check if devices are active
  if ( !spidrctrl->getDeviceCount( &supported_devices ) ) {
    EUDAQ_ERROR( "getDeviceCount" + spidrctrl->errorString());
  }
  EUDAQ_USER("Number of devices supported by firmware: " + std::to_string(supported_devices));

  // set number of active devices
  m_active_devices = config->Get("active_devices", 1);
  EUDAQ_USER("Number of active devices set in configuration: " + std::to_string(m_active_devices));
  if (supported_devices<m_active_devices) {
    EUDAQ_ERROR("You defined more active devices than what the system supports.");
  }
  if (m_active_devices<1) {
    EUDAQ_ERROR("You defined less than one active device.");
  }
  if (m_active_devices>1) {
    EUDAQ_ERROR("Only one active device is supported in this EUDAQ producer version.");
  }

  device_nr = 0;

  // Reset Device
  if( !spidrctrl->reinitDevice( device_nr ) ) {
    EUDAQ_ERROR("reinitDevice: " + spidrctrl->errorString());
  } else {
    EUDAQ_USER("reinitDevice: OK" );
  }

  // set destination (DAQ PC) IP address
  std::string default_daqIP = m_spidrIP;
  while ( default_daqIP.back()!='.' && !default_daqIP.empty() ) {
    default_daqIP.pop_back();
  }
  default_daqIP.push_back('1');
  m_daqIP  = config->Get( "DAQ_IP", default_daqIP );
  int ip[4];
  vector<TString> ipstr = tokenise( m_daqIP, ".");
  for (int i = 0; i < ipstr.size(); i++ ) ip[i] = ipstr[i].Atoi();
  int newDestIP  = ((ip[3] & 0xFF) | ((ip[2] & 0xFF) << 8) | ((ip[1] & 0xFF) << 16) | ((ip[0] & 0xFF) << 24) );
  EUDAQ_USER("Setting destination (DAQ) address to " + std::to_string(ip[0]) + "." + std::to_string(ip[1]) + "." + std::to_string(ip[2]) + "." + std::to_string(ip[3]));
  if( !spidrctrl->setIpAddrDest( device_nr, newDestIP ) ) {
    EUDAQ_ERROR("setIpAddrDest: " + spidrctrl->errorString());
  } else {
    int addr;
    if ( !spidrctrl->getIpAddrDest( device_nr, &addr ) ) {
      EUDAQ_ERROR( "getIpAddrDest: " + spidrctrl->errorString());
    } else {
      EUDAQ_USER("getIpAddrDest: " + std::to_string((addr >> 24) & 0xFF) + "."  + std::to_string((addr >> 16) & 0xFF) + "."  + std::to_string((addr >> 8) & 0xFF) + "."  + std::to_string(addr  & 0xFF) );
    }
  }

  //Due to timing issue, set readout speed at 320 Mbps
  if( !spidrctrl->setReadoutSpeed( device_nr, 320) ) {
    EUDAQ_ERROR("setReadoutSpeed: " + spidrctrl->errorString());
  } else {
    EUDAQ_USER("setReadoutSpeed = 320");
  }

  // set output mask
  if( !spidrctrl->setOutputMask(device_nr, 0xFF) ) {
    EUDAQ_ERROR("setOutputMask: " + spidrctrl->errorString());
  } else {
    EUDAQ_USER("setOutputMask = 0xFF");
  }



  // check link status
  int linkstatus;
  if( !spidrctrl->getLinkStatus( device_nr, &linkstatus ) ) {
    EUDAQ_ERROR( "getLinkStatus: " + spidrctrl->errorString());
  } else {
    // Link status: bits 0-7: 0=link enabled; bits 16-23: 1=link locked
    int links_enabled = (~linkstatus) & 0xFF;
    int links_locked  = (linkstatus & 0xFF0000) >> 16;

    convstream.str(std::string());
    for(int i = 7; i >= 0; i--) {
      convstream << ((links_enabled >> i) & 1);
    }
    EUDAQ_USER("Links enabled: 0b" + convstream.str());
    convstream.str(std::string());
    for(int i = 7; i >= 0; i--) {
      convstream << ((links_locked >> i) & 1);
    }
    EUDAQ_USER("Links locked : 0b" + convstream.str());
  }

  // Device ID
  int device_id = -1;
  if( !spidrctrl->getDeviceId( device_nr, &device_id ) ) {
    EUDAQ_ERROR("getDeviceId: " + spidrctrl->errorString());
  } else {
    convstream.str(std::string());
    convstream << std::hex << device_id << std::dec;
    EUDAQ_USER("getDeviceId: 0x" + std::to_string(device_id));
  }

  int waferno = (device_id >> 8) & 0xFFF;
  int id_y = (device_id >> 4) & 0xF;
  int id_x = (device_id >> 0) & 0xF;
  convstream.str(std::string());
  convstream << "W"  << std::dec << waferno << "_" << (char)((id_x-1) + 'A') << id_y;
  EUDAQ_USER("Timepix3 Chip ID (read from on-board EEPROM): " + convstream.str());


  m_chipID = myTimepix3Config->getChipID( device_id );
  EUDAQ_USER("Timepix3 Chip ID (read from XML config file): " + m_chipID);

  // Header filter mask:
  // ETH:   0000 1100 0111 0000
  // CPU:   1111 0011 1000 1111
  int eth_mask, cpu_mask;
  eth_mask = config->Get("eth_mask", 0xFFFF);
  cpu_mask = config->Get("cpu_mask", 0xF39F);
  if (!spidrctrl->setHeaderFilter(device_nr, eth_mask, cpu_mask)) {
    EUDAQ_ERROR("setHeaderFilter: "+ spidrctrl->errorString());
  }
  if (!spidrctrl->getHeaderFilter(device_nr, &eth_mask, &cpu_mask)) {
    EUDAQ_ERROR("getHeaderFilter: "+ spidrctrl->errorString());
  }
  // clear stringstream:
  convstream.str(std::string());
  for(int i = 15; i >= 0; i--) {
    convstream << ((eth_mask >> i) & 1);
  }
  EUDAQ_USER("ETH mask = 0b" + convstream.str());
  // clear stringstream:
  convstream.str(std::string());
  for(int i = 15; i >= 0; i--) {
    convstream << ((cpu_mask >> i) & 1);
  }
  EUDAQ_USER("CPU mask = 0b" + convstream.str());

  // DACs configuration
  if( !spidrctrl->setDacsDflt( device_nr ) ) {
    EUDAQ_ERROR("setDacsDflt: " + spidrctrl->errorString());
  } else {
    EUDAQ_USER("setDacsDflt: OK");
  }

  // Enable decoder
  if( !spidrctrl->setDecodersEna( 1 ) ) {
    EUDAQ_ERROR("setDecodersEna: " + spidrctrl->errorString());
  } else {
    EUDAQ_USER("setDecodersEna = 1");
  }

  // Pixel configuration
  if( !spidrctrl->resetPixels( device_nr ) ) {
    EUDAQ_ERROR("resetPixels: " + spidrctrl->errorString());
  } else {
    EUDAQ_USER("resetPixels: OK");
  }

  // Get DACs from XML config
  map< string, int > xml_dacs = myTimepix3Config->getDeviceDACs();
  map< string, int >::iterator xml_dacs_it;
  for( xml_dacs_it = xml_dacs.begin(); xml_dacs_it != xml_dacs.end(); ++xml_dacs_it ) {

    string name = xml_dacs_it->first;
    int XMLval = xml_dacs_it->second;
    int val = config->Get( name, XMLval ); // overwrite value from XML if it's uncommented in the conf

    if( TPX3_DAC_CODES.find( name ) != TPX3_DAC_CODES.end() ) {
      int daccode = TPX3_DAC_CODES.at( name );
      if( !spidrctrl->setDac( device_nr, daccode, val ) ) {
        cout << "Error, could not set DAC: " << name << " " << daccode << " " << val << endl;
      } else {
        int tmpval = -1;
        spidrctrl->getDac( device_nr, daccode, &tmpval );
        cout << "Successfully set DAC: " << name << " to " << tmpval << endl;
      }

    } else if( name == "VTHRESH" ) {
      m_xml_VTHRESH = val;
    } else if( name == "GeneralConfig" ) {
      if ( !spidrctrl->setGenConfig( device_nr, val ) ) {
        EUDAQ_ERROR("setGenConfig: " + spidrctrl->errorString());
      } else {
        int config = -1;

        spidrctrl->getGenConfig( device_nr, &config );
        cout << "Successfully set General Config to " << config << endl;
        // Unpack general config for human readable output
        myTimepix3Config->unpackGeneralConfig( config );
      }

    } else if( name == "PllConfig" ) {
      if ( !spidrctrl->setPllConfig( device_nr, val ) ) {
        EUDAQ_ERROR("setPllConfig: " + spidrctrl->errorString());
      } else {
        int config = -1;
        spidrctrl->getPllConfig( device_nr, &config );
        cout << "Successfully set PLL Config to " << config << endl;
      }

    } else if( name == "OutputBlockConfig" ) {
      if ( !spidrctrl->setOutBlockConfig( device_nr, val ) ) {
        EUDAQ_ERROR("setOutBlockConfig: " + spidrctrl->errorString());
      } else {
        int config = -1;
        spidrctrl->getOutBlockConfig( device_nr, &config );
        cout << "Successfully set Output Block Config to " << config << endl;
      }
    }
  }

  // Threshold
  int threshold = config->Get( "threshold", m_xml_VTHRESH );
  int coarse = threshold / 160;
  int fine = threshold - coarse*160 + 352;

  // Coarse threshold:
  if(!spidrctrl->setDac( device_nr, TPX3_VTHRESH_COARSE, coarse ) ) {
    EUDAQ_ERROR("Could not set VTHRESH_COARSE: " + spidrctrl->errorString());
  } else {
    int tmpval = -1;
    spidrctrl->getDac( device_nr, TPX3_VTHRESH_COARSE, &tmpval );
    EUDAQ_EXTRA("Successfully set DAC: VTHRESH_COARSE to " + std::to_string(tmpval));
  }

  // Fine threshold:
  if(!spidrctrl->setDac( device_nr, TPX3_VTHRESH_FINE, fine ) ) {
    EUDAQ_ERROR("Could not set VTHRESH_FINE: " + spidrctrl->errorString());
  } else {
    int tmpval = -1;
    spidrctrl->getDac( device_nr, TPX3_VTHRESH_FINE, &tmpval );
    EUDAQ_EXTRA("Successfully set DAC: VTHRESH_FINE to " + std::to_string(tmpval));
  }
  EUDAQ_USER("Threshold = " + std::to_string(threshold));

  // Reset entire matrix config to zeroes
  spidrctrl->resetPixelConfig();

  // Get per-pixel thresholds from XML
  bool pixfail = false;
  vector< vector< int > > matrix_thresholds = myTimepix3Config->getMatrixDACs();
  for( int x = 0; x < NPIXX; x++ ) {
    for ( int y = 0; y < NPIXY; y++ ) {
      int threshold = matrix_thresholds[y][x]; // x & y are inverted when parsed from XML
      // cout << x << " "<< y << " " << threshold << endl;
      if( !spidrctrl->setPixelThreshold( x, y, threshold ) ) pixfail = true;
    }
  }
  if( !pixfail ) {
    cout << "Successfully built pixel thresholds." << endl;
  } else {
    cout << "Something went wrong building pixel thresholds." << endl;
  }

  // Get pixel mask from XML
  bool maskfail = false;
  vector< vector< bool > > matrix_mask = myTimepix3Config->getMatrixMask();
  for( int x = 0; x < NPIXX; x++ ) {
    for ( int y = 0; y < NPIXY; y++ ) {
      bool mask = matrix_mask[y][x]; // x & y are inverted when parsed from XML
      if( !spidrctrl->setPixelMask( x, y, mask ) ) maskfail = true;
    }
  }
  // Add pixels masked by the user in the conf
  string user_mask = config->Get( "User_Mask", "" );
  vector<TString> pairs = tokenise( user_mask, ":");
  for( int k = 0; k < pairs.size(); ++k ) {
    vector<TString> pair = tokenise( pairs[k], "," );
    int x = pair[0].Atoi();
    int y = pair[1].Atoi();
    cout << "Additinal user mask: " << x << "," << y << endl;
    if( !spidrctrl->setPixelMask( x, y, true ) ) maskfail = true;
  }
  if( !maskfail ) {
    cout << "Successfully built pixel mask." << endl;
  } else {
    cout << "Something went wrong building pixel mask." << endl;
  }

  // Actually set the pixel thresholds and mask
  if( !spidrctrl->setPixelConfig( device_nr ) ) {
    EUDAQ_ERROR("setPixelConfig: " + spidrctrl->errorString());
  } else {
    cout << "Successfully set pixel configuration." << endl;
  }
  unsigned char *pixconf = spidrctrl->pixelConfig();
  int x, y, cnt = 0;
  if( spidrctrl->getPixelConfig( device_nr ) ) {
    for( y = 0; y < NPIXY; ++y ) {
      for( x = 0; x < NPIXX; ++x ) {
        std::bitset<6> bitconf( (unsigned int) pixconf[y*256+x] );
        //if( pixconf[y*256+x] != 0 ) {
        if ( bitconf.test(0) ) { /* masked? */
          //cout << x << ',' << y << ": " << bitconf << endl; // hex << setw(2) << setfill('0') << (unsigned int) pixconf[y*256+x] << dec << endl;
          ++cnt;
        }
      }
    }
    cout << "Pixels masked = " << cnt << endl;
  } else {
    EUDAQ_ERROR("getPixelConfig: " + spidrctrl->errorString());
  }

  // Create SpidrDaq
  spidrdaq = new SpidrDaq( spidrctrl );
  conf_done = true;
  // Also display something for us
  cout << endl;
  cout << "Timepix3 Producer configured. Ready to start run. " << endl;
  cout << endl;
}

double Timepix3Producer::getTpx3Temperature() {
  // Read band gap temperature, whatever that is
  int bg_temp_adc, bg_output_adc;
  if( !spidrctrl->setSenseDac( device_nr, TPX3_BANDGAP_TEMP ) ) {
    EUDAQ_ERROR("setSenseDac: " + spidrctrl->errorString());
  }

  if( !spidrctrl->getAdc( &bg_temp_adc, 64 ) ) {
    EUDAQ_ERROR("getAdc: " + spidrctrl->errorString());
  }

  float bg_temp_V = 1.5*( bg_temp_adc/64. )/4096;
  if( !spidrctrl->setSenseDac( device_nr, TPX3_BANDGAP_OUTPUT ) ) {
    EUDAQ_ERROR("setSenseDac: " + spidrctrl->errorString());
  }

  if( !spidrctrl->getAdc( &bg_output_adc, 64 ) ) {
    EUDAQ_ERROR("getAdc: " + spidrctrl->errorString());
  }

  float bg_output_V = 1.5*( bg_output_adc/64. )/4096;
  m_temp = 88.75 - 607.3 * ( bg_temp_V - bg_output_V);
  return m_temp;
}

void Timepix3Producer::DoStartRun() {
  m_ev = 0;

  std::cout << "Starting run..." << std::endl;

  double temp=getTpx3Temperature();
  EUDAQ_USER("getTpx3Temperature: " + std::to_string(temp));
  std::cout << "Started run." << std::endl;
  m_running = true;
}

void Timepix3Producer::DoStopRun() {

  std::cout << "Stopping run..." << std::endl;
  // Set a flag to signal to the polling loop that the run is over
  m_running = false;

  std::cout << "Stopped run." << std::endl;
}

void Timepix3Producer::DoTerminate() {
  std::cout << "Terminating..." << std::endl;

  // do not touch spidrctrl and spidrdaq if they are not initialized
  if (init_done) {
    // Disble TLU
    if( !spidrctrl->setTluEnable( device_nr, 0 ) ) {
      EUDAQ_ERROR("setTluEnable: " + spidrctrl->errorString());
    } else {
      EUDAQ_USER("setTluEnable = 0");
    }
  }

  // Clean up
  DoReset();
  std::cout << "Terminated." << std::endl;
}


void Timepix3Producer::RunLoop() {

  std::cout << "Starting run loop..." << std::endl;

  unsigned int m_ev_next_update=0;

  // Restart timers to sync Timepix3 and TLU timestamps
  if( !spidrctrl->restartTimers() ) {
    EUDAQ_ERROR("restartTimers: " + spidrctrl->errorString());
  } else {
    EUDAQ_USER("restartTimers: OK");
  }

  // Set Timepix3 acquisition mode
  if( !spidrctrl->datadrivenReadout() ) {
    EUDAQ_ERROR("datadrivenReadout: " + spidrctrl->errorString());
  } else {
    EUDAQ_USER("datadrivenReadout: OK");
  }

  // Sample pixel data
  spidrdaq->setSampling( true );
  spidrdaq->setSampleAll( true );

  // Open shutter
  if( !spidrctrl->openShutter() ) {
    EUDAQ_ERROR("openShutter: " + spidrctrl->errorString());
  } else {
    EUDAQ_USER("openShutter: OK");
  }

/*
  // Enable TLU
  if( !spidrctrl->setTluEnable( device_nr, 1 ) ) {
    EUDAQ_ERROR("setTluEnable: " + spidrctrl->errorString());
  } else {
    EUDAQ_USER("setTluEnable: OK");
  }
*/
  std::map<int, int> header_counter;

  while(1) {
    if(!m_running){
      break;
    }

    bool next_sample = true;

    if(spidrdaq->bufferFull() || spidrdaq->bufferFullOccurred()) {
      EUDAQ_ERROR("Buffer overflow");
    }

    // Log some info
    if(m_ev >= m_ev_next_update) {
      EUDAQ_USER("Timepix3 temperature: " + std::to_string(getTpx3Temperature()) + "°C");
      m_ev_next_update=m_ev+50000;
    }

    // Get a sample of pixel data packets, with timeout in ms
    const unsigned int BUF_SIZE = 8*1024*1024;
    next_sample = spidrdaq->getSample(BUF_SIZE, 1);

    if(next_sample) {
      auto size = spidrdaq->sampleSize();

      // look inside sample buffer...
      std::vector<eudaq::EventSPC> data_buffer;
      while(1) {
        uint64_t data = spidrdaq->nextPacket();

        // ...until the sample buffer is empty
        if(!data) break;

        uint64_t header = (data & 0xF000000000000000) >> 60;
        header_counter[header]++;

        if(m_ev%10000 == 0) {
          std::cout << "Headers: " << listVector(header_counter) << "\r";
        }

        // Data-driven or sequential readout pixel data header?
        if(header == 0x6) { // Or TLU packet header?

          // Send out pixel data accumulated so far:
          auto evup = eudaq::Event::MakeUnique("Timepix3RawEvent");
          for(auto& subevt : data_buffer) {
            evup->AddSubEvent(subevt);
          }
          SendEvent(std::move(evup));
          data_buffer.clear();
          m_ev++;

          // Create and send new trigger event
          auto evtrg = eudaq::Event::MakeUnique("Timepix3TrigEvent");
          evtrg->AddBlock(0, &data, sizeof(data));
          SendEvent(std::move(evtrg));
          m_ev++;
        } else {
          auto evup = eudaq::Event::MakeShared("Timepix3RawEvent");
          evup->AddBlock(0, &data, sizeof(data));
          data_buffer.push_back(evup);
        }

      } // End loop over sample buffer

      // Send remaining pixel data:
      if(!data_buffer.empty()) {
        auto evup = eudaq::Event::MakeUnique("Timepix3RawEvent");
        for(auto& subevt : data_buffer) {
          evup->AddSubEvent(subevt);
        }
        SendEvent(std::move(evup));
        data_buffer.clear();
        m_ev++;
      }
    } // Sample available
  } // Readout loop

  std::cout << "HEADERS: " << std::endl;
  for(auto& i : header_counter) {
    std::cout << i.first << ":\t" << i.second << std::endl;
  }
  std::cout << "Exiting run loop." << std::endl;
}
