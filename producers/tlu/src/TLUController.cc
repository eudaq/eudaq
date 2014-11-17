#include "tlu/TLUController.hh"
#include "tlu/USBTracer.hh"
#include "tlu/TLUAddresses.hh"
#include "eudaq/Platform.hh"
#include "eudaq/Exception.hh"
#include "eudaq/Timer.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Logger.hh"

#ifdef WIN32
# include <cstdio>  // HK
#include "tlu/win_Usleep.h"
#define EUDAQ_uSLEEP(x) uSleep(x)

#else
#define EUDAQ_uSLEEP(x) usleep(x)
# include <unistd.h>
#endif

#include <iostream>
#include <ostream>
#include <fstream>
#include <iomanip>
#include <cstdlib>

using eudaq::mSleep;
using eudaq::hexdec;
using eudaq::to_string;
using eudaq::to_hex;
using eudaq::ucase;


#define MASKOUTTHELASTFOURBITS 0xFFFFFFFFFFFFFFF



#define PCA955_HW_ADDR 4
#define AD5316_HW_ADDR 3
#define PCA955_INPUT0_REGISTER 0
#define PCA955_INPUT1_REGISTER 1
#define PCA955_OUTPUT0_REGISTER 2
#define PCA955_OUTPUT1_REGISTER 3
#define PCA955_POLARITY0_REGISTER 4
#define PCA955_POLARITY1_REGISTER 5
#define PCA955_CONFIG0_REGISTER 6
#define PCA955_CONFIG1_REGISTER 7

namespace tlu {

  void TLU_LEDs::print(std::ostream & os) const {
    os << (left ? 'L' : '.')
       << (right ? 'R' : '.')
       << '-'
       << ((trig & 3) == 3 ? 'Y' : (trig & 2) ? 'R' : (trig & 1) ? 'G' : '.')
       << (busy ? 'B' : '.')
       << (rst ? 'R' : '.'); 
  }

  static const uint64_t NOTIMESTAMP = (uint64_t)-1;

  int do_usb_reset(ZESTSC1_HANDLE Handle); // defined in TLU_USB.cc

  std::string TLUException::make_msg(const std::string & msg, int status, int tries) {
    if (status == 0) {
      return msg;
    } else {
      char * errmsg;
      ZestSC1GetErrorMessage(static_cast<ZESTSC1_STATUS>(status), &errmsg);
      return "ZestSC1 ERROR in function " + msg + (tries ? " (" + eudaq::to_string(tries) + " tries)" : "")
        + " status = " + eudaq::to_string(status) + " (" + errmsg + ").";
    }
  }

  namespace {

    static const double TLUFREQUENCY = 48.001e6;
    static const int TLUFREQUENCYMULTIPLIER = 8;
    static const unsigned FIRSTV2SERIAL = 1000;

    static const char * g_versions[] = {
      "Unknown",
      "v0.1",
      "v0.2a",
      "v0.2c"
    };

//     static const uint32_t g_scaler_address[TLU_TRIGGER_INPUTS] = {
//       TRIGGER_IN0_COUNTER_0,
//       TRIGGER_IN1_COUNTER_0,
//       TRIGGER_IN2_COUNTER_0,
//       TRIGGER_IN3_COUNTER_0
//     };

    // Use one static flag for aborting, since the ErrorHandler can't easily
    // find which instance did the access
    // (an improvement would be to use a static std::map<ZESTSC1_HANDLE,TLUController*>)
    static bool errorhandleraborts(int newmode = -1) {
      static bool mode = false;
      if (newmode >= 0) mode = newmode;
      return mode;
    }

    static void DefaultErrorHandler(const char * function,
                                    ZESTSC1_HANDLE handle,
                                    ZESTSC1_STATUS status,
                                    const char *msg) {
      (void)handle;
      (void)status;
      if (errorhandleraborts()) {
        std::cerr << "ZESTSC1 ERROR:  function " << function << " returned error: " << msg << std::endl;
        usbflushtracefile();
        std::abort();
      }
    }

    static void I2Cdelay(unsigned us = 100) {
      eudaq::Timer t;
      do {
        // wait
      } while (t.uSeconds() < us);
      //usleep(us);
    }

    static unsigned lemo_dac_value(double voltage) {
      static const double vref = 2.049, vgain = 3.3;
      static const unsigned fullscale = 0x3ff;
      double vdac = vref - ((voltage - vref) / vgain);
      return unsigned(fullscale * (vdac / (2*vref))) & 0x3FFU;
    }
  }

  double Timestamp2Seconds(uint64_t t) {
    return t / ( TLUFREQUENCY * TLUFREQUENCYMULTIPLIER ) ;
  }

  void TLUEntry::Print(std::ostream & out) const {
    out << m_eventnum
        << ", " << eudaq::hexdec(m_timestamp, 0)
        << " = " << Timestamp2Seconds(m_timestamp);
  }

  std::string TLUEntry::trigger2String()
  {
	
		  std::string returnValue;
		  for (auto i=TLU_TRIGGER_INPUTS-1;i>=0;--i)
		  {

			  
			  returnValue+= to_string(m_trigger[i]);
		  }
		  return returnValue;
	 
  }

  // Modified to allow for a flag to choose between a 0.0V->1.0V (vref = 0.5) or 0.0V->2.0V (vref = 1.0) range
  // The default is vref = 0.5, but the TLU can be modified by cutting the LC1 trace and strapping the LO1 pads
  //   on the PMT supply board (which changes the SET pin on the ADR130 chip, thus doubling the reference voltage).
  unsigned TLUController::CalcPMTDACValue(double voltage)
  {
    double vref;
	
    static const double vgain = 2.0;  
    static const unsigned fullscale = 0x3ffU;   // 10-bits (AD5316 used on standard TLU)

    if(m_pmtvcntlmod == 0)
      {
	vref = 0.5;  // Standard TLU as shipped
      }
    else
      {
	vref = 1.0;  // After hardware modification on PMT power board (cut LC1, jumper LO1)
      }

    if(voltage < 0.0 || voltage > vref * 2.0)
      {
	std::cout << "Input voltage range [0, " << (unsigned)(vref * 2.0) << ".0 V] " << std::endl;
	return 0;
      }

    unsigned dac_orig = 0xb000 | ((unsigned)(fullscale * (voltage / (vref * vgain))) & fullscale) << 2; 

    // repack the data and return
    return((dac_orig >> 8) | ((dac_orig & 0x00ff) << 8));
  }

  TLUController::TLUController(int errorhandler) :
    m_mask(0),
    m_vmask(0),
    m_amask(0),
    m_omask(0),
    m_ipsel(0xff),
    m_handshakemode(0x3F), 
    m_triggerint(0),
    m_inhibit(true),
    m_vetostatus(0),
    m_fsmstatus(0),
    m_dutbusy(0),
    m_clockstat(0),
    m_dmastat(0),
    m_pmtvcntlmod(0),
    m_fsmstatusvalues(0),
    m_triggernum((unsigned)-1),
    m_timestamp(0),
    m_oldbuf(0),
	m_triggerBuffer(nullptr),
    m_particles(0),
    m_lasttime(0),
    m_errorhandler(errorhandler),
    m_version(0),
    m_addr(0),
    m_timestampzero(0),
    m_correctable_blockread_errors(0),
    m_uncorrectable_blockread_errors(0),
    m_usb_timeout_errors(0),
    m_debug_level(0),
	m_TriggerInformation(0)
  {
    errorhandleraborts(errorhandler == 0);
    for (int i = 0; i < TLU_TRIGGER_INPUTS; ++i) {
      m_scalers[i] = 0;
    }

    // Install an error handler
    ZestSC1RegisterErrorHandler(DefaultErrorHandler);

    OpenTLU();
  }

