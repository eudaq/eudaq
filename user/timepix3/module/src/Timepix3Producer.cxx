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
  string m_spidrIP, m_xmlfileName, m_time, m_chipID;
  Timepix3Config *myTimepix3Config;
  SpidrController *spidrctrl;
  SpidrDaq *spidrdaq;
  int m_xml_VTHRESH;
  float m_temp;

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
  spidrctrl->closeShutter();

  if(spidrctrl) {
    delete spidrctrl;
  }
  if(spidrdaq) {
    delete spidrdaq;
  }
}

void Timepix3Producer::DoInitialise() {
  auto config = GetInitConfiguration();

  // SPIDR-TPX3 IP & PORT
  m_spidrIP  = config->Get( "SPIDR_IP", "192.168.100.10" );
  int ip[4];
  vector<TString> ipstr = tokenise( m_spidrIP, ".");
  for (int i = 0; i < ipstr.size(); i++ ) ip[i] = ipstr[i].Atoi();
  m_spidrPort = config->Get( "SPIDR_Port", 50000 );

  cout << "Connecting to SPIDR at " << ip[0] << "." << ip[1] << "." << ip[2] << "." << ip[3] << ":" << m_spidrPort << endl;

  // Open a control connection to SPIDR-TPX3 module
  // with address 192.168.100.10, default port number 50000
  spidrctrl = new SpidrController( ip[0], ip[1], ip[2], ip[3], m_spidrPort );

  // Create SpidrDaq for later
  spidrdaq = new SpidrDaq( spidrctrl );
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

  // Reset Device
  if( !spidrctrl->reinitDevice( device_nr ) ) {
    EUDAQ_ERROR("reinitDevice: " + spidrctrl->errorString());
  }

  //Due to timing issue, set readout speed at 320 Mbps
  if( !spidrctrl->setReadoutSpeed( device_nr, 320) ) {
    EUDAQ_ERROR("setReadoutSpeed: " + spidrctrl->errorString());
  }

  // Are we connected to the SPIDR-TPX3 module?
  if( !spidrctrl->isConnected() ) {
    std::cout << spidrctrl->ipAddressString() << ": " << spidrctrl->connectionStateString() << ", " << spidrctrl->connectionErrString() << std::endl;
  } else {
    std::cout << "\n------------------------------" << std::endl;
    std::cout << "SpidrController is connected!" << std::endl;
    std::cout << "Class version: " << spidrctrl->versionToString( spidrctrl->classVersion() ) << std::endl;
    EUDAQ_USER("SPIDR Class:    " + spidrctrl->versionToString( spidrctrl->classVersion() ));

    int firmwVersion, softwVersion = 0;
    if( spidrctrl->getFirmwVersion( &firmwVersion ) ) {
      std::cout << "Firmware version: " << spidrctrl->versionToString( firmwVersion ) << std::endl;
      EUDAQ_USER("SPIDR Firmware: " + spidrctrl->versionToString( firmwVersion ));
    }

    if( spidrctrl->getSoftwVersion( &softwVersion ) ) {
      std::cout << "Software version: " << spidrctrl->versionToString( softwVersion ) << std::endl;
    EUDAQ_USER("SPIDR Software: " + spidrctrl->versionToString( softwVersion ));
    }
    std::cout << "------------------------------\n" << std::endl;
  }

  if (!spidrctrl->setExtRefClk(config->Get("external_clock", true))) {
    EUDAQ_ERROR("setExtRefClk"+ spidrctrl->errorString());
  }

  // DACs configuration
  if( !spidrctrl->setDacsDflt( device_nr ) ) {
    EUDAQ_ERROR("setDacsDflt: " + spidrctrl->errorString());
  }

  // Enable decoder
  if( !spidrctrl->setDecodersEna( 1 ) ) {
    EUDAQ_ERROR("setDecodersEna: " + spidrctrl->errorString());
  }

  // Pixel configuration
  if( !spidrctrl->resetPixels( device_nr ) ) {
    EUDAQ_ERROR("resetPixels: " + spidrctrl->errorString());
  }

  // Device ID
  int device_id = -1;
  if( !spidrctrl->getDeviceId( device_nr, &device_id ) ) {
    EUDAQ_ERROR("getDeviceId: " + spidrctrl->errorString());
  }
  cout << "Device ID: " << device_id << endl;


  // Header filter mask:
  // ETH:   0000110001100000
  // CPU:   1111001110011111
  int eth_mask = 0x0C60, cpu_mask = 0xF39F;
  if (!spidrctrl->setHeaderFilter(device_nr, eth_mask, cpu_mask)) {
    EUDAQ_ERROR("setHeaderFilter: "+ spidrctrl->errorString());
  }

  if (!spidrctrl->getHeaderFilter(device_nr, &eth_mask, &cpu_mask)) {
    EUDAQ_ERROR("getHeaderFilter: "+ spidrctrl->errorString());
  }
  std::ostringstream stream;
  for(int i = 15; i >= 0; i--) {
    stream << ((eth_mask >> i) & 1);
  }
  std::cout << "ETH mask: " << stream.str() << std::endl;
  std::ostringstream stream2;
  for(int i = 15; i >= 0; i--) {
    stream2 << ((cpu_mask >> i) & 1);
  }
  std::cout << "CPU mask: " << stream2.str() << std::endl;

  m_chipID = myTimepix3Config->getChipID( device_id );
  cout << "[Timepix3] Chip ID: " << m_chipID << endl;
  EUDAQ_USER("Timepix3 Chip ID: " + m_chipID);

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

  // Threshold
  int threshold = config->Get( "threshold_start", m_xml_VTHRESH );
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

  // Guess what this does?
  spidrctrl->closeShutter();

  // Disble TLU
  if( !spidrctrl->setTluEnable( device_nr, 0 ) ) {
    EUDAQ_ERROR("setTluEnable: " + spidrctrl->errorString());
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
  }

  // Set Timepix3 acquisition mode
  if( !spidrctrl->datadrivenReadout() ) {
    EUDAQ_ERROR("datadrivenReadout: " + spidrctrl->errorString());
  }

  // Sample pixel data
  spidrdaq->setSampling( true );
  spidrdaq->setSampleAll( true );

  // Open shutter
  if( !spidrctrl->openShutter() ) {
    EUDAQ_ERROR("openShutter: " + spidrctrl->errorString());
  }

  // Enable TLU
  if( !spidrctrl->setTluEnable( device_nr, 1 ) ) {
    EUDAQ_ERROR("setTluEnable: " + spidrctrl->errorString());
  }

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
      while(1) {
        uint64_t data = spidrdaq->nextPacket();

        // ...until the sample buffer is empty
        if(!data) break;

        uint64_t header = (data & 0xF000000000000000) >> 60;
        header_counter[header]++;

        std::cout << "Headers: " << listVector(header_counter) << "\r";

        // Data-driven or sequential readout pixel data header?
        if( header == 0xB || header == 0xA ) {
          auto evup = eudaq::Event::MakeUnique("Timepix3RawEvent");
          evup->AddBlock(0, &data, sizeof(data));
          SendEvent(std::move(evup));

        } else if( header == 0x6 ) { // Or TLU packet header?
          auto evup = eudaq::Event::MakeUnique("Timepix3TrigEvent");
          evup->AddBlock(0, &data, sizeof(data));
          SendEvent(std::move(evup));
        }

        m_ev++;
      } // End loop over sample buffer
    } // Sample available
  } // Readout loop

  std::cout << "HEADERS: " << std::endl;
  for(auto& i : header_counter) {
    std::cout << i.first << ":\t" << i.second << std::endl;
  }
  std::cout << "Exiting run loop." << std::endl;
}
