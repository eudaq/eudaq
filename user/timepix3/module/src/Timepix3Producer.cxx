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

  Timepix3Config *myTimepix3Config;
  SpidrController *spidrctrl = nullptr;
  SpidrDaq *spidrdaq = nullptr;
  std::mutex device_mutex_;

  int m_supported_devices = 0;
  int m_active_devices;
  int m_spidrPort;
  string m_spidrIP, m_daqIP, m_xmlfileName, m_chipID;
  bool m_extRefClk, m_extT0;
  int m_xml_VTHRESH;
  float m_temp;

  /** Return the binary representation of a char as std::string
   */
  template <typename T> std::string to_bit_string(const T data, int length=-1) {
    std::ostringstream stream;
    // if length is not defined, use a standard (full) one
    if (length<0 || length > std::numeric_limits<T>::digits) {
      length = std::numeric_limits<T>::digits;
    }
    while(length >= 0) {
      stream << ((data >> length) & 1);
      length--;
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
      os << "0x" << to_hex_string(it.first) << ": ";
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
: eudaq::Producer(name, runcontrol), m_running(false) {
  myTimepix3Config = new Timepix3Config();
}

Timepix3Producer::~Timepix3Producer() {
  // force run stop
  m_running = false;
  // wait for all to finish
  std::lock_guard<std::mutex> lock{device_mutex_};
  if(spidrctrl) {
    delete spidrctrl;
    spidrctrl = nullptr;
  }
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
          if( !spidrctrl->getTimer(0, &timer_lo1, &timer_hi1) ) {
  		        EUDAQ_ERROR("getTimer: " + spidrctrl->errorString());
          //} else {
          //  std::cout << "Device no. " << device_nr << ", full timer: 0x" << to_hex_string(timer_hi1, 4) << to_hex_string(timer_lo1, 8) << std::endl;
          }
        }
        usleep(200000);
        usleep(200000);
        if ( !(cnt & 0xFF) ) {
          // update temperature every 16th cycle only
          getTpx3Temperature(0);
          //std::cout << "Tpx3Temp: " << Tpx3Temp << " °C" << std::endl;
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
  std::lock_guard<std::mutex> lock{device_mutex_};
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
  if (m_init || m_config || m_running) {
    EUDAQ_WARN("Timepix3Producer: Initializing while it is already in initialized state. Performing reset first...");
    DoReset();
  }
  std::lock_guard<std::mutex> lock{device_mutex_};

  auto config = GetInitConfiguration();

  EUDAQ_USER("Timepix3Producer initializing with: " + config->Name());


  // SPIDR IP & PORT
  m_spidrIP  = config->Get( "SPIDR_IP", "192.168.100.10" );
  int ip[4];
  if (!tokenize_ip_addr(m_spidrIP, ip) ) {
      EUDAQ_ERROR("Incorrect SPIDR IP address: " + m_spidrIP);
  }
  m_spidrPort = config->Get( "SPIDR_Port", 50000 );

  // Open a control connection to SPIDR-TPX3 module
  spidrctrl = new SpidrController( ip[0], ip[1], ip[2], ip[3], m_spidrPort );

  // reset SPIDR
  int errstat;
  if( !spidrctrl->reset( &errstat ) ) {
    EUDAQ_ERROR("reset ERROR: " + spidrctrl->errorString());
  } else if (errstat) {
    EUDAQ_ERROR("reset not complete, error code: 0x" + to_hex_string(errstat));
  } else {
    EUDAQ_EXTRA("reset: OK" );
  }

  // Are we connected to the SPIDR?
  if (!spidrctrl->isConnected()) {
    EUDAQ_ERROR("Connection to SPIDR failed at: " + spidrctrl->ipAddressString() + "; status: "
     + spidrctrl->connectionStateString() + ", " + spidrctrl->connectionErrString());
     //FIXME: return fail
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
  }
  EUDAQ_EXTRA("Number of devices supported by firmware: " + std::to_string(m_supported_devices));

  // set number of active devices
  m_active_devices = config->Get("active_devices", 1);
  EUDAQ_INFO("Number of active devices set in configuration: " + std::to_string(m_active_devices));
  if (m_supported_devices<m_active_devices) {
    EUDAQ_ERROR("You defined more active devices than what the system supports.");
  }
  if (m_active_devices<1) {
    EUDAQ_ERROR("You defined less than one active device.");
  }
  if (m_active_devices>1) {
    EUDAQ_ERROR("Only one active device is supported in this EUDAQ producer version.");
  }

  m_init = true;
  EUDAQ_USER("Timepix3Producer Init done");
} // DoInitialise()

//----------------------------------------------------------
//  CONFIGURE
//----------------------------------------------------------
void Timepix3Producer::DoConfigure() {
  if (!m_init) {
    EUDAQ_ERROR("DoConfigure: Trying to configure an uninitialized module. This will not work.");
    return;
  }
  if (m_running) {
    EUDAQ_WARN("DoConfigure: Trying to configure a running module. Trying to stop it first.");
    m_running = false;
  }
  std::lock_guard<std::mutex> lock{device_mutex_};

  if (!spidrctrl) {
    EUDAQ_ERROR("DoConfigure: spidrctrl was not initialized. Can not configure it.");
    return;
  }

  auto config = GetConfiguration();
  EUDAQ_USER("Timepix3Producer configuring: " + config->Name());
  // Configuration file values are accessible as config->Get(name, default)

  // set whether external clock (TLU) is used or device runs standalone
  m_extRefClk = config->Get("external_clock", false);
  if (!spidrctrl->setExtRefClk(m_extRefClk)) {
    EUDAQ_ERROR("setExtRefClk: " + spidrctrl->errorString());
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
  } else {
    EUDAQ_DEBUG("reinitDevices: OK" );
  }

  for (int device_nr=0; device_nr<m_active_devices; device_nr++){
    EUDAQ_INFO("Configuring device " + std::to_string(device_nr));

    //Due to timing issue, set readout speed at 320 Mbps
    if( !spidrctrl->setReadoutSpeed( device_nr, 320) ) {
      EUDAQ_ERROR("setReadoutSpeed: " + spidrctrl->errorString());
    } else {
      EUDAQ_DEBUG("setReadoutSpeed = 320");
    }

    // set output mask
    if( !spidrctrl->setOutputMask(device_nr, 0xFF) ) {
      EUDAQ_ERROR("setOutputMask: " + spidrctrl->errorString());
    } else {
      EUDAQ_DEBUG("setOutputMask = 0xFF");
    }

    int dataread;
    // check outblock register configuration
    if( !spidrctrl->getOutBlockConfig(device_nr, &dataread) ) {
      EUDAQ_ERROR("getOutBlockConfig: " + spidrctrl->errorString());
    } else {
      EUDAQ_DEBUG("getOutBlockConfig: 0x" + to_hex_string(dataread, 4));
    }

    // check pll register configuration
    if( !spidrctrl->getPllConfig(device_nr, &dataread) ) {
      EUDAQ_ERROR("getPllConfig: " + spidrctrl->errorString());
    } else {
      EUDAQ_DEBUG("getPllConfig: 0x" + to_hex_string(dataread, 4));
    }

    // check general configuration register
    if (!spidrctrl->getGenConfig( device_nr, &dataread )) {
      EUDAQ_ERROR("getGenConfig: " + spidrctrl->errorString());
    } else {
      EUDAQ_DEBUG("getGenConfig: 0x" + to_hex_string(dataread, 4));
    }

    // Load Timepix3 XML config file
    m_xmlfileName = config->Get( "XMLConfig_dev" + std::to_string(device_nr), "" );
    if (m_xmlfileName.empty()) {
      EUDAQ_DEBUG("Did not find configuration entry for \"XMLConfig_dev" + std::to_string(device_nr) + "\". Trying to use \"XMLConfig\".");
      m_xmlfileName = config->Get( "XMLConfig", "" );
    }
    myTimepix3Config->ReadXMLConfig( m_xmlfileName );
    cout << "Configuration file created on: " << myTimepix3Config->getTime() << endl;

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
    } else {
      int addr;
      if ( !spidrctrl->getIpAddrDest( device_nr, &addr ) ) {
        EUDAQ_ERROR( "getIpAddrDest: " + spidrctrl->errorString());
      } else {
        EUDAQ_DEBUG("getIpAddrDest: " + std::to_string((addr >> 24) & 0xFF) + "."  + std::to_string((addr >> 16) & 0xFF) +
        "."  + std::to_string((addr >> 8) & 0xFF) + "."  + std::to_string(addr  & 0xFF) );
      }
    }

    // check link status
    int linkstatus;
    if( !spidrctrl->getLinkStatus( device_nr, &linkstatus ) ) {
      EUDAQ_ERROR( "getLinkStatus: " + spidrctrl->errorString());
    } else {
      // Link status: bits 0-7: 0=link enabled; bits 16-23: 1=link locked
      int links_enabled = (~linkstatus) & 0xFF;
      int links_locked  = (linkstatus & 0xFF0000) >> 16;
      EUDAQ_DEBUG("Links enabled: 0b" + to_bit_string(links_enabled, 8));
      EUDAQ_DEBUG("Links locked : 0b" + to_bit_string(links_locked, 8));
    }

    // get Device ID
    int device_id = -1;
    if( !spidrctrl->getDeviceId( device_nr, &device_id ) ) {
      EUDAQ_ERROR("getDeviceId: " + spidrctrl->errorString());
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
    }
    if (!spidrctrl->getHeaderFilter(device_nr, &eth_mask, &cpu_mask)) {
      EUDAQ_ERROR("getHeaderFilter: "+ spidrctrl->errorString());
    }
    EUDAQ_EXTRA("ETH mask = 0b" + to_bit_string(eth_mask, 16));
    EUDAQ_EXTRA("CPU mask = 0b" + to_bit_string(cpu_mask, 16));


    // DACs configuration default (might be skipped, already done in reinitDevices())
    if( !spidrctrl->setDacsDflt( device_nr ) ) {
      EUDAQ_ERROR("setDacsDflt: " + spidrctrl->errorString());
    } else {
      EUDAQ_DEBUG("setDacsDflt: OK");
    }

    // Pixel configuration (might be skipped, already done in reinitDevices())
    // Does NOT reset pixel configuration bits. This still needs to be done
    //  manually by setting all bits explicitely to zero.
    if( !spidrctrl->resetPixels( device_nr ) ) {
      EUDAQ_ERROR("resetPixels: " + spidrctrl->errorString());
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
    } else {
      // read pixel configuration from device
      if( !spidrctrl->getPixelConfig( device_nr ) ) {
        EUDAQ_ERROR("getPixelConfig: " + spidrctrl->errorString());
      } else {
        int cnt_mask;
        int cnt_test;
        int cnt_thrs;
        if( checkPixelBits(&cnt_mask, &cnt_test, &cnt_thrs) ) {
          EUDAQ_ERROR("Pixel configuration should be all zeros, but is not. Pixels with mask set: "
                      + std::to_string(cnt_mask) + ", testbit set: " + std::to_string(cnt_test)
                      + " threshold set: " + std::to_string(cnt_thrs));
        } else {
          EUDAQ_DEBUG("Pixel configuration sucessfully set to zero and read back.");
        }
      }
    }

    spidrctrl->resetPixelConfig();
    spidrctrl->setPixelTestEna( 10, 10 );
    spidrctrl->setPixelTestEna( 11, 11 );
    spidrctrl->setPixelTestEna( 12, 12 );
    spidrctrl->setPixelMask( ALL_PIXELS, ALL_PIXELS );
    spidrctrl->setPixelMask( 9, 9 );
    spidrctrl->setPixelMask( 10, 10 );
    spidrctrl->setPixelMask( 11, 11 );
    spidrctrl->setPixelMask( 12, 12 );
    //spidrctrl->setPixelMask( 10, 10, 0 );
    EUDAQ_DEBUG("Setting testbit for all pixels");

    // Upload the pixel configuration to the device
    // Actually set the pixel thresholds and mask
    if( !spidrctrl->setPixelConfig( device_nr ) ) {
      EUDAQ_ERROR("setPixelConfig: " + spidrctrl->errorString());
    } else {
      EUDAQ_DEBUG("SetPixelConfig: OK");
    }

    // select second local pixel configuration set for reading the data from chip
    spidrctrl->selectPixelConfig(1);
    // read pixel configuration from device
    if( !spidrctrl->getPixelConfig( device_nr ) ) {
      EUDAQ_ERROR("getPixelConfig: " + spidrctrl->errorString());
    } else {
      EUDAQ_DEBUG("getPixelConfig: OK");
      if (0 != spidrctrl->comparePixelConfig(0,1)) {
        EUDAQ_ERROR("Pixel configuration that was read back from the chip does not correspond to what was written in it.");
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
    spidrctrl->selectPixelConfig(1);

    if( !spidrctrl->setGenConfig( device_nr,
                                  TPX3_POLARITY_HPLUS |
                                  TPX3_ACQMODE_TOA_TOT |
                                  TPX3_GRAYCOUNT_ENA |
                                  TPX3_TESTPULSE_ENA |
                                  TPX3_FASTLO_ENA |
                                  TPX3_SELECTTP_DIGITAL
                                   ) ) {
      EUDAQ_ERROR( "setGenConfig: " + spidrctrl->errorString());
    } else {
      int config = -1;
      // check general configuration register
      if (!spidrctrl->getGenConfig( device_nr, &config )) {
        EUDAQ_ERROR("getGenConfig: " + spidrctrl->errorString());
      } else {
        EUDAQ_DEBUG("getGenConfig: 0x" + to_hex_string(config, 4));
        // Unpack general config for human readable output
        myTimepix3Config->unpackGeneralConfig( config );
      }
    }
    ////////////////////////////////////////
    // ----------------------------------------------------------
    // Test pulse and CTPR configuration
    unsigned char *ctpr;

    // read CTPR from device
    if (!spidrctrl->getCtpr(device_nr, &ctpr)) {
      EUDAQ_ERROR( "getCtpr: " + spidrctrl->errorString());
    } else {
      std::cout << "device CTPR = 0x ";
      for (int i=0; i<(256/8); i++){
        std::cout << to_hex_string(ctpr[i]) << " ";
      }
      std::cout << std::endl;
    }

    // Enable test-pulses for some columns
    std::cout << "activating testpulse to columns: ";
    for(int col=0; col<256; ++col ) {
      //if( col >= 11 && col < 12 ) {
        std::cout << col << " ";
        spidrctrl->setCtprBit( col );
      //} else {
      //  spidrctrl->setCtprBit( col, 0);
      //}
    }
    std::cout << std::endl;
    // check content of local CTPR:
    ctpr = spidrctrl->ctpr();
    std::cout << "local  CTPR = 0x ";
    for (int i=0; i<(256/8); i++){
      std::cout << to_hex_string(ctpr[i]) << " ";
    }
    std::cout << std::endl;

    // Upload test-pulse register to the device
    if( !spidrctrl->setCtpr( device_nr ) ) {
      EUDAQ_ERROR( "setCtpr: " + spidrctrl->errorString());
    }

    // Timepix3 test pulse configuration
    if( !spidrctrl->setTpPeriodPhase( device_nr, 100, 0 ) ) {
      EUDAQ_ERROR( "setTpPeriodPhase: " + spidrctrl->errorString());
    }
    if( !spidrctrl->setTpNumber( device_nr, 1 ) ) {
      EUDAQ_ERROR( "setTpNumber: " + spidrctrl->errorString());
    }

/*  // not needed for digital testpulse injection
    // Coarse vtp:
    if(!spidrctrl->setDac( device_nr, TPX3_VTP_COARSE, 0x00 ) ) {
      EUDAQ_ERROR("Could not set VTP_COARSE: " + spidrctrl->errorString());
    } else {
      int tmpval = -1;
      spidrctrl->getDac( device_nr, TPX3_VTP_COARSE, &tmpval );
      EUDAQ_EXTRA("Successfully set DAC: VTP_COARSE to 0x" + to_hex_string(tmpval,4));
    }

    // Fine vtp:
    if(!spidrctrl->setDac( device_nr, TPX3_VTP_FINE, 0x001 ) ) {
      EUDAQ_ERROR("Could not set VTP_FINE: " + spidrctrl->errorString());
    } else {
      int tmpval = -1;
      spidrctrl->getDac( device_nr, TPX3_VTP_FINE, &tmpval );
      EUDAQ_EXTRA("Successfully set DAC: VTP_FINE to 0x" + to_hex_string(tmpval,4));
    }
*/

    // Check settings
    int tp_period, tp_phase, tp_num;
    if( !spidrctrl->getTpPeriodPhase( device_nr, &tp_period, &tp_phase ) ||
        !spidrctrl->getTpNumber( device_nr, &tp_num ) ) {
      EUDAQ_ERROR( "getTpPeriodPhase, getTpNumber: " + spidrctrl->errorString());
    } else {
      EUDAQ_INFO("tp_phase = " + std::to_string(tp_phase) + ", tp_period = " + std::to_string(tp_period) + ", tp_num = " + std::to_string(tp_num));
    }
    // read CTPR from device
    if (!spidrctrl->getCtpr(device_nr, &ctpr)) {
      EUDAQ_ERROR( "getCtpr: " + spidrctrl->errorString());
    } else {
      std::cout << "device CTPR = 0x ";
      for (int i=0; i<(256/8); i++){
        std::cout << to_hex_string(ctpr[i]) << " ";
      }
      std::cout << std::endl;
    }

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
    EUDAQ_ERROR("DoStartRun: Timepix3 producer is not configured properly. I can not start the run.");
    m_running = false;
    return;
  }
  if (m_running) {
    EUDAQ_WARN("DoStartRun: Timepix3 producer is already running. I'm trying to stop if first. This might however create a mess in the runs.");
    m_running = false;
  }
  EUDAQ_DEBUG("DoStartRun: Locking mutex...");

  // Create SpidrDaq
  std::lock_guard<std::mutex> lock{device_mutex_};
  if (m_running) {
    EUDAQ_ERROR("DoStartRun: Oh no, the producer is running again, although I just stopped it. Something is really wrong here. I'm giving it up.");
    return;
  }
  spidrdaq = new SpidrDaq( spidrctrl );


  EUDAQ_USER("Timepix3Producer starting run...");

  // Set Timepix3 into acquisition mode
  if( !spidrctrl->datadrivenReadout() ) {
    EUDAQ_ERROR( "datadrivenReadout: " + spidrctrl->errorString());
  } else {
    EUDAQ_DEBUG( "datadrivenReadout: OK");
  }

  // ----------------------------------------------------------
  // Configure the shutter trigger
  int trigger_mode = 4; // SPIDR_TRIG_AUTO;
  int trigger_length_us = 100000; // 100 ms
  int trigger_freq_hz = 1; //  Hz
  int trigger_count = 5; // triggers
  if( !spidrctrl->setShutterTriggerConfig( trigger_mode, trigger_length_us,
                                          trigger_freq_hz, trigger_count ) )
    EUDAQ_ERROR( "###setShutterTriggerConfig" );

  ////////////////////////////

  int device_nr = 0;
  // Start timer internally, if configured to do so
  if (!m_extT0) {
    // Restart timers to sync Timepix3 and TLU timestamps. Resets both SPIDR and Timepix3 timers.
    if( !spidrctrl->restartTimers() ) {
      EUDAQ_ERROR( "restartTimers:: " + spidrctrl->errorString());
    } else {
      EUDAQ_INFO( "restartTimers: Internal T0 generated.");
    }
  } else {
    EUDAQ_INFO( "External T0 signal is expected, NOT restarting timers.");
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
      EUDAQ_ERROR( "RunLoop: DoStartRun() was not executed properly before calling RunLoop(). Aborting run loop." );
      return;
    }
    EUDAQ_WARN( "RunLoop: Waiting to finish DoStartRun(). Attempt no. " + std::to_string(start_attempts));
    sleep(1);
  }

  std::lock_guard<std::mutex> lock{device_mutex_};
  EUDAQ_USER("Timepix3Producer starting run loop...");

  // create a thread to periodically ask for full timestamps, temperatures and others
  ts_thread = std::thread(&Timepix3Producer::timestamp_thread, this);

  //int device_nr = 0;
  std::map<int, int> header_counter;

  // set sampling parameters
  spidrdaq->setSampleAll( true );
  // Sample pixel data
  spidrdaq->setSampling( true );
  //spidrdaq->startRecording( "test.dat", 123, "This is test data" );

  // ----------------------------------------------------------
  // Get data samples and display some pixel data details
  // Start triggers
  if( !spidrctrl->startAutoTrigger() )
    EUDAQ_ERROR( "###startAutoTrigger" );
  // ----------------------------------------------------------
  while(m_running) {

    bool next_sample = true;
    if(spidrdaq->bufferFullOccurred()) {
      EUDAQ_WARN("Buffer overflow");
    }

    // Get a sample of pixel data packets, with timeout in ms
    const unsigned int BUF_SIZE = 8*1024*1024;
    next_sample = spidrdaq->getSample(BUF_SIZE, 3000);

    if(next_sample) {
      auto size = spidrdaq->sampleSize();
      std::cout << "Got a sample of a size " << size << std::endl;
      // look inside sample buffer...
      std::vector<eudaq::EventSPC> data_buffer;
      while(1) {
        uint64_t data = spidrdaq->nextPacket();

        // ...until the sample buffer is empty
        if(!data) break;

        uint64_t header = (data & 0xF000000000000000) >> 60;
        header_counter[header]++;

        std::cout << "Found header id: 0x" << std::hex << header << " data is: 0x" << data << std::dec << std::endl;

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
  m_running = false;
  std::lock_guard<std::mutex> lock{device_mutex_};
  if(spidrdaq) {
    delete spidrdaq;
    spidrdaq = nullptr;
  } else {
    EUDAQ_WARN("DoStopRun: There was no spidrdaq instance. This is weird. Was there really a run to stop?");
  }
  EUDAQ_USER("Timepix3Producer stopped run.");
} // DoStopRun()