  void TLUController::OpenTLU() {
    
    // Request information about the system
    unsigned long NumCards = 0;
    unsigned long CardIDs[256] = {0};
    unsigned long SerialNumbers[256] = {0};
    ZESTSC1_FPGA_TYPE FPGATypes[256] = {ZESTSC1_FPGA_UNKNOWN};
	int status = ZestSC1CountCards( &NumCards, CardIDs, SerialNumbers, FPGATypes);
    if (status != 0) throw TLUException("ZestSC1CountCards", status);

    if ( m_debug_level & TLU_DEBUG_CONFIG ) {
      std::cout << "DEBUG: NumCards: " << NumCards << std::endl;
      for (unsigned i = 0; i < NumCards; ++i) {
        std::cout << "DEBUG: Card " << i
                  << ", ID = " << hexdec(CardIDs[i])
                  << ", SerialNum = 0x" << hexdec(SerialNumbers[i])
                  << ", FPGAType = " << hexdec(FPGATypes[i])
                  << ", Possible TLU: " << (FPGATypes[i] == ZESTSC1_XC3S1000 ?
                                            "Yes" : "No")
                  << std::endl;
      }
    }

    unsigned found = NumCards;
    for (unsigned i = 0; i < NumCards; ++i) {
      if (FPGATypes[i] == ZESTSC1_XC3S1000) {
        if (found == NumCards) {
          found = i;
        } else {
          throw TLUException("More than 1 possible TLU detected");
        }
      }
    }

    if (found == NumCards) {
      throw TLUException("No TLU detected");
    }

    m_serial = SerialNumbers[found];
    // Open the card
    status = ZestSC1OpenCard(CardIDs[found], &m_handle);
    if (status != 0) throw TLUException("ZestSC1OpenCard", status);
    ZestSC1SetTimeOut(m_handle, 200);
  }

  void TLUController::LoadFirmware() {
    if (m_filename == "") {
      m_filename =TLUFIRMWARE_PATH;
      m_filename+= "/TLU";
      if (m_version > 1) m_filename += "2";
      m_filename += "_Toplevel.bit";
    } else if (m_filename.find_first_not_of("0123456789") == std::string::npos) {
      std::string filename = TLUFIRMWARE_PATH;
      filename+= "/TLU";
      if (m_version == 2) filename += "2";
      filename += "_Toplevel-" + m_filename + ".bit";
      m_filename = filename;
    }
	std::cout<<"Loading bitfile: \""<<m_filename<<"\""<<std::endl;
    ZestSC1ConfigureFromFile(m_handle, const_cast<char*>(m_filename.c_str()));
    InhibitTriggers(true);
  }





  void TLUController::Initialize() {
#ifndef WIN32
    if (m_version == 2) {
      if (!SetupLemo()) {
        // If LEMO ADC does not respond, we must have a v0.2c TLU, so increment version number
        m_version++;
      }
    }
#else 
	 m_version=3;
#endif

    SetPMTVcntl(0U);

    // set up beam trigger
    WriteRegister(m_addr->TLU_BEAM_TRIGGER_VMASK_ADDRESS, m_vmask);
    WriteRegister(m_addr->TLU_BEAM_TRIGGER_AMASK_ADDRESS, m_amask);
    WriteRegister(m_addr->TLU_BEAM_TRIGGER_OMASK_ADDRESS, m_omask);

    SetDUTMask(m_mask, false);

    // Reset pointers
    WriteRegister(m_addr->TLU_RESET_REGISTER_ADDRESS, 
                  (1 << m_addr->TLU_TRIGGER_COUNTER_RESET_BIT)
                  | (1 << m_addr->TLU_BUFFER_POINTER_RESET_BIT) 
                  | (1 << m_addr->TLU_TRIGGER_FSM_RESET_BIT) 
                  ); 

    WriteRegister(m_addr->TLU_INTERNAL_TRIGGER_INTERVAL, m_triggerint);

    WritePCA955(m_addr->TLU_I2C_BUS_MOTHERBOARD,
                m_addr->TLU_I2C_BUS_MOTHERBOARD_TRIGGER__ENABLE_IPSEL_IO, 0xff00); // turn mb and lemo trigger outputs on
    WritePCA955(m_addr->TLU_I2C_BUS_MOTHERBOARD,
                m_addr->TLU_I2C_BUS_MOTHERBOARD_RESET_ENABLE_IO, 0xff00); // turn mb and lemo reset outputs on
  }

  TLUController::~TLUController() {
    delete[] m_oldbuf;
	delete[] m_triggerBuffer;
    ZestSC1CloseCard(m_handle);
  }

  void TLUController::Configure() {
    if (m_version == 0) {
      if (m_serial >= FIRSTV2SERIAL || m_serial == 260) { // DEST TLU v0.2 has serial 260
        m_version = 2;
      } else {
        m_version = 1;
      }
    }
    if (m_version == 1) {
      m_addr = &v0_1;
    } else {
      m_addr = &v0_2;
    }
    if (!m_oldbuf) {
		m_oldbuf = new uint64_t[m_addr->TLU_BUFFER_DEPTH];
		m_triggerBuffer=new unsigned[m_addr->TLU_BUFFER_DEPTH];
	}
    LoadFirmware();
    Initialize();
  }


    bool TLUController::SetPMTVcntl(unsigned value)
    {
        SelectBus(m_addr->TLU_I2C_BUS_DISPLAY);
	try {
	    WriteI2C16((AD5316_HW_ADDR << 2) | m_addr->TLU_I2C_BUS_PMT_DAC, 0x0f, CalcPMTDACValue((double)value/1000.0));  // Convert mV value to volts
	} catch (const eudaq::Exception &) {
	    return false;
	}
    
	return true;
    }

    // in mV, TLU_PMTS entries expected, sets each PMT control voltage separately
    // Note: PMT 1 (Chan 0 on faceplate) is connected to AD5316 DACD and PMT4 is connected to DACA (2->DACC, 3->DACB)
    bool TLUController::SetPMTVcntl(unsigned *values, double *gain_errors, double *offset_errors)
    {
	double target_voltages[TLU_PMTS];

	for(int i = 0; i < TLU_PMTS; i++)
	{
	    if(gain_errors != NULL)
	    {
		target_voltages[i] = (double)values[i] * (1.0 / gain_errors[i]);
	    }
	    else
	    {
		target_voltages[i] = (double)values[i];
	    }

	    // Convert to Volts at the same time
	    if(offset_errors != NULL)
	    {
		target_voltages[i] = (target_voltages[i] - offset_errors[i]) / 1000.0;
	    }
	    else
	    {
		target_voltages[i] = target_voltages[i] / 1000.0;
	    }
	}

        SelectBus(m_addr->TLU_I2C_BUS_DISPLAY);
	for(int i = 0; i < TLU_PMTS; i++)
	{
	    try {
	        WriteI2C16((AD5316_HW_ADDR << 2) | m_addr->TLU_I2C_BUS_PMT_DAC, 0x08 >> i, CalcPMTDACValue(target_voltages[i]));
	    } catch (const eudaq::Exception &) {
	        return false;
	    }
	}
    
	return true;
    }

