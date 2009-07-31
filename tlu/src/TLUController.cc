#include "tlu/TLUController.hh"
#include "tlu/USBTracer.hh"
#include "tlu/TLUAddresses.hh"
#include "eudaq/Platform.hh"
#include "eudaq/Exception.hh"
#include "eudaq/Timer.hh"

#if EUDAQ_PLATFORM_IS(WIN32)
# include <cstdio>  // HK
#else
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
using eudaq::ucase;

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

  static const unsigned long long NOTIMESTAMP = (unsigned long long)-1;

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
    static const unsigned FIRSTV2SERIAL = 1000;

    static const char * g_versions[] = {
      "Unknown",
      "v0.1",
      "v0.2a",
      "v0.2c"
    };

//     static const unsigned long g_scaler_address[TLU_TRIGGER_INPUTS] = {
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

  double Timestamp2Seconds(unsigned long long t) {
    return t / TLUFREQUENCY;
  }

  void TLUEntry::Print(std::ostream & out) const {
    out << m_eventnum
        << ", " << eudaq::hexdec(m_timestamp, 0)
        << " = " << Timestamp2Seconds(m_timestamp);
  }

  TLUController::TLUController(int errorhandler) :
    m_mask(0),
    m_vmask(0),
    m_amask(0),
    m_omask(0),
    m_ipsel(0xff),
    m_triggerint(0),
    m_inhibit(true),
    m_vetostatus(0),
    m_fsmstatus(0),
    m_triggernum(0),
    m_timestamp(0),
    m_oldbuf(0),
    m_particles(0),
    m_lasttime(0),
    m_errorhandler(errorhandler),
    m_version(0),
    m_addr(0),
    m_timestampzero(0)
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
    int status = ZestSC1CountCards(&NumCards, CardIDs, SerialNumbers, FPGATypes);
    if (status != 0) throw TLUException("ZestSC1CountCards", status);

