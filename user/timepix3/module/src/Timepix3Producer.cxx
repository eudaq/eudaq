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
  void RunLoop() override;

  static const uint32_t m_id_factory = eudaq::cstr2hash("Timepix3Producer");
private:
  std::thread ts_thread;   // thread for 1 sec timestamps
  bool m_running = false;
  bool m_init = false;
  bool m_config = false;
  double getTpx3Temperature(int device_nr = 0);
  void timestamp_thread();
  bool checkPixelBits(int *cnt_mask, int *cnt_test, int *cnt_thrs);

  SpidrController *spidrctrl = nullptr;
  SpidrDaq *spidrdaq = nullptr;
  std::mutex controller_mutex_;
  std::mutex daq_mutex_;

  int m_supported_devices = 0;
  int m_active_devices;
  int m_spidrPort;
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


  /** Helper function to return a printed list of an integer vector, used to shield
  *  debug code from being executed if debug level is not sufficient
  */
  template <typename T> std::string listVector(std::map<T, T> vec, std::string separator = ", ") {
    std::stringstream os;
    for(auto it : vec) {
      os << "0x" << to_hex_string(it.first, 1) << ": ";
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
: eudaq::Producer(name, runcontrol), m_running(false) {}

Timepix3Producer::~Timepix3Producer() {
  // force run stop
  m_running = false;
  // wait for all to finish
  std::lock_guard<std::mutex> lock_ctrl{controller_mutex_};
  if(spidrctrl) {
    delete spidrctrl;
    spidrctrl = nullptr;
  }
  std::lock_guard<std::mutex> lock_daq{daq_mutex_};
  if(spidrdaq) {
    delete spidrdaq;
    spidrdaq = nullptr;
  }

}

//----------------------------------------------------------
//  GET TEMPERATURE
//----------------------------------------------------------
double Timepix3Producer::getTpx3Temperature(int device_nr) {
  // Read band gap temperature, whatever that is
  int bg_temp_adc, bg_output_adc;
  if( !spidrctrl->setSenseDac( device_nr, TPX3_BANDGAP_TEMP ) ) {
    EUDAQ_ERROR("getTpx3Temperature - setSenseDac: " + spidrctrl->errorString());
  }
  usleep(100000);

  if( !spidrctrl->getAdc( &bg_temp_adc, 64 ) ) {
    EUDAQ_ERROR("getTpx3Temperature - getAdc: " + spidrctrl->errorString());
  }

  float bg_temp_V = 1.5*( bg_temp_adc/64. )/4096;
  if( !spidrctrl->setSenseDac( device_nr, TPX3_BANDGAP_OUTPUT ) ) {
    EUDAQ_ERROR("getTpx3Temperature - setSenseDac: " + spidrctrl->errorString());
  }
  usleep(100000);

  if( !spidrctrl->getAdc( &bg_output_adc, 64 ) ) {
    EUDAQ_ERROR("getTpx3Temperature - getAdc: " + spidrctrl->errorString());
  }

  float bg_output_V = 1.5*( bg_output_adc/64. )/4096;
  m_temp = 88.75 - 607.3 * ( bg_temp_V - bg_output_V);
  return m_temp;
} // getTpx3Temperature()

//----------------------------------------------------------
//  PERIODIC REQUEST FOR TIMESTAMPS ETC.
//----------------------------------------------------------
void Timepix3Producer::timestamp_thread() {
    unsigned int timer_lo1, timer_hi1;
    unsigned int cnt = 0;

    while (m_running) {
        usleep(200000);
        // request full timestamps
        for (int device_nr=0; device_nr<m_active_devices; device_nr++){
          std::lock_guard<std::mutex> lock{controller_mutex_};
          if( !spidrctrl->getTimer(0, &timer_lo1, &timer_hi1) ) {
  		        EUDAQ_ERROR("getTimer: " + spidrctrl->errorString());
          }
        }
        usleep(200000);
        usleep(200000);
        if ( !(cnt & 0xFF) ) {
          // update temperature every 16th cycle only
          std::lock_guard<std::mutex> lock_ctrl{controller_mutex_};
          getTpx3Temperature(0);
        } else {
          // sleep when no temperature measurement is done,
          // because getTpx3Temperature() contains 2x 100 ms sleep
          usleep(200000);
        }
        usleep(200000);
        cnt++;
    }
    return;
}


//----------------------------------------------------------
//  READ AND COUNT PIXEL MASK BITS
//----------------------------------------------------------
bool Timepix3Producer::checkPixelBits(int *cnt_mask, int *cnt_test, int *cnt_thrs) {
  // check if all is reset
  unsigned char *pixcfgmatrix = spidrctrl->pixelConfig();
  int x, y;
  bool notzero = false;
  (*cnt_mask) = 0;
  (*cnt_test) = 0;
  (*cnt_thrs) = 0;
  for( y = 0; y < NPIXY; ++y ) {
    for( x = 0; x < NPIXX; ++x ) {
      unsigned char pixconf = ( pixcfgmatrix[y*NPIXX+x] );
      if (pixconf == 0 ) {
        continue;
      } else {
        notzero = true;
        // masked?
        if ( pixconf & TPX3_PIXCFG_MASKBIT ) {
          (*cnt_mask)++;
        }
        // injection enabled?
        if ( pixconf & TPX3_PIXCFG_TESTBIT ) {
          (*cnt_test)++;
        }
        // threshold set?
        if ( pixconf & TPX3_PIXCFG_THRESH_MASK ) {
          (*cnt_thrs)++;
        }
      }
    }
  }
  return notzero;
}

//----------------------------------------------------------
//  RESET
//----------------------------------------------------------
void Timepix3Producer::DoReset() {
  EUDAQ_USER("Timepix3Producer is going to be reset.");
  // stop the run, if it is still running
  m_running = false;
  // wait for other functions to finish
  std::lock_guard<std::mutex> lock_ctrl{controller_mutex_};
  std::lock_guard<std::mutex> lock_daq{daq_mutex_};
  m_init = false;
  m_config = false;

  if(spidrctrl) {
    delete spidrctrl;
    spidrctrl = nullptr;
  }

  if(spidrdaq) {
    delete spidrdaq;
    spidrdaq = nullptr;
  }
  EUDAQ_USER("Timepix3Producer was reset.");
} // DoReset()

//----------------------------------------------------------
//  INIT
//----------------------------------------------------------
void Timepix3Producer::DoInitialise() {
  bool serious_error = false;
  if (m_init || m_config || m_running) {
    EUDAQ_WARN("Timepix3Producer: Initializing while it is already in initialized state. Performing reset first...");
    DoReset();
  }

  auto config = GetInitConfiguration();

  EUDAQ_USER("Timepix3Producer initializing with configuration: " + config->Name());


  // SPIDR IP & PORT
  m_spidrIP  = config->Get( "SPIDR_IP", "192.168.100.10" );
  int ip[4];
  if (!tokenize_ip_addr(m_spidrIP, ip) ) {
      EUDAQ_ERROR("Incorrect SPIDR IP address: " + m_spidrIP);
  }
  m_spidrPort = config->Get( "SPIDR_Port", 50000 );

  // Open a control connection to SPIDR-TPX3 module
  std::lock_guard<std::mutex> lock{controller_mutex_};
  spidrctrl = new SpidrController( ip[0], ip[1], ip[2], ip[3], m_spidrPort );

  // reset SPIDR
  int errstat;
  if( !spidrctrl->reset( &errstat ) ) {
    EUDAQ_ERROR("reset ERROR: " + spidrctrl->errorString());
    serious_error = true;
  } else if (errstat && errstat != 0x6000) {
    EUDAQ_ERROR("reset not complete, error code: 0x" + to_hex_string(errstat));
  } else {
    EUDAQ_EXTRA("reset: OK" );
  }

  // Are we connected to the SPIDR?
  if (!spidrctrl->isConnected()) {
    EUDAQ_THROW("Connection to SPIDR failed at: " + spidrctrl->ipAddressString() + "; status: "
     + spidrctrl->connectionStateString() + ", " + spidrctrl->connectionErrString());
     return;
  } else {
    EUDAQ_INFO("SPIDR is connected at " + spidrctrl->ipAddressString() + "; status: " + spidrctrl->connectionStateString());
    EUDAQ_INFO("SPIDR Class:    " + spidrctrl->versionToString( spidrctrl->classVersion() ));

    int firmwVersion, softwVersion = 0;
    if( spidrctrl->getFirmwVersion( &firmwVersion ) ) {
      EUDAQ_INFO("SPIDR Firmware: " + spidrctrl->versionToString( firmwVersion ));
    }

    if( spidrctrl->getSoftwVersion( &softwVersion ) ) {
      EUDAQ_INFO("SPIDR Software: " + spidrctrl->versionToString( softwVersion ));
    }
  }

  // determine number of devices, does not check if devices are active
  if ( !spidrctrl->getDeviceCount( &m_supported_devices ) ) {
    EUDAQ_ERROR( "getDeviceCount" + spidrctrl->errorString());
    serious_error = true;
  }
  if (m_supported_devices == 0) {
    EUDAQ_ERROR("SPIDR returned zero as a numer of supported devices. This is weird.");
  } else {
    EUDAQ_EXTRA("Number of devices supported by firmware: " + std::to_string(m_supported_devices));
  }

  // set number of active devices
  m_active_devices = config->Get("active_devices", 1);
  EUDAQ_INFO("Number of active devices set in configuration: " + std::to_string(m_active_devices));
  if (m_supported_devices<m_active_devices) {
    m_active_devices = m_supported_devices;
    EUDAQ_WARN("You defined more active devices than what the system supports. Changing it to " + std::to_string(m_active_devices));
  }
  if (m_active_devices<1) {
    EUDAQ_WARN("You defined less than one active device.");
    m_active_devices = 0;
  }
  if (m_active_devices>1) {
    EUDAQ_WARN("Only one active device is supported in this EUDAQ producer version.");
    m_active_devices = 1;
  }

  if (serious_error) {
    EUDAQ_THROW("Timepix3Producer: There were major errors during initialization. See the log.");
    return;
  }
  m_init = true;
  EUDAQ_USER("Timepix3Producer Init done");
} // DoInitialise()

//----------------------------------------------------------
//  CONFIGURE
//----------------------------------------------------------
void Timepix3Producer::DoConfigure() {
  bool serious_error = false;
  if (!m_init) {
    EUDAQ_THROW("DoConfigure: Trying to configure an uninitialized module. This will not work.");
    return;
  }
  if (m_running) {
    EUDAQ_WARN("DoConfigure: Trying to configure a running module. Trying to stop it first.");
    m_running = false;
  }

  auto myTimepix3Config = new Timepix3Config();


  std::lock_guard<std::mutex> lock{controller_mutex_};
  if (!spidrctrl) {
    EUDAQ_THROW("DoConfigure: spidrctrl was not initialized. Can not configure it.");
    return;
  }

  auto config = GetConfiguration();
  EUDAQ_USER("Timepix3Producer configuring: " + config->Name());

  // Configuration file values are accessible as config->Get(name, default)
  config->Print();

  // sleep for 1 second, to make sure the TLU clock is already present
  sleep (1);

  // set whether external clock (TLU) is used or device runs standalone
  m_extRefClk = config->Get("external_clock", false);
  if (!spidrctrl->setExtRefClk(m_extRefClk)) {
    EUDAQ_ERROR("setExtRefClk: " + spidrctrl->errorString());
    serious_error = true;
  } else {
    EUDAQ_INFO("setExtRefClk = " + (m_extRefClk ? std::string("true") : std::string("false")));
  }
  // assume that if we set an external clock we also provide external t0,
  // but leave the option to configure it
  m_extT0 = config->Get("external_t0", m_extRefClk);
  EUDAQ_INFO("external_T0 = " + (m_extT0 ? std::string("true") : std::string("false")));

  // Resets All connected Timepix3 Devices
  if( !spidrctrl->reinitDevices() ) {
    EUDAQ_ERROR("reinitDevices: " + spidrctrl->errorString());
    serious_error = true;
  } else {
    EUDAQ_DEBUG("reinitDevices: OK" );
  }

  for (int device_nr=0; device_nr<m_active_devices; device_nr++){
    EUDAQ_INFO("Configuring device " + std::to_string(device_nr));

    //Due to timing issue, set readout speed at 320 Mbps
    if( !spidrctrl->setReadoutSpeed( device_nr, 320) ) {
      EUDAQ_ERROR("setReadoutSpeed: " + spidrctrl->errorString());
      serious_error = true;
    } else {
      EUDAQ_DEBUG("setReadoutSpeed = 320");
    }

    // set output mask
    if( !spidrctrl->setOutputMask(device_nr, 0xFF) ) {
      EUDAQ_ERROR("setOutputMask: " + spidrctrl->errorString());
      serious_error = true;
    } else {
      EUDAQ_DEBUG("setOutputMask = 0xFF");
    }

    // set destination (DAQ PC) IP address
    std::string default_daqIP = m_spidrIP;
    while ( default_daqIP.back()!='.' && !default_daqIP.empty() ) {
      default_daqIP.pop_back();
    }
    default_daqIP.push_back('1');
    m_daqIP  = config->Get( "DAQ_IP", default_daqIP );
    int ip[4];
    if (!tokenize_ip_addr(m_daqIP, ip) ) {
        EUDAQ_ERROR("Incorrect SPIDR IP address: " + m_spidrIP);
    }
    int newDestIP  = ((ip[3] & 0xFF) | ((ip[2] & 0xFF) << 8) | ((ip[1] & 0xFF) << 16) | ((ip[0] & 0xFF) << 24) );
    EUDAQ_DEBUG("Setting destination (DAQ) address to " + std::to_string(ip[0]) + "." + std::to_string(ip[1]) + "."
     + std::to_string(ip[2]) + "." + std::to_string(ip[3]));
    if( !spidrctrl->setIpAddrDest( device_nr, newDestIP ) ) {
      EUDAQ_ERROR("setIpAddrDest: " + spidrctrl->errorString());
      serious_error = true;
    } else {
      int addr;
      if ( !spidrctrl->getIpAddrDest( device_nr, &addr ) ) {
        EUDAQ_ERROR( "getIpAddrDest: " + spidrctrl->errorString());
        serious_error = true;
      } else {
        EUDAQ_DEBUG("getIpAddrDest: " + std::to_string((addr >> 24) & 0xFF) + "."  + std::to_string((addr >> 16) & 0xFF) +
        "."  + std::to_string((addr >> 8) & 0xFF) + "."  + std::to_string(addr  & 0xFF) );
      }
    }

    // check link status
    int linkstatus;
    if( !spidrctrl->getLinkStatus( device_nr, &linkstatus ) ) {
      EUDAQ_ERROR( "getLinkStatus: " + spidrctrl->errorString());
      serious_error = true;
    } else {
      // Link status: bits 0-7: 0=link enabled; bits 16-23: 1=link locked
      int links_enabled = (~linkstatus) & 0xFF;
      int links_locked  = (linkstatus & 0xFF0000) >> 16;
      EUDAQ_DEBUG("Links enabled: 0b" + to_bit_string(links_enabled, 8));
      EUDAQ_DEBUG("Links locked : 0b" + to_bit_string(links_locked, 8));
      if (links_locked != links_enabled) {
        EUDAQ_ERROR("Timepix3Producer DoInitialise: Locked links do not correspond to enabled links.");
        serious_error = true;
      }
    }

    // get Device ID
    int device_id = -1;
    if( !spidrctrl->getDeviceId( device_nr, &device_id ) ) {
      EUDAQ_ERROR("getDeviceId: " + spidrctrl->errorString());
      serious_error = true;
    } else {
      EUDAQ_DEBUG("getDeviceId: 0x" + to_hex_string(device_id));
    }
    int waferno = (device_id >> 8) & 0xFFF;
    int id_y = (device_id >> 4) & 0xF;
    int id_x = (device_id >> 0) & 0xF;
    m_chipID = std::to_string(waferno) + "_" + (char)((id_x-1) + 'A') + std::to_string(id_y);
    EUDAQ_INFO("Timepix3 Chip ID (read from Timepix3 chip): W" + m_chipID);

    // set header filter mask
    int eth_mask, cpu_mask;
    eth_mask = config->Get("eth_mask", 0x0C70);
    cpu_mask = config->Get("cpu_mask", 0xF39F);
    if (!spidrctrl->setHeaderFilter(device_nr, eth_mask, cpu_mask)) {
      EUDAQ_ERROR("setHeaderFilter: "+ spidrctrl->errorString());
      serious_error = true;
    }
    if (!spidrctrl->getHeaderFilter(device_nr, &eth_mask, &cpu_mask)) {
      EUDAQ_ERROR("getHeaderFilter: "+ spidrctrl->errorString());
      serious_error = true;
    }
    EUDAQ_EXTRA("ETH mask = 0b" + to_bit_string(eth_mask, 16));
    EUDAQ_EXTRA("CPU mask = 0b" + to_bit_string(cpu_mask, 16));

    // DACs configuration default (might be skipped, already done in reinitDevices())
    if( !spidrctrl->setDacsDflt( device_nr ) ) {
      EUDAQ_ERROR("setDacsDflt: " + spidrctrl->errorString());
      serious_error = true;
    } else {
      EUDAQ_DEBUG("setDacsDflt: OK");
    }

    // Enable decoder
    if( !spidrctrl->setDecodersEna( true ) ) {
      EUDAQ_ERROR("setDecodersEna: " + spidrctrl->errorString());
      serious_error = true;
    } else {
      EUDAQ_DEBUG("setDecodersEna: OK");
    }

    EUDAQ_INFO("Device temperature: " + std::to_string(getTpx3Temperature(device_nr)) + " °C");

    // Load Timepix3 XML config file
    m_xmlfileName = config->Get( "XMLConfig_dev" + std::to_string(device_nr), "" );
    if (m_xmlfileName.empty()) {
      EUDAQ_DEBUG("Did not find configuration entry for \"XMLConfig_dev" + std::to_string(device_nr) + "\". Trying to use \"XMLConfig\".");
      m_xmlfileName = config->Get( "XMLConfig", "" );
    }
    if (m_xmlfileName.empty()) {
      EUDAQ_THROW("Timepix3Producer: Empty XML configuration file name.");
    }

    myTimepix3Config->ReadXMLConfig( m_xmlfileName );
    EUDAQ_INFO("Reading configuration file " + m_xmlfileName );
    std::string conftime = myTimepix3Config->getTime();
    EUDAQ_INFO("Configuration file created on: " + conftime);

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
          EUDAQ_ERROR("Could not set DAC: " + name + " " + std::to_string(daccode) + " " + std::to_string(val) );
          serious_error = true;
        } else {
          int readval = -1;
          spidrctrl->getDac( device_nr, daccode, &readval );
          if (val == readval) {
            EUDAQ_INFO("DAC \"" + name + "\" was successfully set to " + std::to_string(readval) );
          } else {
            EUDAQ_ERROR("DAC \"" + name + "\" was set to " + std::to_string(val) + " but readback value was "  + std::to_string(readval) );
          }
        }

      }
      else if( name == "VTHRESH" ) {
        m_xml_VTHRESH = val;
      }
      else if( name == "GeneralConfig" ) {
        if ( !spidrctrl->setGenConfig( device_nr, val ) ) {
          EUDAQ_ERROR("setGenConfig: " + spidrctrl->errorString());
          serious_error = true;
        } else {
          int readval = -1;
          spidrctrl->getGenConfig( device_nr, &readval );
          if (val == readval) {
            EUDAQ_INFO("General Config register was successfully set to 0x" + to_hex_string(readval, 4) );
          } else {
            EUDAQ_ERROR("General Config register was set to 0x" + to_hex_string(val, 4) + " but readback value was 0x"  + to_hex_string(readval, 4) );
          }
        }
      }
      else if( name == "PllConfig" ) {
        if ( !spidrctrl->setPllConfig( device_nr, val ) ) {
          EUDAQ_ERROR("setPllConfig: " + spidrctrl->errorString());
          serious_error = true;
        } else {
          int readval = -1;
          spidrctrl->getPllConfig( device_nr, &readval );
          if (val == readval) {
            EUDAQ_INFO("PLL Config register was successfully set to 0x" + to_hex_string(readval, 4) );
          } else {
            EUDAQ_ERROR("PLL Config register was set to 0x" + to_hex_string(val, 4) + " but readback value was 0x"  + to_hex_string(readval, 4) );
          }
        }
      }
      else if( name == "OutputBlockConfig" ) {
        if ( !spidrctrl->setOutBlockConfig( device_nr, val ) ) {
          EUDAQ_ERROR("setOutBlockConfig: " + spidrctrl->errorString());
          serious_error = true;
        } else {
          int readval = -1;
          spidrctrl->getOutBlockConfig( device_nr, &readval );
          if (val == readval) {
            EUDAQ_INFO("Output Block Config register was successfully set to 0x" + to_hex_string(readval, 4) );
          } else {
            EUDAQ_ERROR("Output Block Config register was set to 0x" + to_hex_string(val, 4) + " but readback value was 0x"  + to_hex_string(readval, 4) );
          }
        }
      }
    }

    // Threshold
    int threshold = config->Get( "threshold", m_xml_VTHRESH );
    int coarse = threshold / 160;
    int fine = threshold - coarse*160 + 352;
    EUDAQ_INFO("Setting threshold to " + std::to_string(threshold));

    // Coarse threshold:
    if(!spidrctrl->setDac( device_nr, TPX3_VTHRESH_COARSE, coarse ) ) {
      EUDAQ_ERROR("Could not set VTHRESH_COARSE: " + spidrctrl->errorString());
      serious_error = true;
    } else {
      int readval = -1;
      spidrctrl->getDac( device_nr, TPX3_VTHRESH_COARSE, &readval );
      if (coarse == readval) {
        EUDAQ_DEBUG("VTHRESH_COARSE was successfully set to 0x" + to_hex_string(readval, 4) );
      } else {
        EUDAQ_ERROR("VTHRESH_COARSE was set to 0x" + to_hex_string(coarse, 4) + " but readback value was 0x"  + to_hex_string(readval, 4) );
      }
    }

    // Fine threshold:
    if(!spidrctrl->setDac( device_nr, TPX3_VTHRESH_FINE, fine ) ) {
      EUDAQ_ERROR("Could not set VTHRESH_FINE: " + spidrctrl->errorString());
      serious_error = true;
    } else {
      int readval = -1;
      spidrctrl->getDac( device_nr, TPX3_VTHRESH_FINE, &readval );
      if (fine == readval) {
        EUDAQ_DEBUG("VTHRESH_FINE was successfully set to 0x" + to_hex_string(readval, 4) );
      } else {
        EUDAQ_ERROR("VTHRESH_FINE was set to 0x" + to_hex_string(fine, 4) + " but readback value was 0x"  + to_hex_string(readval, 4) );
      }
    }

    // Pixel reset (might be skipped, already done in reinitDevices())
    // Does NOT reset pixel configuration bits. This still needs to be done
    //  manually by setting all bits explicitely to zero.
    if( !spidrctrl->resetPixels( device_nr ) ) {
      EUDAQ_ERROR("resetPixels: " + spidrctrl->errorString());
      serious_error = true;
    } else {
      EUDAQ_DEBUG("resetPixels: OK");
    }

    // select default set of local pixel configuration
    spidrctrl->selectPixelConfig(0);
    // Set all pixel configuration bits to zero
    // It is required to properly reset the pixel matrix and
    //  resetPixels() does not do that.
    spidrctrl->resetPixelConfig();
    // Upload the pixel configuration to the device
    if( !spidrctrl->setPixelConfig( device_nr ) ) {
      EUDAQ_ERROR("setPixelConfig: " + spidrctrl->errorString());
      serious_error = true;
    } else {
      // read pixel configuration from device
      if( !spidrctrl->getPixelConfig( device_nr ) ) {
        EUDAQ_ERROR("getPixelConfig: " + spidrctrl->errorString());
        serious_error = true;
      } else {
        int cnt_mask;
        int cnt_test;
        int cnt_thrs;
        if( checkPixelBits(&cnt_mask, &cnt_test, &cnt_thrs) ) {
          EUDAQ_ERROR("Pixel configuration should be all zeros, but is not. Pixels with mask set: "
                      + std::to_string(cnt_mask) + ", testbit set: " + std::to_string(cnt_test)
                      + " threshold set: " + std::to_string(cnt_thrs));
          serious_error = true;
        } else {
          EUDAQ_DEBUG("Pixel configuration sucessfully set to zero and read back.");
        }
      }
    }

    // set pixels according to an XML file
    // clear all
    spidrctrl->resetPixelConfig();

    // Get per-pixel thresholds from XML
    bool pixfail = false;
    vector< vector< int > > matrix_thresholds = myTimepix3Config->getMatrixDACs();
    for( int x = 0; x < NPIXX; x++ ) {
      for ( int y = 0; y < NPIXY; y++ ) {
        int threshold = matrix_thresholds[y][x]; // x & y are inverted when parsed from XML
         /*
         if (threshold) {
           cout << "threshold tune: " << x << " "<< y << " -> " << threshold << endl;
         }
         */
        if( !spidrctrl->setPixelThreshold( x, y, threshold ) ) pixfail = true;
      }
    }
    if( !pixfail ) {
      EUDAQ_DEBUG("Successfully loaded pixel thresholds.");
    } else {
      EUDAQ_ERROR("Something went wrong while building pixel thresholds.");
      serious_error = true;
    }

    // Get pixel mask from XML
    pixfail = false;
    vector< vector< bool > > matrix_mask = myTimepix3Config->getMatrixMask();
    for( int x = 0; x < NPIXX; x++ ) {
      for ( int y = 0; y < NPIXY; y++ ) {
        bool mask = matrix_mask[y][x]; // x & y are inverted when parsed from XML
        /*
        if (mask) {
          cout << "mask: " << x << " "<< y << endl;
        }
        */
        if( !spidrctrl->setPixelMask( x, y, mask ) ) pixfail = true;
      }
    }

    auto tokenise = [](const std::string& istring, const char separator) {
      std::vector<std::string> tokens;
      if(istring.size()<1) {
        return tokens;
      }
      tokens.push_back("");
      for (int i=0; i<istring.size(); i++) {
        if (istring[i] == separator) {
          tokens.push_back("");
        }
        else {
          tokens.back().push_back(istring[i]);
        }
      }
      return tokens;
    };

    // Add pixels masked by the user in the conf
    string user_mask = config->Get( "User_Mask", "" );
    vector<string> conf_tmp = tokenise( user_mask, ':');
    for( int k = 0; k < conf_tmp.size(); ++k ) {
      vector<string> pair = tokenise( conf_tmp[k], ',' );
      int x = std::stoi(pair[0]);
      int y = std::stoi(pair[1]);
      EUDAQ_INFO("Additional user mask: " + std::to_string(x) + "," + std::to_string(y) );
      if( !spidrctrl->setPixelMask( x, y, true ) ) pixfail = true;
    }
    if( !pixfail ) {
      EUDAQ_DEBUG("Successfully loaded pixel mask.");
    } else {
      EUDAQ_ERROR("Something went wrong while building pixel mask.");
      serious_error = true;
    }

    // enable testpulse to pixels
    string user_testpix = config->Get( "User_TestPix", "" );
    conf_tmp.clear();
    conf_tmp = tokenise( user_testpix, ':');
    for( int k = 0; k < conf_tmp.size(); ++k ) {
      vector<string> pair = tokenise( conf_tmp[k], ',' );
      int x = std::stoi(pair[0]);
      int y = std::stoi(pair[1]);
      EUDAQ_INFO("Enabling testpulse to pixel: " + std::to_string(x) + "," + std::to_string(y) );
      if( !spidrctrl->setPixelTestEna( x, y, true ) ) {
        EUDAQ_ERROR("Something went wrong while building testpulse-enabled pixel matrix.");
        serious_error = true;
      }
    }

    // Upload the pixel configuration to the device
    // Actually set the pixel thresholds and mask
    if( !spidrctrl->setPixelConfig( device_nr ) ) {
      EUDAQ_ERROR("setPixelConfig: " + spidrctrl->errorString());
      serious_error = true;
    } else {
      EUDAQ_DEBUG("SetPixelConfig: OK");
    }

    // select second local pixel configuration set for reading the data from chip
    spidrctrl->selectPixelConfig(1);
    // read pixel configuration from device
    if( !spidrctrl->getPixelConfig( device_nr ) ) {
      EUDAQ_ERROR("getPixelConfig: " + spidrctrl->errorString());
      serious_error = true;
    } else {
      EUDAQ_DEBUG("getPixelConfig: OK");
      if (0 != spidrctrl->comparePixelConfig(0,1)) {
        EUDAQ_ERROR("Pixel configuration that was read back from the chip does not correspond to what was written in it.");
        serious_error = true;
      } else {
        EUDAQ_DEBUG("Successfully set and read back new pixel configuration.");
      }
      int cnt_mask;
      int cnt_test;
      int cnt_thrs;
      checkPixelBits(&cnt_mask, &cnt_test, &cnt_thrs);
      EUDAQ_INFO("Read from chip: Pixels with mask set: " + std::to_string(cnt_mask)
      + ", testbit set: " + std::to_string(cnt_test)
      + " threshold set: " + std::to_string(cnt_thrs) );
    }
    // return to default pixel configuration set
    spidrctrl->selectPixelConfig(0);

    // enable testpulse to columns - set CTPR
    string user_testcols = config->Get( "User_TestCols", "" );
    conf_tmp.clear();
    conf_tmp = tokenise( user_testcols, ':');
    for( int k = 0; k < conf_tmp.size(); ++k ) {
      int col = std::stoi(conf_tmp[k]);
      EUDAQ_INFO("Enabling testpulse in CTPR to column: " + std::to_string(col));
      if( !spidrctrl->setCtprBit( col ) ) {
        EUDAQ_ERROR("Something went wrong while enabling column in CTPR.");
        serious_error = true;
      }
    }
    // Upload test-pulse register to the device
    if( !spidrctrl->setCtpr( device_nr ) ) {
      EUDAQ_ERROR( "setCtpr: " + spidrctrl->errorString());
      serious_error = true;
    }

  }
  if (serious_error) {
    EUDAQ_THROW("Timepix3Producer: There were major errors during configuration. See the log.");
    return;
  }
  m_config = true;
  // Also display something for us
  EUDAQ_USER("Timepix3Producer configured. Ready to start run. ");
} // DoConfigure()