    // Used by CalcPMTDACValue to determine voltage range produced by the TLU for the PMT Vcntl values
    void TLUController::SetPMTVcntlMod(unsigned value)
    {
        m_pmtvcntlmod = value;
    }

    // Preserved for backwards compatibility
    bool TLUController::SetupLVPower(int value) {
        return(SetPMTVcntl(value));
    }

  bool TLUController::SetupLemo() {
    SelectBus(m_addr->TLU_I2C_BUS_LEMO);
    try {
      // TTL (0x3) -> -0.5
      // NIM (0xC) ->  0.4
      WriteI2C16((AD5316_HW_ADDR << 2) | m_addr->TLU_I2C_BUS_LEMO_DAC,
                 0x3, 0xf000 | (lemo_dac_value(-0.5) << 2));
      WriteI2C16((AD5316_HW_ADDR << 2) | m_addr->TLU_I2C_BUS_LEMO_DAC,
                 0xC, 0xf000 | (lemo_dac_value(0.4) << 2));
    } catch (const eudaq::Exception &) {
      
      return false;
    }
    // Set high-z for TTL, 50-ohm for NIM
    WritePCA955(m_addr->TLU_I2C_BUS_LEMO, m_addr->TLU_I2C_BUS_LEMO_RELAY_IO, 0x03);
    return true;
  }

  void TLUController::Start() {
    // restart triggers
    //ResetTriggerCounter();
    //Initialize();

    InhibitTriggers(false);
  }

  void TLUController::Stop() {
    InhibitTriggers(true);
  }

  void TLUController::ResetTriggerCounter() {
    WriteRegister(m_addr->TLU_RESET_REGISTER_ADDRESS, 1 << m_addr->TLU_TRIGGER_COUNTER_RESET_BIT);
    WriteRegister(m_addr->TLU_RESET_REGISTER_ADDRESS, 0);
  }

  void TLUController::ResetScalers() {
    WriteRegister(m_addr->TLU_RESET_REGISTER_ADDRESS, 1 << m_addr->TLU_TRIGGER_SCALERS_RESET_BIT);
    WriteRegister(m_addr->TLU_RESET_REGISTER_ADDRESS, 0);
  }

  void TLUController::ResetTimestamp() {
    WriteRegister(m_addr->TLU_RESET_REGISTER_ADDRESS, 1 << m_addr->TLU_TIMESTAMP_RESET_BIT);
    m_timestampzero = eudaq::Time::Current();
    // WriteRegister(m_addr->TLU_RESET_REGISTER_ADDRESS, 0); // don't need to write zero afterwards.
  }

  void TLUController::ResetUSB() {
    do_usb_reset(m_handle);
    // this fails with error:
    // "The requested card ID does not correspond to any devices in the system"
    // Why?
    OpenTLU();
  }

  void TLUController::SetFirmware(const std::string & filename) {
    m_filename = filename;
  }

  void TLUController::SetVersion(unsigned version) {
    m_version = version;
  }

  void TLUController::SetDebugLevel(unsigned level) {
    m_debug_level = level;
  }

  void TLUController::SetDUTMask(unsigned char mask, bool updateleds) {
    m_mask = mask;
    if (m_addr) WriteRegister(m_addr->TLU_DUT_MASK_ADDRESS, m_mask);
    if (updateleds) UpdateLEDs();
  }

  void TLUController::SetVetoMask(unsigned char mask) {
    m_vmask = mask;
    if (m_addr) WriteRegister(m_addr->TLU_BEAM_TRIGGER_VMASK_ADDRESS, m_vmask);
  }

  void TLUController::SetAndMask(unsigned char mask) {
    m_amask = mask;
    if (m_addr) WriteRegister(m_addr->TLU_BEAM_TRIGGER_AMASK_ADDRESS, m_amask);
  }

  void TLUController::SetOrMask(unsigned char mask) {
    m_omask = mask;
    if (m_addr) WriteRegister(m_addr->TLU_BEAM_TRIGGER_OMASK_ADDRESS, m_omask);
  }

  void TLUController::SetEnableDUTVeto(unsigned char mask) {
    m_enabledutveto = mask;
    if (m_addr) WriteRegister(m_addr->TLU_ENABLE_DUT_VETO_ADDRESS, m_enabledutveto);
  }

  void TLUController::SetStrobe(uint32_t period , uint32_t width) {
    m_strobeperiod = period;
    m_strobewidth = width;

    if (m_addr) {
      if (m_strobeperiod != 0 && m_strobewidth != 0) { // if either period or width is zero don't enable strobe
        WriteRegister24(m_addr->TLU_STROBE_PERIOD_ADDRESS_0, m_strobeperiod);
        WriteRegister24(m_addr->TLU_STROBE_WIDTH_ADDRESS_0, m_strobewidth);
        WriteRegister(m_addr->TLU_STROBE_ENABLE_ADDRESS, 1); // enable strobe, but strobe won't start running until time-stamp is reset.
      } else {
        WriteRegister(m_addr->TLU_STROBE_ENABLE_ADDRESS, 0); // disable strobe.
      }
    }
  }

   void TLUController::SetHandShakeMode(unsigned handshakemode) {  //$$change
     m_handshakemode = handshakemode;
     if (m_addr) WriteRegister(m_addr->TLU_HANDSHAKE_MODE_ADDRESS, m_handshakemode);
   }

  void TLUController::SetTriggerInterval(unsigned millis) {
    m_triggerint = millis;
    if (m_addr) WriteRegister(m_addr->TLU_INTERNAL_TRIGGER_INTERVAL, m_triggerint);
  }
  void TLUController::SetTriggerInformation( unsigned TriggerInf )
  {m_TriggerInformation=TriggerInf;

  if (m_addr) WriteRegister(m_addr->TLU_WRITE_TRIGGER_BITS_MODE_ADDRESS, m_TriggerInformation);
  }

  unsigned char TLUController::GetAndMask() const {
    return ReadRegister8(m_addr->TLU_BEAM_TRIGGER_AMASK_ADDRESS);
  }

  unsigned char TLUController::GetOrMask() const {
    return ReadRegister8(m_addr->TLU_BEAM_TRIGGER_OMASK_ADDRESS);
  }

  unsigned char TLUController::GetVetoMask() const {
    return ReadRegister8(m_addr->TLU_BEAM_TRIGGER_VMASK_ADDRESS);
  }

  uint32_t TLUController::GetStrobeWidth() const {
    return ReadRegister24(m_addr->TLU_STROBE_WIDTH_ADDRESS_0);
  }

  uint32_t TLUController::GetStrobePeriod() const {
    return ReadRegister24(m_addr->TLU_STROBE_PERIOD_ADDRESS_0);
  }