#if TLUDEBUG
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
#endif

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
      m_filename = "TLU";
      if (m_version > 1) m_filename += "2";
      m_filename += "_Toplevel.bit";
    } else if (m_filename.find_first_not_of("0123456789") == std::string::npos) {
      std::string filename = "../tlu/TLU";
      if (m_version == 2) filename += "2";
      filename += "_Toplevel-" + m_filename + ".bit";
      m_filename = filename;
    }
    ZestSC1ConfigureFromFile(m_handle, const_cast<char*>(m_filename.c_str()));
    InhibitTriggers(true);
  }

  void TLUController::Initialize() {
    if (m_version == 2) {
      if (!SetupLemo()) {
        // If LEMO ADC does not respond, we must have a v0.2c TLU, so increment version number
        m_version++;
      }
    }

    // set up beam trigger
    WriteRegister(m_addr->TLU_BEAM_TRIGGER_VMASK_ADDRESS, m_vmask);
    WriteRegister(m_addr->TLU_BEAM_TRIGGER_AMASK_ADDRESS, m_amask);
    WriteRegister(m_addr->TLU_BEAM_TRIGGER_OMASK_ADDRESS, m_omask);

    // Write to reset a few times...
    //for (int i=0; i<10 ; i++) {
    //mysleep (100);
    mSleep(20);
    WriteRegister(m_addr->TLU_DUT_RESET_ADDRESS, 0x3F);
    WriteRegister(m_addr->TLU_DUT_RESET_ADDRESS, 0x00);
    //}

    // Reset pointers
    WriteRegister(m_addr->TLU_RESET_REGISTER_ADDRESS, (1 << m_addr->TLU_TRIGGER_COUNTER_RESET_BIT)
                  | (1 << m_addr->TLU_BUFFER_POINTER_RESET_BIT) | (1 << m_addr->TLU_TRIGGER_FSM_RESET_BIT));
    WriteRegister(m_addr->TLU_RESET_REGISTER_ADDRESS, 0x00);

    WriteRegister(m_addr->TLU_INTERNAL_TRIGGER_INTERVAL, m_triggerint);

    SetDUTMask(m_mask, false);

    WritePCA955(m_addr->TLU_I2C_BUS_MOTHERBOARD,
                m_addr->TLU_I2C_BUS_MOTHERBOARD_TRIGGER__ENABLE_IPSEL_IO, 0xff00); // turn mb and lemo trigger outputs on
    WritePCA955(m_addr->TLU_I2C_BUS_MOTHERBOARD,
                m_addr->TLU_I2C_BUS_MOTHERBOARD_RESET_ENABLE_IO, 0xff00); // turn mb and lemo reset outputs on
  }

  TLUController::~TLUController() {
    delete[] m_oldbuf;
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
    if (!m_oldbuf) m_oldbuf = new unsigned long long[m_addr->TLU_BUFFER_DEPTH];
    LoadFirmware();
    Initialize();
  }

  bool TLUController::SetupLemo() {
    SelectBus(m_addr->TLU_I2C_BUS_LEMO);
    try {
      // TTL (0x3) -> -0.5
      // NIM (0xC) ->  0.4
      WriteI2C16((AD5316_HW_ADDR << 2) | m_addr->TLU_I2C_BUS_LEMO_ADC,
                 0x3, 0xf000 | (lemo_dac_value(-0.5) << 2));
      WriteI2C16((AD5316_HW_ADDR << 2) | m_addr->TLU_I2C_BUS_LEMO_ADC,
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
    WriteRegister(m_addr->TLU_RESET_REGISTER_ADDRESS, 0);
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

  void TLUController::SetTriggerInterval(unsigned millis) {
    m_triggerint = millis;
    if (m_addr) WriteRegister(m_addr->TLU_INTERNAL_TRIGGER_INTERVAL, m_triggerint);
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

  void TLUController::Update(bool timestamps) {
    unsigned entries = 0;
    unsigned long long * timestamp_buffer = 0;
    if (timestamps) {
      bool oldinhibit = InhibitTriggers();

      WriteRegister(m_addr->TLU_STATE_CAPTURE_ADDRESS, 0xFF);

#if TLUDEBUG
      //std::cout << "TLU::Update: after initial write" << std::endl;
#endif
      entries = ReadRegister16(m_addr->TLU_REGISTERED_BUFFER_POINTER_ADDRESS_0);

#if TLUDEBUG
      std::cout << "TLU::Update: after 1 read, entries " << entries << std::endl;
#endif

      timestamp_buffer = ReadBlock(entries);

#if TLUDEBUG
      //std::cout << "TLU::Update: after 2 reads" << std::endl;
#endif

      // Reset buffer pointer
      WriteRegister(m_addr->TLU_RESET_REGISTER_ADDRESS, 1 << m_addr->TLU_BUFFER_POINTER_RESET_BIT);
      WriteRegister(m_addr->TLU_RESET_REGISTER_ADDRESS, 0);

      InhibitTriggers(oldinhibit);
    } else {
      WriteRegister(m_addr->TLU_STATE_CAPTURE_ADDRESS, 0xFF);
      entries = ReadRegister16(m_addr->TLU_REGISTERED_BUFFER_POINTER_ADDRESS_0);
      WriteRegister(m_addr->TLU_RESET_REGISTER_ADDRESS, 1 << m_addr->TLU_BUFFER_POINTER_RESET_BIT);
      WriteRegister(m_addr->TLU_RESET_REGISTER_ADDRESS, 0);
    }

#if TLUDEBUG
    std::cout << "TLU::Update: entries=" << entries << std::endl;
#endif

    m_fsmstatus = ReadRegister8(m_addr->TLU_TRIGGER_FSM_STATUS_ADDRESS);
    m_vetostatus = ReadRegister8(m_addr->TLU_TRIG_INHIBIT_ADDRESS);
#if TLUDEBUG
    //std::cout << "TLU::Update: fsm " << m_fsmstatus << " veto " << m_vetostatus << std::endl;
#endif

    m_triggernum = ReadRegister32(m_addr->TLU_REGISTERED_TRIGGER_COUNTER_ADDRESS_0);
    m_timestamp = ReadRegister64(m_addr->TLU_REGISTERED_TIMESTAMP_ADDRESS_0);
#if TLUDEBUG
    std::cout << "TLU::Update: trigger " << m_triggernum << " timestamp " << m_timestamp << std::endl;
    std::cout << "TLU::Update: scalers";
#endif

    for (int i = 0; i < TLU_TRIGGER_INPUTS; ++i) {
      m_scalers[i] = ReadRegister16(m_addr->TLU_SCALERS(i));
#if TLUDEBUG
      std::cout << ", [" << i << "] " << m_scalers[i];
#endif
    }
#if TLUDEBUG
    std::cout << std::endl;
#endif

    m_particles = ReadRegister32(m_addr->TLU_REGISTERED_PARTICLE_COUNTER_ADDRESS_0);

    // Read timestamp buffer of BUFFER_DEPTH entries from TLU
    m_buffer.clear();
    int trig = m_triggernum - entries;
    for (unsigned i = 0; i < entries; ++i) {
      m_buffer.push_back(TLUEntry(timestamp_buffer ? timestamp_buffer[i] : NOTIMESTAMP, trig++));
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

  void TLUController::WriteRegister(unsigned long offset, unsigned char val) {
    int status = ZESTSC1_SUCCESS;
    int delay = 0;
    const int count = m_errorhandler ? m_errorhandler : 1;
    for (int i = 0; i < count; ++i) {
      if (delay == 0) {
        delay = 20;
      } else {
        usleep(delay);
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

  unsigned char TLUController::ReadRegister8(unsigned long offset) const {
    unsigned char val = ReadRegisterRaw(offset);
    //usbtrace(" R", offset, val);
    return val;
  }

  unsigned short TLUController::ReadRegister16(unsigned long offset) const {
    unsigned short val = ReadRegisterRaw(offset);
    val |= static_cast<unsigned short>(ReadRegisterRaw(offset+1)) << 8;
    //usbtrace(" R", offset, val);
    return val;
  }

  unsigned long TLUController::ReadRegister32(unsigned long offset) const {
    unsigned long val = 0;
    for (int i = 0; i < 4; ++i) {
      val |= static_cast<unsigned long>(ReadRegisterRaw(offset+i)) << (8*i);
    }
    //usbtrace(" R", offset, val);
    return val;
  }

  unsigned long long TLUController::ReadRegister64(unsigned long offset) const {
    unsigned long long val = 0;
    for (int i = 0; i < 8; ++i) {
      val |= static_cast<unsigned long long>(ReadRegisterRaw(offset+i)) << (8*i);
    }
    //usbtrace(" R", offset, val);
    return val;
  }

  unsigned char TLUController::ReadRegisterRaw(unsigned long offset) const {
    unsigned char val = 0;
    int status = ZESTSC1_SUCCESS;
    int delay = 0;
    const int count = m_errorhandler ? m_errorhandler : 1;
    for (int i = 0; i < count; ++i) {
      if (delay == 0) {
        delay = 20;
      } else {
        usleep(delay);
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

  unsigned long long * TLUController::ReadBlock(unsigned entries) {
    if (!entries) return 0;

    unsigned long long buffer[4096]; // should be m_addr->TLU_BUFFER_DEPTH
    if (m_addr->TLU_BUFFER_DEPTH > 4096) EUDAQ_THROW("Buffer size error");
    for (unsigned i = 0; i < m_addr->TLU_BUFFER_DEPTH; ++i) {
      buffer[i] = m_oldbuf[i];
    }

    int result = ZESTSC1_SUCCESS;
    for (int tries = 0; tries < 3; ++tries) {
      // Request block transfer from TLU
      usleep(10);
      WriteRegister(m_addr->TLU_INITIATE_READOUT_ADDRESS, 0xFF);
      //usleep(10);
      result = ZestSC1ReadData(m_handle, buffer, sizeof buffer);

#if TLUDEBUG
      char * errmsg = 0;
      ZestSC1GetErrorMessage(static_cast<ZESTSC1_STATUS>(result), &errmsg);
      std::cout << (result == ZESTSC1_SUCCESS ? "" : "#### Warning: ") << errmsg << std::endl;
#endif
      if (result == ZESTSC1_SUCCESS && buffer[entries-1] != m_oldbuf[entries-1]) {
        for (unsigned i = 0; i < m_addr->TLU_BUFFER_DEPTH; ++i) {
          m_oldbuf[i] = buffer[i];
        }
        usbtrace("BR", 0, buffer, m_addr->TLU_BUFFER_DEPTH, result);
        return m_oldbuf;
      }
    }
    usbtrace("bR", 0, buffer, m_addr->TLU_BUFFER_DEPTH, result);
    std::cout << (buffer[0] == m_oldbuf[0] ? "*" : "#") << std::flush;
    return 0;
  }

  void TLUController::Print(std::ostream &out, bool timestamps) const {
    if (timestamps) {
      for (size_t i = 0; i < m_buffer.size(); ++i) {
        unsigned long long d = m_buffer[i].Timestamp() - m_lasttime;
        out << " " << std::setw(8) << m_buffer[i] << ", diff=" << d << (d <= 0 ? "***" : "") << "\n";
        m_lasttime = m_buffer[i].Timestamp();
      }
    }
    out << "Status:    FSM:" << m_fsmstatus << " Veto:" << m_vetostatus << " BUF:" << m_buffer.size() << "\n"
        << "Scalers:   ";
    for (int i = 0; i < TLU_TRIGGER_INPUTS; ++i) {
      std::cout << m_scalers[i] << (i < (TLU_TRIGGER_INPUTS - 1) ? ", " : "\n");
    }
    out << "Particles: " << m_particles << "\n"
        << "Triggers:  " << m_triggernum << "\n"
        << "Entries:   " << NumEntries() << "\n"
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
      unsigned long addr = m_addr->TLU_I2C_BUS_LEMO_LED_IO;
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