//----------------------------------------------------------
//  START RUN
//----------------------------------------------------------
void Timepix3Producer::DoStartRun() {
  if (!(m_init && m_config)){
    m_running = false;
    EUDAQ_THROW("DoStartRun: Timepix3 producer is not configured properly. I can not start the run.");
    return;
  }
  if (m_running) {
    m_running = false;
    EUDAQ_WARN("DoStartRun: Timepix3 producer is already running. I'm trying to stop if first. This might however create a mess in the runs.");
  }
  EUDAQ_DEBUG("DoStartRun: Locking mutex...");

  // Create SpidrDaq
  std::lock_guard<std::mutex> lock_ctrl{controller_mutex_};
  std::lock_guard<std::mutex> lock_daq{daq_mutex_};
  if (m_running) {
    EUDAQ_ERROR("DoStartRun: Oh no, the producer is running again, although I just stopped it. Something is really wrong here. I'm giving it up.");
    return;
  }
  spidrdaq = new SpidrDaq( spidrctrl );


  EUDAQ_USER("Timepix3Producer starting run...");

  // Start timer internally, if configured to do so
  if( !spidrctrl->restartTimers() ) {
    EUDAQ_THROW( "Could not send restartTimers command: " + spidrctrl->errorString());
  } else {
    EUDAQ_DEBUG( "restartTimers: OK");
  }

  // Set Timepix3 into acquisition mode
  if( !spidrctrl->datadrivenReadout() ) {
    EUDAQ_THROW( "Could not start data driven readout: " + spidrctrl->errorString());
  } else {
    EUDAQ_DEBUG( "datadrivenReadout: OK");
  }

  // Open the shutter
  if( !spidrctrl->openShutter() ) {
    EUDAQ_THROW( "Could not open the shutter: " + spidrctrl->errorString());
  } else {
    EUDAQ_DEBUG( "openShutter: OK");
  }

  if (!m_extT0) {
    int device_nr = 0;
    // Restart timers to sync Timepix3 and TLU timestamps. Resets both SPIDR and Timepix3 timers.
    if( !spidrctrl->t0Sync(device_nr) ) {
      EUDAQ_THROW( "Could not send T0: " + spidrctrl->errorString());
    } else {
      EUDAQ_INFO( "Internal T0 generated.");
    }
  } else {
    EUDAQ_INFO( "External T0 signal is expected, NOT generating it internally.");
  }

  m_running = true;

  EUDAQ_USER("Timepix3Producer started run.");
} // DoStartRun()