  unsigned char TLUController::GetStrobeStatus() const {
    return ReadRegister8(m_addr->TLU_STROBE_ENABLE_ADDRESS);
  }

  unsigned char TLUController::GetDUTClockStatus() const {
    return ReadRegister8(m_addr->TLU_DUT_CLOCK_DEBUG_ADDRESS);
  }

  unsigned char TLUController::GetEnableDUTVeto() const {
    return ReadRegister8(m_addr->TLU_ENABLE_DUT_VETO_ADDRESS);
  }

  std::string TLUController::GetStatusString() const {
    std::string result;
    unsigned bit = 1;
    for (int i = 0; i < TLU_DUTS; ++i) {
      if (i) result += ",";
      if (m_mask & bit) {
        result += to_hex(!!(m_dutbusy & bit) + 2 * !!(m_clockstat & bit));
        result += to_hex(m_fsmstatusvalues >> 4*i & 0xf);
      } else {
        result += "--";
      }
      bit <<= 1;
    }
    result += " (" + to_string(m_vetostatus) + "," + to_string(m_dmastat) + ")";
    return result;
  }

  unsigned char TLUController::getTriggerInformation() const
  {
	      return ReadRegister8(m_addr->TLU_WRITE_TRIGGER_BITS_MODE_ADDRESS);
  }

  int TLUController::DUTnum(const std::string & name) {
    if (ucase(name) == "RJ45") return IN_RJ45;
    if (ucase(name) == "LEMO") return IN_LEMO;
    if (ucase(name) == "HDMI") return IN_HDMI;
    if (name.find_first_not_of("0123456789") == std::string::npos) {
      return eudaq::from_string(name, 0);
    }
    EUDAQ_THROW("Bad DUT input name");
  }

  void TLUController::SelectDUT(const std::string & name, unsigned mask, bool updateleds) {
    SelectDUT(DUTnum(name), mask, updateleds);
  }

  void TLUController::SelectDUT(int input, unsigned mask, bool updateleds) {
    for (int i = 0; i < 4; ++i) {
      if ((mask >> i) & 1) {
        m_ipsel &= ~(3 << (2*i));
        m_ipsel |= (input & 3) << (2*i);
      }
    }
    if (m_addr) WritePCA955(m_addr->TLU_I2C_BUS_MOTHERBOARD,
                            m_addr->TLU_I2C_BUS_MOTHERBOARD_FRONT_PANEL_IO, m_ipsel);
    if (updateleds) UpdateLEDs();
  }

  void seperate_timing_info_from_trigger_info(uint64_t * timestamp_buffer,unsigned* trigger,unsigned entries){
	  for (unsigned i = 0; i < entries; ++i) {
		 trigger[i]=timestamp_buffer[i]>>60;
		 timestamp_buffer[i]=timestamp_buffer[i]& MASKOUTTHELASTFOURBITS;

	  }

  }
  void TLUController::Update(bool timestamps) {
    unsigned entries = 0;
    unsigned old_triggernum = m_triggernum;
    uint64_t * timestamp_buffer = 0;
	unsigned* trigger_buffer=nullptr;
    m_dmastat = ReadRegister8(m_addr->TLU_DMA_STATUS_ADDRESS);
    if (timestamps) {
      bool oldinhibit = InhibitTriggers();

      WriteRegister(m_addr->TLU_STATE_CAPTURE_ADDRESS, 0xFF);

      entries = ReadRegister16(m_addr->TLU_REGISTERED_BUFFER_POINTER_ADDRESS_0);

      if ( m_debug_level & TLU_DEBUG_UPDATE ) {
        std::cout << "TLU::Update: after 1 read, entries " << entries << std::endl;
      }
	  
      timestamp_buffer = ReadBlock(entries);
	  trigger_buffer=m_triggerBuffer;   // this is not multi thread save. but since both are not multi thread save it seems to be ok

      // Reset buffer pointer
      WriteRegister(m_addr->TLU_RESET_REGISTER_ADDRESS, 1 << m_addr->TLU_BUFFER_POINTER_RESET_BIT);

      InhibitTriggers(oldinhibit);
    } else {
      WriteRegister(m_addr->TLU_STATE_CAPTURE_ADDRESS, 0xFF);
      entries = ReadRegister16(m_addr->TLU_REGISTERED_BUFFER_POINTER_ADDRESS_0);
      WriteRegister(m_addr->TLU_RESET_REGISTER_ADDRESS, 1 << m_addr->TLU_BUFFER_POINTER_RESET_BIT);
    }

    if ( m_debug_level & TLU_DEBUG_UPDATE ) {
      std::cout << "TLU::Update: entries=" << entries << std::endl;
    }

    m_fsmstatus = ReadRegister8(m_addr->TLU_TRIGGER_FSM_STATUS_ADDRESS);
    m_fsmstatusvalues = ReadRegister24(m_addr->TLU_TRIGGER_FSM_STATUS_VALUE_ADDRESS_0);
    m_vetostatus = ReadRegister8(m_addr->TLU_TRIG_INHIBIT_ADDRESS);
    m_dutbusy = ReadRegister8(m_addr->TLU_DUT_BUSY_ADDRESS);
    m_clockstat = ReadRegister8(m_addr->TLU_DUT_CLOCK_DEBUG_ADDRESS);

    if ( m_debug_level & TLU_DEBUG_UPDATE ) {
      std::cout << "TLU::Update: fsm 0x" << std::hex << m_fsmstatus << " status values 0x" << m_fsmstatusvalues << " veto 0x" << (int) m_vetostatus << " DUT Clock status 0x" << (int) ReadRegister8(m_addr->TLU_DUT_CLOCK_DEBUG_ADDRESS) << std::dec << std::endl;
    }

    m_triggernum = ReadRegister32(m_addr->TLU_REGISTERED_TRIGGER_COUNTER_ADDRESS_0);
    m_timestamp = ReadRegister64(m_addr->TLU_REGISTERED_TIMESTAMP_ADDRESS_0);
    if ( m_debug_level & TLU_DEBUG_UPDATE ) {
      std::cout << "TLU::Update: trigger " << m_triggernum << " timestamp 0x" << std::hex << m_timestamp << std::dec << std::endl;
      std::cout << "TLU::Update: scalers";
    }


    for (int i = 0; i < TLU_TRIGGER_INPUTS; ++i) {
      m_scalers[i] = ReadRegister16(m_addr->TLU_SCALERS(i));

      if ( m_debug_level & TLU_DEBUG_UPDATE ) {
        std::cout << ", [" << i << "] " << m_scalers[i];
      }
    }

    if ( m_debug_level & TLU_DEBUG_UPDATE ) {
      std::cout << std::endl;
    }

    m_particles = ReadRegister32(m_addr->TLU_REGISTERED_PARTICLE_COUNTER_ADDRESS_0);

    // Read timestamp buffer of BUFFER_DEPTH entries from TLU
    m_buffer.clear();
    unsigned trig = m_triggernum - entries;
    if (entries > 0) {
      if (trig != old_triggernum) {

        EUDAQ_ERROR("Unexpected trigger number: " + to_string(trig) +
                    " (expecting " + to_string(old_triggernum) + ")");
      }
      for (unsigned i = 0; i < entries; ++i) {
        
        
		  m_buffer.push_back(
			  TLUEntry(
			  timestamp_buffer ? timestamp_buffer[i] : NOTIMESTAMP,
			  trig++, 
			  trigger_buffer ? trigger_buffer[i]:0)
			  );
      }
    }
    //mSleep(1);
  }


