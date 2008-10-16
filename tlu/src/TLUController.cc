#include "tlu/TLUController.hh"
#include "tlu/USBTracer.hh"
#include "tlu/TLUAddresses.hh"
#include "eudaq/Platform.hh"
#include "eudaq/Exception.hh"
#include "eudaq/Timer.hh"

#if EUDAQ_PLATFORM_IS(WIN32)
# include <cstdio>  // HK
# include <cstdlib>  // HK
#else
# include <unistd.h>
#endif

#include <iostream>
#include <ostream>
#include <fstream>
#include <iomanip>

using eudaq::mSleep;
using eudaq::hexdec;
using eudaq::to_string;

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
        abort();
      }
    }

    static void I2Cdelay(unsigned us = 100) {
      eudaq::Timer t;
      do {
        // wait
      } while (t.uSeconds() < us);
      //usleep(us);
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

  // Error handler function
  TLUController::TLUController(int errorhandler) :
    //m_filename(""),
    //m_errorhandler(0),
    m_mask(0),
    m_vmask(0),
    m_amask(0),
    m_omask(0),
    m_triggerint(0),
    m_inhibit(true),
    m_vetostatus(0),
    m_fsmstatus(0),
    m_ledstatus(0),
    m_triggernum(0),
    m_timestamp(0),
    m_oldbuf(0),
    m_particles(0),
    m_lasttime(0),
    m_errorhandler(errorhandler),
    m_addr(0)
  {
    errorhandleraborts(errorhandler == 0);
    for (int i = 0; i < TLU_TRIGGER_INPUTS; ++i) {
      m_scalers[i] = 0;
    }
    if (m_filename == "") m_filename = "TLU_Toplevel.bit";
//     for (unsigned i = 0; i < BUFFER_DEPTH; ++i) {
//       m_oldbuf[i] = 0;
//     }

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
    ZestSC1ConfigureFromFile(m_handle, const_cast<char*>(m_filename.c_str()));
    InhibitTriggers(true);
  }

  void TLUController::Initialize() {
    // set up beam trigger
    WriteRegister(m_addr->TLU_BEAM_TRIGGER_VMASK_ADDRESS, m_vmask);
    WriteRegister(m_addr->TLU_BEAM_TRIGGER_AMASK_ADDRESS, m_amask);
    WriteRegister(m_addr->TLU_BEAM_TRIGGER_OMASK_ADDRESS, m_omask);

    // Write to reset a few times...
    //for (int i=0; i<10 ; i++) {
    //mysleep (100);
    mSleep(1);
    WriteRegister(m_addr->TLU_DUT_RESET_ADDRESS, 0x3F);
    WriteRegister(m_addr->TLU_DUT_RESET_ADDRESS, 0x00);
    //}

    // Reset pointers
    WriteRegister(m_addr->TLU_RESET_REGISTER_ADDRESS, 0x0F);
    WriteRegister(m_addr->TLU_RESET_REGISTER_ADDRESS, 0x00);

    WriteRegister(m_addr->TLU_INTERNAL_TRIGGER_INTERVAL, m_triggerint);

    SetDUTMask(m_mask);
    //SetLeftLEDs(m_mask, m_mask);

    WriteI2C(PCA955_HW_ADDR, m_addr->TLU_I2C_BUS_MOTHERBOARD,
             m_addr->TLU_I2C_BUS_MOTHERBOARD_TRIGGER__ENABLE_IPSEL_IO, 0xff00); // turn mb and lemo trigger outputs on
    WriteI2C(PCA955_HW_ADDR, m_addr->TLU_I2C_BUS_MOTHERBOARD,
             m_addr->TLU_I2C_BUS_MOTHERBOARD_RESET_ENABLE_IO, 0xff00); // turn mb and lemo reset outputs on
    WriteI2C(PCA955_HW_ADDR, m_addr->TLU_I2C_BUS_MOTHERBOARD,
             m_addr->TLU_I2C_BUS_MOTHERBOARD_FRONT_PANEL_IO, 0xffff); // select mb busy input
  }

  TLUController::~TLUController() {
    delete[] m_oldbuf;
    ZestSC1CloseCard(m_handle);
  }

  void TLUController::Configure() {
    if (m_version == 0) {
      if (m_serial < FIRSTV2SERIAL) {
        m_version = 1;
      } else {
        m_version = 2;
      }
    }
    if (m_version == 1) {
      m_addr = &v0_1;
    } else {
      m_addr = &v0_2;
    }
    if (!m_oldbuf) m_oldbuf = new unsigned long long[m_addr->TLU_BUFFER_DEPTH];
    if (m_filename == "") {
      m_filename = "TLU";
      if (m_version == 2) m_filename += "2";
      m_filename += "_Toplevel.bit";
    } else if (m_filename.find_first_not_of("0123456789") == std::string::npos) {
      std::string filename = "../tlu/TLU";
      if (m_version == 2) filename += "2";
      filename += "_Toplevel-" + m_filename + ".bit";
      m_filename = filename;
    }
    LoadFirmware();
    Initialize();
    //InhibitTriggers(true);
  }

  void TLUController::Start() {
    // restart triggers
    //ResetTriggerCounter();
    //Initialize();

    SetLEDs(m_mask, m_mask);
    InhibitTriggers(false);
  }

  void TLUController::Stop() {
    InhibitTriggers(true);
    //SetLEDs(0, 0);
  }

  void TLUController::ResetTriggerCounter() {
    WriteRegister(m_addr->TLU_RESET_REGISTER_ADDRESS, 1 << m_addr->TLU_TRIGGER_COUNTER_RESET_BIT);
    WriteRegister(m_addr->TLU_RESET_REGISTER_ADDRESS, 0);
  }

  void TLUController::ResetScalers() {
#ifdef TRIGGER_SCALERS_RESET_BIT
    WriteRegister(m_addr->TLU_RESET_REGISTER_ADDRESS, 1 << m_addr->TLU_TRIGGER_SCALERS_RESET_BIT);
    WriteRegister(m_addr->TLU_RESET_REGISTER_ADDRESS, 0);
#else
    EUDAQ_THROW("Not implemented");
#endif
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

  void TLUController::SetDUTMask(unsigned char mask) {
    m_mask = mask;
    if (m_addr) WriteRegister(m_addr->TLU_DUT_MASK_ADDRESS, m_mask);
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

  void TLUController::Update() {
    bool oldinhibit = InhibitTriggers();

    WriteRegister(m_addr->TLU_STATE_CAPTURE_ADDRESS, 0xFF);

#if TLUDEBUG
    //std::cout << "TLU::Update: after initial write" << std::endl;
#endif
    unsigned entries = ReadRegister16(m_addr->TLU_REGISTERED_BUFFER_POINTER_ADDRESS_0);

#if TLUDEBUG
    std::cout << "TLU::Update: after 1 read, entries " << entries << std::endl;
#endif

    unsigned long long * timestamp_buffer = ReadBlock(entries);

#if TLUDEBUG
    //std::cout << "TLU::Update: after 2 reads" << std::endl;
#endif

    // Reset buffer pointer
    WriteRegister(m_addr->TLU_RESET_REGISTER_ADDRESS, 1 << m_addr->TLU_BUFFER_POINTER_RESET_BIT);
    WriteRegister(m_addr->TLU_RESET_REGISTER_ADDRESS, 0);

    InhibitTriggers(oldinhibit);

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
    if (timestamp_buffer) {
      int trig = m_triggernum - entries;
      for (unsigned i = 0; i < entries; ++i) {
        m_buffer.push_back(TLUEntry(timestamp_buffer[i], ++trig));
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

  void TLUController::Print(std::ostream &out) const {
    for (size_t i = 0; i < m_buffer.size(); ++i) {
      unsigned long long d = m_buffer[i].Timestamp() - m_lasttime;
      out << " " << std::setw(8) << m_buffer[i] << ", diff=" << d << (d <= 0 ? "***" : "") << "\n";
      m_lasttime = m_buffer[i].Timestamp();
    }
    out << "Status:    FSM:" << m_fsmstatus << " Veto:" << m_vetostatus << " BUF:" << m_buffer.size() << "\n"
        << "Scalers:   ";
    for (int i = 0; i < TLU_TRIGGER_INPUTS; ++i) {
      std::cout << m_scalers[i] << (i < (TLU_TRIGGER_INPUTS - 1) ? ", " : "\n");
    }
    out << "Particles: " << m_particles << "\n"
        << "Triggers:  " << m_triggernum << "\n"
        << "Timestamp: " << eudaq::hexdec(m_timestamp, 0)
        << " = " << Timestamp2Seconds(m_timestamp) << std::endl;
  }

  unsigned TLUController::GetVersion() const {
    return m_version;
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

  void TLUController::SetLEDs(int left, int right) {
    for (int i = 0; i < 8; ++i) {
      unsigned bitr = 1 << 2*i, bitl = bitr << 1;
      if (right >= 0) {
        if ((right >> i) & 1) {
          m_ledstatus |= bitr;
        } else {
          m_ledstatus &= ~bitr;
        }
      }
      if (left >= 0) {
        if ((left >> i) & 1) {
          m_ledstatus |= bitl;
        } else {
          m_ledstatus &= ~bitl;
        }
      }
    }
    if (m_addr) {
      if (m_version == 1) {
        WriteRegister(m_addr->TLU_DUT_LED_ADDRESS, left);
      } else {
        unsigned data = (m_ledstatus & ~2) | ((m_ledstatus & 1) << 1) | ((m_ledstatus >> 1) & 1);
        data = ~data & 0xffff;
        WriteI2C(PCA955_HW_ADDR, m_addr->TLU_I2C_BUS_MOTHERBOARD, m_addr->TLU_I2C_BUS_MOTHERBOARD_LED_IO, data);
      }
    }
  }

  void TLUController::SetLemoLEDs(unsigned val) {
    WriteI2C(PCA955_HW_ADDR, m_addr->TLU_I2C_BUS_LEMO, m_addr->TLU_I2C_BUS_LEMO_LED_IO, val);
  }

  void TLUController::WriteI2C(unsigned hw, unsigned bus, unsigned device, unsigned data) {
    if (m_version < 2) return;
#if TLUDEBUG
    std::cout << "DEBUG: PCA955 bus=" << bus << ", device=" << device << ", data=" << hexdec(data) << std::endl;
#endif
    // select i2c bus
    WriteI2Clines(1, 1);
    I2Cdelay();
    WriteRegister(m_addr->TLU_DUT_I2C_BUS_SELECT_ADDRESS, bus);
    I2Cdelay();
    // set pca955 io as output
    WriteI2C16((hw << 3) | device, PCA955_CONFIG0_REGISTER, 0);
    I2Cdelay(2500); // for scope debugging
    // write pca955 io data
    WriteI2C16((hw << 3) | device, PCA955_OUTPUT0_REGISTER, data);
  }

  void TLUController::WriteI2C16(unsigned device, unsigned command, unsigned data) {
#if TLUDEBUG
    std::cout << "DEBUG: I2C16 device=" << device << ", command=" << command
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
      EUDAQ_THROW("I2C device failed to acknowledge");
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