//----------------------------------------------------------
//  RUN LOOP
//----------------------------------------------------------
void Timepix3Producer::RunLoop() {
  int start_attempts = 0;
  while (!(m_running && spidrdaq)) {
    if (++start_attempts > 3) {
      EUDAQ_THROW( "RunLoop: DoStartRun() was not executed properly before calling RunLoop(). Aborting run loop." );
      return;
    }
    EUDAQ_WARN( "RunLoop: Waiting to finish DoStartRun(). Attempt no. " + std::to_string(start_attempts));
    sleep(1);
  }

  std::lock_guard<std::mutex> lock{daq_mutex_};
  EUDAQ_USER("Timepix3Producer starting run loop...");

  // create a thread to periodically ask for full timestamps, temperatures and others
  ts_thread = std::thread(&Timepix3Producer::timestamp_thread, this);

  std::map<int, int> header_counter;

  // set sampling parameters
  spidrdaq->setSampleAll( true );
  // Sample pixel data
  spidrdaq->setSampling( true );

  while(m_running) {

    if(spidrdaq->bufferFullOccurred()) {
      EUDAQ_WARN("Buffer overflow");
    }

    // Get a sample of pixel data packets, with timeout in ms
    const unsigned int BUF_SIZE = 8*1024*1024;
    bool next_sample = spidrdaq->getSample(BUF_SIZE, 300);

    if(next_sample) {
      auto size = spidrdaq->sampleSize();
      // look inside sample buffer...
      std::vector<eudaq::EventSPC> data_buffer;

      // ...until the sample buffer is empty
      while(uint64_t data = spidrdaq->nextPacket()) {

        // Break if we're supposed to stop:
        if(!m_running) break;

        uint64_t header = (data & 0xF000000000000000) >> 60;
        header_counter[header]++;

        // it's TDC counter
        if(header == 0x6) {
          // Send out pixel data accumulated so far:
          auto evup = eudaq::Event::MakeUnique("Timepix3RawEvent");
          for(auto& subevt : data_buffer) {
            evup->AddSubEvent(subevt);
          }
          SendEvent(std::move(evup));
          data_buffer.clear();

          // Create and send new trigger event
          auto evtrg = eudaq::Event::MakeUnique("Timepix3TrigEvent");
          evtrg->AddBlock(0, &data, sizeof(data));
          SendEvent(std::move(evtrg));

        } else {
          // pixel data OR timestamp (OR something else)
          // add it to the data_buffer
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
      }
    } // Sample available
  } // Readout loop

  // Stop sampling
  spidrdaq->setSampling( false );
  // kill timestamp thread
  m_running = false; // just in case there was another way how to get from the run loop above...
  ts_thread.join();
  ts_thread.~thread();
  // show header statitstics
  std::cout << "Headers: " << listVector(header_counter) << endl;
  EUDAQ_USER("Timepix3Producer exiting run loop.");
} // RunLoop()

//----------------------------------------------------------
//  STOP RUN
//----------------------------------------------------------
void Timepix3Producer::DoStopRun() {
  EUDAQ_USER("Timepix3Producer stopping run...");
  // Set a flag to signal to the polling loop that the run is over

  // Log mutex for SPIDR Controller - inside scope to release afterwards in case another thread depends on it.
  {
    std::lock_guard<std::mutex> lock_ctrl{controller_mutex_};
    // Close the shutter
    if( !spidrctrl->closeShutter() ) {
      EUDAQ_ERROR( "Could not close the shutter: " + spidrctrl->errorString());
    } else {
      EUDAQ_DEBUG( "closeShutter: OK");
    }
    // Stop Timepix3 readout
    if( !spidrctrl->pauseReadout() ) {
      EUDAQ_ERROR( "Could not stop data readout: " + spidrctrl->errorString());
    } else {
      EUDAQ_DEBUG( "pauseReadout: OK");
    }

    // Get some info
    int dataread;
    unsigned int timer_hi, timer_lo;
    if( !spidrctrl->getShutterCounter(&dataread) ) {
      EUDAQ_ERROR( "getShutterCounter: " + spidrctrl->errorString());
    } else {
      EUDAQ_DEBUG( "getShutterCounter: " + std::to_string(dataread));
    }

    if( !spidrctrl->getShutterStart( 0, &timer_lo, &timer_hi ) ) {
      EUDAQ_ERROR( "getShutterStart: " + spidrctrl->errorString());
    } else {
      EUDAQ_DEBUG( "getShutterStart: 0x" + to_hex_string(timer_hi,4) + to_hex_string(timer_lo,8));
    }

    if( !spidrctrl->getShutterEnd( 0, &timer_lo, &timer_hi ) ) {
      EUDAQ_ERROR( "getShutterEnd: " + spidrctrl->errorString());
    } else {
      EUDAQ_DEBUG( "getShutterEnd: 0x" + to_hex_string(timer_hi,4) + to_hex_string(timer_lo,8));
    }
  }

  sleep(1);
  m_running = false;

  // wait for RunLoop() to finish
  std::lock_guard<std::mutex> lock_daq{daq_mutex_};
  if(spidrdaq) {
    delete spidrdaq;
    spidrdaq = nullptr;
  } else {
    EUDAQ_WARN("DoStopRun: There was no spidrdaq instance. This is weird. Was there really a run to stop?");
  }
  EUDAQ_USER("Timepix3Producer stopped run.");
} // DoStopRun()