  unsigned char TLUController::GetTriggerStatus() const {
    return ReadRegister8(m_addr->TLU_TRIG_INHIBIT_ADDRESS);
  }


  bool TLUController::InhibitTriggers(bool inhibit) {
    WriteRegister(m_addr->TLU_TRIG_INHIBIT_ADDRESS, inhibit);
    bool result = m_inhibit;
    m_inhibit = inhibit;
    return result;
  }

  void TLUController::WriteRegister(uint32_t offset, unsigned char val) {
    int status = ZESTSC1_SUCCESS;
    int delay = 0;
    const int count = m_errorhandler ? m_errorhandler : 1;
    for (int i = 0; i < count; ++i) {
      if (delay == 0) {
        delay = 20;
      } else {
        EUDAQ_uSLEEP(delay);
        delay += delay;
      }
      status = ZestSC1WriteRegister(m_handle, offset, val);
      usbtrace(" W", offset, val, status);
      if (status == ZESTSC1_SUCCESS) break;
    }
    if (status != ZESTSC1_SUCCESS) {
      usbflushtracefile();
      throw TLUException("WriteRegister", status, count);
    }
  }

  void TLUController::WriteRegister24(uint32_t offset, uint32_t val) {
    int status = ZESTSC1_SUCCESS;
    int delay = 0;

    const int count = m_errorhandler ? m_errorhandler : 1;

    for (int byte = 0; byte < 3; ++byte ) {

      for (int i = 0; i < count; ++i) {
        if (delay == 0) {
          delay = 20;
        } else {
           EUDAQ_uSLEEP(delay);
          delay += delay;
        }
        status = ZestSC1WriteRegister(m_handle, offset+byte, ((val >> (8*byte)) & 0xFF)  );
        usbtrace(" W", offset, val, status);
        if (status == ZESTSC1_SUCCESS) break;
      }
      if (status != ZESTSC1_SUCCESS) {
        usbflushtracefile();
        throw TLUException("WriteRegister24", status, count);
      }

    }
  }

  unsigned char TLUController::ReadRegister8(uint32_t offset) const {
    unsigned char val = ReadRegisterRaw(offset);
    //usbtrace(" R", offset, val);
    return val;
  }

  unsigned short TLUController::ReadRegister16(uint32_t offset) const {
    unsigned short val = ReadRegisterRaw(offset);
    val |= static_cast<unsigned short>(ReadRegisterRaw(offset+1)) << 8;
    //usbtrace(" R", offset, val);
    return val;
  }

  uint32_t TLUController::ReadRegister24(uint32_t offset) const {
    uint32_t val = 0;
    for (int i = 0; i < 3; ++i) {
      val |= static_cast<uint32_t>(ReadRegisterRaw(offset+i)) << (8*i);
    }
    //usbtrace(" R", offset, val);
    return val;
  }

  uint32_t TLUController::ReadRegister32(uint32_t offset) const {
    uint32_t val = 0;
    for (int i = 0; i < 4; ++i) {
      val |= static_cast<uint32_t>(ReadRegisterRaw(offset+i)) << (8*i);
    }
    //usbtrace(" R", offset, val);
    return val;
  }

  uint64_t TLUController::ReadRegister64(uint32_t offset) const {
    uint64_t val = 0;
    for (int i = 0; i < 8; ++i) {
      val |= static_cast<uint64_t>(ReadRegisterRaw(offset+i)) << (8*i);
    }
    //usbtrace(" R", offset, val);
    return val;
  }

  unsigned char TLUController::ReadRegisterRaw(uint32_t offset) const {
    unsigned char val = 0;
    int status = ZESTSC1_SUCCESS;
    int delay = 0;
    const int count = m_errorhandler ? m_errorhandler : 1;
    for (int i = 0; i < count; ++i) {
      if (delay == 0) {
        delay = 20;
      } else {
         EUDAQ_uSLEEP(delay);
        delay += delay;
      }
      status = ZestSC1ReadRegister(m_handle, offset, &val);
      usbtrace(" R", offset, val, status);
      if (status == ZESTSC1_SUCCESS) break;
    }
    if (status != ZESTSC1_SUCCESS) {
      usbflushtracefile();
      throw TLUException("ReadRegister", status, count);
    }
    return val;
  }

  uint64_t * TLUController::ReadBlock(unsigned entries) {
    if (!entries) return 0;

    const int count = m_errorhandler ? m_errorhandler : 1;
    static const unsigned buffer_offset = 2;
    unsigned num_errors = 0;

    for (int i = 0; i < count; ++i) { // loop round trying to read buffer

      // read the timestamp buffer
      unsigned num_errors ;
      num_errors = ReadBlockRaw( entries , buffer_offset );

      if ( num_errors == 0 ) break;

      // try to correct error

      if ( m_debug_level & TLU_DEBUG_BLOCKREAD ) {
        std::cout << "### Warning: detected error in block read. Trying a soft correction by reading blocks again with NO padding" << std::endl;
        EUDAQ_WARN("### Warning: detected error in block read. Trying a soft correction by reading blocks again with NO padding");
      }
      num_errors = ReadBlockSoftErrorCorrect( entries , false );

      if ( num_errors == 0 ) break;

      if ( m_debug_level & TLU_DEBUG_BLOCKREAD ) {
        std::cout << "### Warning: detected error in block read. Trying a soft correction by reading blocks again with padding" << std::endl;
        EUDAQ_WARN("### Warning: detected error in block read. Trying a soft correction by reading blocks again with padding");
      }
      num_errors = ReadBlockSoftErrorCorrect( entries , true );

      if ( num_errors == 0 ) break;

      if ( m_debug_level & TLU_DEBUG_BLOCKREAD ) {
        std::cout << "### Warning: detected error in block read. Trying a soft correction by reading blocks again with NO padding (2nd time)" << std::endl;
        EUDAQ_WARN("### Warning: detected error in block read. Trying a soft correction by reading blocks again with NO padding (2nd time)");
      }
      num_errors = ReadBlockSoftErrorCorrect( entries , false );

      if ( num_errors == 0 ) break;

      if ( m_debug_level & TLU_DEBUG_BLOCKREAD ) {
        std::cout << "### Warning: detected error in block read. Trying a soft correction by reading blocks again with padding (2nd time)" << std::endl;
        EUDAQ_WARN("### Warning: detected error in block read. Trying a soft correction by reading blocks again with padding (2nd time)");
      }
      num_errors = ReadBlockSoftErrorCorrect( entries , true );

      if ( num_errors == 0 ) break;

      // then try to reset DMA
      if ( m_debug_level & TLU_DEBUG_BLOCKREAD ) {
        std::cout << "### Warning: Re-read of block data failed to correct problem. Will try to reset DMA buffer pointer" << std::endl;
        EUDAQ_WARN("### Warning: Re-read of block data failed to correct problem. Will try to reset DMA buffer pointer");
      }
      num_errors = ResetBlockRead( entries ) ;

      if ( num_errors == 0 ) break;

    }

    if ( num_errors != 0) {
      throw TLUException("ReadBlock" , 0 , count);
    }

    return m_oldbuf;

  }

  unsigned  TLUController::ReadBlockRaw(unsigned entries , unsigned buffer_offset) {

    unsigned num_errors = 0;
    unsigned num_correctable_errors = 0;
    unsigned num_uncorrectable_errors = 0;

    const uint64_t timestamp_mask  = 0x0FFFFFFFFFFFFFFFULL;

    //    uint64_t buffer[4][4096]; // should be m_addr->TLU_BUFFER_DEPTH
    if (m_addr->TLU_BUFFER_DEPTH > 4096) EUDAQ_THROW("Buffer size error");

    int result = ZESTSC1_SUCCESS;

    WriteRegister(m_addr->TLU_INITIATE_READOUT_ADDRESS, ( 1<< m_addr->TLU_ENABLE_DMA_BIT) ); // the first write sets transfer going. Further writes do nothing.

     EUDAQ_uSLEEP(10);

    // Read four buffers at once. first buffer will contain some data from previous readout,
    // but futher reads should return indentical data, but data corruption will mean than the buffer contents aren't identical.
    // Try to correct this using multiple reads.
    // result = ZestSC1ReadData(m_handle, m_working_buffer[0], sizeof m_working_buffer );
    // Change syntax. Should be exactly the same but getting mysterious SEGFAULTs so hack at random...
    result = ZestSC1ReadData(m_handle, &m_working_buffer, sizeof m_working_buffer );

    if ( m_debug_level & TLU_DEBUG_BLOCKREAD ) {
      char * errmsg = 0;
      ZestSC1GetErrorMessage(static_cast<ZESTSC1_STATUS>(result), &errmsg);
      std::cout << (result == ZESTSC1_SUCCESS ? "" : "#### Warning: ") << errmsg << std::endl;
    }


    // check to make sure that the first entry is zero ( which it should be unless something has slipped )
    for (int tries = 0; tries < 4; ++tries) { 
      if (m_working_buffer[tries][0] !=0) {
        std::cout << "### Warning: m_working_buffer[buf][0] != 0. This shouldn't happen. buf = " << tries << std::endl;
        EUDAQ_WARN("### Warning: m_working_buffer[buf][0] != 0. This shouldn't happen. buf = " + eudaq::to_string(tries) );
        num_uncorrectable_errors++;
      }
    }

    for (unsigned i = buffer_offset; i < entries+buffer_offset; ++i) {

      // std::cout << std::setw(8) <<  m_working_buffer[0][i] << "  " <<  m_working_buffer[1][i] << "  " <<   m_working_buffer[2][i] << "  " <<   m_working_buffer[3][i] << std::endl;

      // check that at least 2 out of three timestamps agree with each other....
      // due to latency in buffer transfer the real data starts at the third 64-bit word.
      if (( m_working_buffer[1][i] == m_working_buffer[2][i] ) ||
          ( m_working_buffer[1][i] == m_working_buffer[3][i] ) ) {
        m_oldbuf[i-buffer_offset] = m_working_buffer[1][i];

      } else if ( m_working_buffer[2][i] == m_working_buffer[3][i] ) {
        m_oldbuf[i-buffer_offset] = m_working_buffer[2][i];
      } else {
        m_oldbuf[i-buffer_offset] = 0;
        std::cout << "### Warning: Uncorrectable data error in timestamp buffer. location = " << i << "data ( buffer =2,3,4) : " << std::setw(8) << m_working_buffer[1][i] << "  " << m_working_buffer[2][i] << "  " << m_working_buffer[3][i] << std::endl;
        EUDAQ_WARN("### Warning: Uncorrectable data error in timestamp buffer. location = " + eudaq::to_string(i) + "data ( buffer =2,3,4) : "  + eudaq::to_string(m_working_buffer[1][i]) +  "  " +   eudaq::to_string(m_working_buffer[2][i]) + "  " +  eudaq::to_string( m_working_buffer[3][i]) );
        num_correctable_errors++;
      }

      if (( m_working_buffer[1][i] != m_working_buffer[2][i] ) ||
          ( m_working_buffer[1][i] != m_working_buffer[3][i] ) ||
          ( m_working_buffer[2][i] != m_working_buffer[3][i] )
          ){
          num_correctable_errors++; }

    }

    // Check to make sure that the current timestamp[0] is more recent than previous timestamp[events]
    uint64_t last_timestamp = m_lasttime & timestamp_mask;
    uint64_t first_timestamp = m_oldbuf[0] & timestamp_mask;
    if (  last_timestamp >= first_timestamp ) {
      std::cout << "### Warning: First time-stamp from current buffer is older than last timestamp of previous buffer: (m_lasttime , buf[0]) " << std::setw(8) <<  m_lasttime  << "  " << m_oldbuf[0] << std::endl;
      num_uncorrectable_errors++;
    }

	if (m_TriggerInformation==USE_TRIGGER_INPUT_INFORMATION)
	{

		seperate_timing_info_from_trigger_info(m_oldbuf,m_triggerBuffer,entries);
	}


    // check that the timestamps are chronological ...
    for (unsigned i = 1; i < entries; ++i) {

      uint64_t current_timestamp = m_oldbuf[i]; // & timestamp_mask;
      uint64_t previous_timestamp = m_oldbuf[i-1]; // & timestamp_mask;

      if ( previous_timestamp >= current_timestamp ) {

        // throw TLUException("Timestamp data check error: first timestamp of  this block is more recent that last timestamp of previous block", 1, count);

        std::cout << "Timestamp data check error: timestamps not in chronological order: " << std::setw(8) <<  m_oldbuf[i-1]  << " >  " << m_oldbuf[i] << std::endl;
        num_uncorrectable_errors++;

      } 


    }

    // need to add checking that timestamp is in the correct ball-park ( use Timestamp2Seconds ... )

    // usbtrace("BR", 0, m_working_buffer[3], m_addr->TLU_BUFFER_DEPTH, result);

    m_uncorrectable_blockread_errors += num_uncorrectable_errors; 
    m_correctable_blockread_errors += num_correctable_errors;

    // return the number of uncorrectable errors....
    num_errors = num_uncorrectable_errors ;

    if ( m_debug_level & TLU_DEBUG_BLOCKREAD ) {
      std::cout << "Debug - dumping block" << std::endl;
      PrintBlock( m_working_buffer , 4 , 4096 );
    
      std::cout << "Debug - about to return num_errors ( num_correctable ) = " << num_errors << "  ( " << num_correctable_errors << " )" << std::endl;
    }

    return num_errors;
  }


  // try to recover from a problem by reading the TLU buffer repeatedly.
  unsigned TLUController::ReadBlockSoftErrorCorrect( unsigned entries , bool pad ) {

    // first read the 4 block again three times...

    static const unsigned buffer_offset = 2;
    
    unsigned num_errors ;

    // uint64_t buffer[12][4096]; // should be m_addr->TLU_BUFFER_DEPTH
    uint64_t padding_buffer[2048];

    std::cout << "### Error recovery: About to read out blocks three times..." << std::endl;
    EUDAQ_INFO("Error recovery: About to read out blocks three times...");

    int result = ZESTSC1_SUCCESS;

    result = ZestSC1ReadData(m_handle, m_working_buffer[0], sizeof m_working_buffer );


    if ( m_debug_level & TLU_DEBUG_BLOCKREAD ) {
      char * errmsg = 0;
      ZestSC1GetErrorMessage(static_cast<ZESTSC1_STATUS>(result), &errmsg);
      std::cout << (result == ZESTSC1_SUCCESS ? "" : "#### Warning (1st read): ") << errmsg << std::endl;
    }


    result = ZestSC1ReadData(m_handle, m_working_buffer[0], sizeof m_working_buffer );
    result = ZestSC1ReadData(m_handle, m_working_buffer[0], sizeof m_working_buffer );
    result = ZestSC1ReadData(m_handle, m_working_buffer[0], sizeof m_working_buffer );


    if ( m_debug_level & TLU_DEBUG_BLOCKREAD ) {
      char * errmsg = 0;
      ZestSC1GetErrorMessage(static_cast<ZESTSC1_STATUS>(result), &errmsg);
      std::cout << (result == ZESTSC1_SUCCESS ? "" : "#### Warning (2nd read): ") << errmsg << std::endl;
    }


    if ( pad ) {
      std::cout <<"### Reading 2048 uint64_t words to pad ...."  << std::endl;   
      result = ZestSC1ReadData(m_handle, padding_buffer, sizeof padding_buffer );
    } else {
      std::cout <<"### No padding block read ...."  << std::endl;   
    }


    if ( m_debug_level & TLU_DEBUG_BLOCKREAD ) {
      char * errmsg = 0;
      ZestSC1GetErrorMessage(static_cast<ZESTSC1_STATUS>(result), &errmsg);
      std::cout << (result == ZESTSC1_SUCCESS ? "" : "#### Warning (2nd read): ") << errmsg << std::endl;

      std::cout << "#### Read buffers to resync. About to read data ..." << std::endl;
      EUDAQ_INFO("Error recovery:  Read buffers to resync. About to read data ...");
    }


    num_errors = ReadBlockRaw( entries , buffer_offset );

    if ( m_debug_level & TLU_DEBUG_BLOCKREAD ) {
      std::cout << "#### Number of errors reported by ReadBlockRaw after rsync = " << num_errors << std::endl;
      EUDAQ_INFO("Error recovery: Number of errors reported by ReadBlockRaw after rsync = " + eudaq::to_string( num_errors) );
    }

    return num_errors;

  }


  unsigned TLUController::ResetBlockRead( unsigned entries ) {

    std::cout << "### Warning: Attempting to reset block transfer" << std::endl;

    // Assert reset line on DMA controller buffer address...
    unsigned original_dma_status = ReadRegister8(m_addr->TLU_INITIATE_READOUT_ADDRESS);
    std::cout << "Read back INITIATE_READOUT_ADDRESS: " << original_dma_status << std::endl;

    WriteRegister(m_addr->TLU_INITIATE_READOUT_ADDRESS ,
                  (original_dma_status | (1<< m_addr->TLU_RESET_DMA_COUNTER_BIT)) );
    unsigned new_dma_status = ReadRegister8(m_addr->TLU_INITIATE_READOUT_ADDRESS);
    std::cout << "### Read back (after raising reset line) INITIATE_READOUT_ADDRESS: " << new_dma_status << std::endl;


    // read out some data to reset transfer machine.

    static const unsigned buffer_offset = 2;
    unsigned num_errors;

    std::cout << "### About to read data with pointer held reset" << std::endl;
    num_errors = ReadBlockRaw( entries , buffer_offset );
    std::cout << "### Read data with pointer held reset" << std::endl;

    WriteRegister(m_addr->TLU_INITIATE_READOUT_ADDRESS ,
                  original_dma_status );
    new_dma_status = ReadRegister8(m_addr->TLU_INITIATE_READOUT_ADDRESS);
    std::cout << "### Read back (after dropping reset line) INITIATE_READOUT_ADDRESS: " << new_dma_status << std::endl;

    // and readout data....
    std::cout << "### Reading block after resetting pointer." << std::endl;
    num_errors = ReadBlockRaw( entries , buffer_offset );

    std::cout << "### Reading block after resetting pointer. (2nd time)" << std::endl;
    num_errors = ReadBlockRaw( entries , buffer_offset );
    
    return num_errors;

  }

  void TLUController::PrintBlock( uint64_t  block[][4096] , unsigned nbuf , unsigned bufsize ) {

    // print contents of 4-buffer block
    unsigned buf , sample;
    for ( sample = 0 ; sample < bufsize ; sample++ ) {

      std::cout << " " << sample << " " << std::setw(8) ;

      for (buf = 0 ; buf < nbuf ; buf++ ) {

        std::cout << " " << std::hex << block[buf][sample] << std::dec ;
      }

      std::cout << std::endl;
    }

  }


  void TLUController::Print(std::ostream &out, bool timestamps) const {
    if (timestamps) {
      for (size_t i = 0; i < m_buffer.size(); ++i) {
        uint64_t d = m_buffer[i].Timestamp() - m_lasttime;
        out << " " << std::setw(8) << m_buffer[i] << ", diff=" << d << (d <= 0 ? "***" : "") << "\n";
        m_lasttime = m_buffer[i].Timestamp();
      }
    }
    out << "Status:    " << GetStatusString() << "\n"
        << "Scalers:   ";
    for (int i = 0; i < TLU_TRIGGER_INPUTS; ++i) {
      std::cout << m_scalers[i] << (i < (TLU_TRIGGER_INPUTS - 1) ? ", " : "\n");
    }
    out << "Particles: " << m_particles << "\n"
        << "Triggers:  " << m_triggernum << "\n"
        << "Entries:   " << NumEntries() << "\n"
        << "TS errors: " << m_correctable_blockread_errors << ", " << m_uncorrectable_blockread_errors << " (redundancy, re-read)\n"
        << "Timestamp: " << eudaq::hexdec(m_timestamp, 0)
        << " = " << Timestamp2Seconds(m_timestamp) << std::endl;
  }

  std::string TLUController::GetVersion() const {
    size_t maxversion = sizeof g_versions / sizeof *g_versions;
    return g_versions[m_version < maxversion ? m_version : 0];
  }

  std::string TLUController::GetFirmware() const {
    return m_filename;
  }

  unsigned TLUController::GetFirmwareID() const {
    return ReadRegister8(m_addr->TLU_FIRMWARE_ID_ADDRESS);
  }

  unsigned TLUController::GetSerialNumber() const {
    return m_serial;
  }

  unsigned TLUController::GetLibraryID(unsigned ver) const {
    if (ver == 1) {
      return v0_1.TLU_FIRMWARE_ID;
    } else if (ver == 2) {
      return v0_2.TLU_FIRMWARE_ID;
    } else if (ver == 0 && m_addr) {
      return m_addr->TLU_FIRMWARE_ID;
    }
    EUDAQ_THROW("TLU is not configured");
  }

  unsigned TLUController::GetParticles() const {
    return m_particles;
  }

  unsigned TLUController::GetScaler(unsigned i) const {
    if (i >= (unsigned)TLU_TRIGGER_INPUTS) EUDAQ_THROW("Scaler number out of range");
    return m_scalers[i];
  }

  void TLUController::UpdateLEDs() {
    std::vector<TLU_LEDs> leds(TLU_DUTS);
    //std::cout << "mask: " << hexdec(m_mask) << ", ipsel: " << hexdec(m_ipsel) << std::endl;
    for (int i = 0; i < TLU_DUTS; ++i) {
      int bit = 1 << i;
      if (m_mask & bit) leds[i].left = 1;
      if (i >= TLU_LEMO_DUTS) {
        leds[i].right = leds[i].left;
        continue;
      }
      int ipsel = (m_ipsel >> (2*i)) & 3;
      switch (ipsel) {
      case IN_RJ45:
        leds[i].right = leds[i].left;
        leds[i].trig = 1;
        break;
      case IN_LEMO:
        leds[i].busy = leds[i].left;
        leds[i].trig = 1;
        break;
      case IN_HDMI:
        leds[i].trig = 2;
        break;
      }
    }
    //std::cout << "LEDS: " << to_string(leds) << std::endl;
    SetLEDs(leds);
  }

  void TLUController::SetLEDs(int left, int right) {
    std::vector<TLU_LEDs> leds(TLU_DUTS);
    for (int i = 0; i < TLU_DUTS; ++i) {
      int bit = 1 << i;
      if (left & bit) leds[i].left = 1;
      if (right & bit) leds[i].right = 1;
    }
    SetLEDs(leds);
  }

  void TLUController::SetLEDs(const std::vector<TLU_LEDs> & leds) {
    if (!m_addr) EUDAQ_THROW("Cannot set LEDs on unconfigured TLU");
    if (m_version == 1) {
      int ledval = 0;
      for (int i = 0; i < TLU_DUTS; ++i) {
        if (leds[i].left) ledval |= 1 << i;
      }
      WriteRegister(m_addr->TLU_DUT_LED_ADDRESS, ledval);
    } else {
      int dutled = 0, lemoled = 0;
      //std::cout << "LED:";
      for (int i = 0; i < TLU_DUTS; ++i) {
        //std::cout << ' ' << leds[i];
        if (leds[i].left) dutled |= 1 << (2*i+1);
        if (leds[i].right) dutled |= 1 << (2*i);
        if (leds[i].rst) lemoled |= 1 << (2*i);
        if (leds[i].busy) lemoled |= 1 << (2*i+1);
        int swap = ((leds[i].trig >> 1) & 1) | ((leds[i].trig & 1) << 1);
        lemoled |= swap << (2*i+8);
      }
      //std::cout << "\n  dut=" << hexdec(dutled) << ", lemo=" << hexdec(lemoled) << std::endl;
      // Swap the two LSBs, to work around routing bug on TLU
      dutled = (dutled & ~3) | ((dutled & 1) << 1) | ((dutled >> 1) & 1);
      // And invert, because 1 means off
      dutled ^= 0xffff;
      // Invert lowest 8 bits (LEMO reset/busy LEDs)
      lemoled ^= 0xff;
      //std::cout << "  dut=" << hexdec(dutled) << ", lemo=" << hexdec(lemoled) << std::endl;
      try {
        WritePCA955(m_addr->TLU_I2C_BUS_MOTHERBOARD, m_addr->TLU_I2C_BUS_MOTHERBOARD_LED_IO, dutled);
      } catch (const eudaq::Exception & e) {
        std::cerr << "Error writing DUT LEDs: " << e.what() << std::endl;
      }
      uint32_t addr = m_addr->TLU_I2C_BUS_LEMO_LED_IO;
      if (m_version < 3) addr = m_addr->TLU_I2C_BUS_LEMO_LED_IO_VB;
      try {
        WritePCA955(m_addr->TLU_I2C_BUS_LEMO, addr, lemoled);
      } catch (const eudaq::Exception & e) {
        std::cerr << "Error writing LEMO LEDs: " << e.what() << std::endl;
      }
    }
  }

  void TLUController::SelectBus(unsigned bus) {
    // select i2c bus
#if TLUDEBUG
    std::cerr << "DEBUG: SelectBus " << bus << std::endl;
#endif
    WriteI2Clines(1, 1);
    I2Cdelay();
    WriteRegister(m_addr->TLU_DUT_I2C_BUS_SELECT_ADDRESS, bus);
    I2Cdelay();
  }

  void TLUController::WritePCA955(unsigned bus, unsigned device, unsigned data) {
    if (m_version < 2) return;
#if TLUDEBUG
    std::cout << "DEBUG: PCA955 bus=" << bus << ", device=" << hexdec(device) << ", data=" << hexdec(data) << std::endl;
#endif

    SelectBus(bus);
    // set pca955 io as output
    WriteI2C16((PCA955_HW_ADDR << 3) | device, PCA955_CONFIG0_REGISTER, 0);
    I2Cdelay(200);
    // write pca955 io data
    WriteI2C16((PCA955_HW_ADDR << 3) | device, PCA955_OUTPUT0_REGISTER, data);
  }

  void TLUController::WriteI2C16(unsigned device, unsigned command, unsigned data) {
#if TLUDEBUG
    std::cout << "DEBUG: I2C16 device=" << device << ", command=" << eudaq::hexdec(command)
              << ", data=" << hexdec(data) << std::endl;
#endif

    // execute i2c start
    WriteI2Clines(1, 1);
    I2Cdelay();
    WriteI2Clines(1, 0);
    I2Cdelay();
    //
    WriteI2Cbyte(device << 1); // lsb=0 for write
    WriteI2Cbyte(command);
    WriteI2Cbyte(data & 0xff);
    WriteI2Cbyte(data >> 8);
    // execute i2c stop
    WriteI2Clines(0, 0);
    I2Cdelay();
    WriteI2Clines(1, 0);
    I2Cdelay();
    WriteI2Clines(1, 1);
    I2Cdelay();
  }

  void TLUController::WriteI2Cbyte(unsigned data) {
#if TLUDEBUG
    std::cout << "DEBUG: I2C data=" << hexdec(data, 2) << std::endl;
#endif
    for (int i = 0; i < 8; ++i) {
      bool sda = (data >> (7-i)) & 1;
   	  WriteI2Clines(0, sda);
      I2Cdelay();
  	  WriteI2Clines(1, sda);
        I2Cdelay();
    }
    // check for ack
    WriteI2Clines(0, 1);
    I2Cdelay();
    if (WriteI2Clines(1, 1)) {
      throw eudaq::Exception("I2C device failed to acknowledge");
    }
  }

  bool TLUController::WriteI2Clines(bool scl, bool sda) {
#if TLUDEBUG
    //std::cout << scl << sda << ",";
#endif
    WriteRegister(m_addr->TLU_DUT_I2C_BUS_DATA_ADDRESS,
                  (scl << m_addr->TLU_I2C_SCL_OUT_BIT) | (sda << m_addr->TLU_I2C_SDA_OUT_BIT));
    I2Cdelay();
    return ReadRegisterRaw(m_addr->TLU_DUT_I2C_BUS_DATA_ADDRESS) & (1 << m_addr->TLU_I2C_SDA_IN_BIT);
  }

}

