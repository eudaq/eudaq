#include "tlu/TLUController.hh"
#include "tlu/TLU_address_map.h"
#include "eudaq/Platform.hh"
#include "eudaq/Exception.hh"
#include "eudaq/Time.hh"

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

//#define TLUDEBUG

#ifndef INTERNAL_TRIGGER_INTERVAL_ADDRESS
#define INTERNAL_TRIGGER_INTERVAL_ADDRESS INTERNAL_TRIGGER_INTERVAL
#endif

namespace tlu {

class USBTraceFile {
public:
  static USBTraceFile & Instance() {
    static USBTraceFile tracefile;
    return tracefile;
  }
  void Open(const std::string & filename) {
    Close();
    m_filename = filename;
    if (filename != "") {
      m_file.open(filename.c_str(), std::ios::app);
      if (!m_file.is_open()) EUDAQ_THROW("Unable to open USB trace file " + m_filename);
      WriteString("# Log started at " + eudaq::Time::Current().Formatted() + "\n");
    }
  }
  void Close() {
    if (m_file.is_open()) {
      m_file.close();
    }
  }
  void WriteString(const std::string & s) {
    if (m_file.is_open()) {
      m_file.write(s.c_str(), s.size());
      m_file.flush();
    }
  }
  void Write(const char * mode, unsigned long addr, const std::string & data) {
    eudaq::Time time = eudaq::Time::Current();
    if (addr == m_prev_addr && data == m_prev_data && mode == m_prev_mode) {
      m_prev_time = time;
      m_repeated++;
    } else {
      FlushRepeated();
      std::ostringstream s;
      s << time.Formatted("%T.%6") << " " << mode
        << " " << eudaq::hexdec(addr) << " " << data << std::endl;
      WriteString(s.str());
      m_prev_time = time;
      m_prev_mode = mode;
      m_prev_data = data;
      m_prev_addr = addr;
    }
  }
  bool IsOpen() { return m_file.is_open(); }
  ~USBTraceFile() {
    FlushRepeated();
    Close();
  }
private:
  USBTraceFile() : m_repeated(0), m_prev_time(0), m_prev_addr(0) {}
  void FlushRepeated() {
    if (m_repeated) {
      std::ostringstream s;
      s << m_prev_time.Formatted("%T.%6") << " ## Repeated " << m_repeated << " time(s)\n";
      m_repeated = 0;
      WriteString(s.str());
      s.clear();
    }
  }
  std::string m_filename;
  std::ofstream m_file;
  unsigned long m_repeated;
  eudaq::Time m_prev_time;
  std::string m_prev_mode, m_prev_data;
  unsigned long m_prev_addr;
};

void setusbtracefile(const std::string & filename) {
  if (getusbtracelevel() > 0) {
    USBTraceFile::Instance().Open(filename);
  } else {
    EUDAQ_THROW("VME tracing not enabled in this build");
  }
}

void dousbtrace(const char * mode, unsigned long addr, const std::string & data) {
  if (!USBTraceFile::Instance().IsOpen()) return;
  USBTraceFile::Instance().Write(mode, addr, data);
}


  int do_usb_reset(ZESTSC1_HANDLE Handle); // defined in TLU_USB.cc

  namespace {

    static const double TLUFREQUENCY = 48.001e6;

    static const unsigned long g_scaler_address[TLU_TRIGGER_INPUTS] = {
      TRIGGER_IN0_COUNTER_0,
      TRIGGER_IN1_COUNTER_0,
      TRIGGER_IN2_COUNTER_0,
      TRIGGER_IN3_COUNTER_0
    };

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
  void DefaultErrorHandler(const char * function,
                           ZESTSC1_HANDLE handle,
                           ZESTSC1_STATUS status,
                           const char *msg) {
    (void)handle;
    (void)status;
    std::cout << "ZESTSC1 ERROR:  function " << function << " returned error: " << msg << std::endl;
    //throw TLUException(s);
  }

  TLUController::TLUController(const std::string  & filename, ErrorHandler err) :
    m_filename(filename),
    //m_errorhandler(0),
    m_mask(1),
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
    m_oldbuf(new unsigned long long [BUFFER_DEPTH]),
    m_lasttime(0)
  {
    for (int i = 0; i < TLU_TRIGGER_INPUTS; ++i) {
      m_scalers[i] = 0;
    }
    if (m_filename == "") m_filename = "TLU_Toplevel.bit";
    for (unsigned i = 0; i < BUFFER_DEPTH; ++i) {
      m_oldbuf[i] = 0;
    }

    // Install an error handler
    ZestSC1RegisterErrorHandler(err ? err : DefaultErrorHandler);

    m_handle = OpenTLU();

#ifdef TLUDEBUG
    std::cout << "DEBUG: TLU handle = " << m_handle << std::endl;
#endif

    Initialize();
  }

  ZESTSC1_HANDLE TLUController::OpenTLU() {
    // Request information about the system
    unsigned long NumCards = 0;
    unsigned long CardIDs[256];
    unsigned long SerialNumbers[256];
    ZESTSC1_FPGA_TYPE FPGATypes[256];
    ZestSC1CountCards(&NumCards, CardIDs, SerialNumbers, FPGATypes);

#ifdef TLUDEBUG
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


    // Open the card
    ZESTSC1_HANDLE handle;
    ZestSC1OpenCard(CardIDs[found], &handle);
    ZestSC1SetTimeOut(handle, 200);
    return handle;
  }

  void TLUController::Initialize() {
    ZestSC1ConfigureFromFile(m_handle, const_cast<char*>(m_filename.c_str()));
    InhibitTriggers(true);

    // set up beam trigger
    WriteRegister(BEAM_TRIGGER_VMASK_ADDRESS, m_vmask);
    WriteRegister(BEAM_TRIGGER_AMASK_ADDRESS, m_amask);
    WriteRegister(BEAM_TRIGGER_OMASK_ADDRESS, m_omask);

    // Write to reset a few times...
    //for (int i=0; i<10 ; i++) {
    //mysleep (100);
    mSleep(1);
    WriteRegister(DUT_RESET_ADDRESS, 0x3F);
    WriteRegister(DUT_RESET_ADDRESS, 0x00);
    //}

    // Reset pointers
    WriteRegister(RESET_REGISTER_ADDRESS, 0x0F);
    WriteRegister(RESET_REGISTER_ADDRESS, 0);

    WriteRegister(INTERNAL_TRIGGER_INTERVAL_ADDRESS, m_triggerint);

    // Set input mask
    WriteRegister(DUT_MASK_ADDRESS, m_mask);

    SetLEDs(0);
  }

  TLUController::~TLUController() {
    delete[] m_oldbuf;
    ZestSC1CloseCard(m_handle);
  }

  void TLUController::Start() {
    // restart triggers
    //ResetTriggerCounter();

    Initialize();

    SetLEDs(m_mask);
    InhibitTriggers(false);
  }

  void TLUController::Stop() {
    InhibitTriggers(true);
    SetLEDs(0);
  }

  void TLUController::ResetTriggerCounter() {
    WriteRegister(RESET_REGISTER_ADDRESS, 1<<TRIGGER_COUNTER_RESET_BIT);
    WriteRegister(RESET_REGISTER_ADDRESS, 0);
  }

  void TLUController::ResetScalers() {
    WriteRegister(RESET_REGISTER_ADDRESS, 1<<TRIGGER_SCALERS_RESET_BIT);
    WriteRegister(RESET_REGISTER_ADDRESS, 0);
  }

  void TLUController::ResetUSB() {
    do_usb_reset(m_handle);
    // this fails with error:
    // "The requested card ID does not correspond to any devices in the system"
    // Why?
    m_handle = OpenTLU();
  }

  void TLUController::SetDUTMask(unsigned char mask) {
    WriteRegister(DUT_MASK_ADDRESS, m_mask = mask);
  }

  void TLUController::SetVetoMask(unsigned char mask) {
    WriteRegister(BEAM_TRIGGER_VMASK_ADDRESS, m_vmask = mask);
  }

  void TLUController::SetAndMask(unsigned char mask) {
    WriteRegister(BEAM_TRIGGER_AMASK_ADDRESS, m_amask = mask);
  }

  void TLUController::SetOrMask(unsigned char mask) {
    WriteRegister(BEAM_TRIGGER_OMASK_ADDRESS, m_omask = mask);
  }

  void TLUController::SetTriggerInterval(unsigned millis) {
    WriteRegister(INTERNAL_TRIGGER_INTERVAL_ADDRESS, m_triggerint = millis);
  }

  unsigned char TLUController::GetAndMask() const {
    return ReadRegister(BEAM_TRIGGER_AMASK_ADDRESS);
  }

  unsigned char TLUController::GetOrMask() const {
    return ReadRegister(BEAM_TRIGGER_OMASK_ADDRESS);
  }

  unsigned char TLUController::GetVetoMask() const {
    return ReadRegister(BEAM_TRIGGER_VMASK_ADDRESS);
  }

  void TLUController::Update() {
    bool oldinhibit = m_inhibit;
    InhibitTriggers(true);

    WriteRegister(STATE_CAPTURE_ADDRESS, 0xFF);

#ifdef TLUDEBUG
    //std::cout << "TLU::Update: after initial write" << std::endl;
#endif
    unsigned entries = ReadRegister16(REGISTERED_BUFFER_POINTER_ADDRESS_0);

#ifdef TLUDEBUG
    //std::cout << "TLU::Update: after 1 read, entries " << entries << std::endl;
#endif

    unsigned long long * timestamp_buffer = ReadBlock(entries);

#ifdef TLUDEBUG
    //std::cout << "TLU::Update: after 2 reads" << std::endl;
#endif

    // Reset buffer pointer
    WriteRegister(RESET_REGISTER_ADDRESS, 1<<BUFFER_POINTER_RESET_BIT);
    WriteRegister(RESET_REGISTER_ADDRESS, 0);

    InhibitTriggers(oldinhibit);

#ifdef TLUDEBUG
    //std::cout << "TLU::Update: entries=" << entries << std::endl;
#endif

    m_fsmstatus = ReadRegister(TRIGGER_FSM_STATUS_ADDRESS);
    m_vetostatus = ReadRegister(TRIG_INHIBIT_ADDRESS);
#ifdef TLUDEBUG
    //std::cout << "TLU::Update: fsm " << m_fsmstatus << " veto " << m_vetostatus << std::endl;
#endif

    m_triggernum = ReadRegister32(REGISTERED_TRIGGER_COUNTER_ADDRESS_0);
    m_timestamp = ReadRegister64(REGISTERED_TIMESTAMP_ADDRESS_0);
#ifdef TLUDEBUG
    //std::cout << "TLU::Update: trigger " << m_triggernum << " timestamp " << m_timestamp << std::endl;
    //std::cout << "TLU::Update: scalers";
#endif

    for (int i = 0; i < TLU_TRIGGER_INPUTS; ++i) {
      m_scalers[i] = ReadRegister16(g_scaler_address[i]);
#ifdef TLUDEBUG
      //std::cout << ", [" << i << "] " << m_scalers[i];
#endif
    }
#ifdef TLUDEBUG
    //std::cout << std::endl;
#endif

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
    return ReadRegister(TRIG_INHIBIT_ADDRESS);
  }


  void TLUController::InhibitTriggers(bool inhibit) {
    WriteRegister(TRIG_INHIBIT_ADDRESS, inhibit);
    m_inhibit = inhibit;
  }

  void TLUController::WriteRegister(unsigned long offset, unsigned char val) {
    usbtrace(" W", offset, val);
    ZestSC1WriteRegister(m_handle, offset, val);
  }

  unsigned char TLUController::ReadRegister(unsigned long offset) const {
    unsigned char val;
    ZestSC1ReadRegister(m_handle, offset, &val);
    usbtrace(" R", offset, val);
    return val;
  }

  unsigned short TLUController::ReadRegister16(unsigned long offset) const {
    unsigned short val = ReadRegister(offset);
    val |= ReadRegister(offset+1) << 8;
    usbtrace(" R", offset, val);
    return val;
  }

  unsigned long TLUController::ReadRegister32(unsigned long offset) const {
    unsigned long val = ReadRegister16(offset);
    val |= ReadRegister16(offset+2) << 16;
    usbtrace(" R", offset, val);
    return val;
  }

  unsigned long long TLUController::ReadRegister64(unsigned long offset) const {
    unsigned long long val = ReadRegister32(offset);
    val |= static_cast<unsigned long long>(ReadRegister32(offset+4)) << 32;
    usbtrace(" R", offset, val);
    return val;
  }

  unsigned long long * TLUController::ReadBlock(unsigned entries) {
    if (!entries) return 0;

    unsigned long long buffer[BUFFER_DEPTH];
    for (unsigned i = 0; i < BUFFER_DEPTH; ++i) {
      buffer[i] = m_oldbuf[i];
    }

    for (int tries = 0; tries < 3; ++tries) {
      // Request block transfer from TLU
      WriteRegister(INITIATE_READOUT_ADDRESS, 0xFF);
#ifdef TLUDEBUG
      //std::cout << "ReadBlock 1st write, tries " << tries << std::endl;
#endif
      int result = ZestSC1ReadData(m_handle, buffer, sizeof buffer);

#ifdef TLUDEBUG
      std::cout << (result == ZESTSC1_SUCCESS ? "" : "#### Warning: ") << "ZestSC1ReadData returned " << result << std::endl;
#endif
      if (result == ZESTSC1_SUCCESS && buffer[entries-1] != m_oldbuf[entries-1]) {
        for (unsigned i = 0; i < BUFFER_DEPTH; ++i) {
          m_oldbuf[i] = buffer[i];
        }
        usbtrace("BR", 0, buffer, BUFFER_DEPTH);
        return m_oldbuf;
      }
    }
    usbtrace("bR", 0, buffer, BUFFER_DEPTH);
    std::cout << (buffer[0] == m_oldbuf[0] ? "*" : "#") << std::flush;
    return 0;
  }

  void TLUController::Print(std::ostream &out) const {
    for (size_t i = 0; i < m_buffer.size(); ++i) {
      unsigned long long d = m_buffer[i].Timestamp() - m_lasttime;
      out << " " << std::setw(8) << m_buffer[i] << ", diff=" << d << (d <= 0 ? "***" : "") << std::endl;
      m_lasttime = m_buffer[i].Timestamp();
    }
    out << "Status:    FSM:" << m_fsmstatus << " Veto:" << m_vetostatus << " BUF:" << m_buffer.size() << std::endl
        << "Scalers:   ";
    for (int i = 0; i < TLU_TRIGGER_INPUTS; ++i) {
      std::cout << m_scalers[i] << (i < (TLU_TRIGGER_INPUTS - 1) ? ", " : "\n");
    }
    out << "Triggers:  " << m_triggernum << std::endl
        << "Timestamp: " << eudaq::hexdec(m_timestamp, 0)
        << " = " << Timestamp2Seconds(m_timestamp) << std::endl;
  }

  unsigned TLUController::GetFirmwareID() const {
    return ReadRegister(FIRMWARE_ID_ADDRESS);
  }

  unsigned TLUController::GetLibraryID() {
    return FIRMWARE_ID;
  }

  void TLUController::SetLEDs(unsigned int val) {
    WriteRegister(DUT_LED_ADDRESS, m_ledstatus = val);
  }

  unsigned TLUController::GetLEDs() const {
    return m_ledstatus;
  }

}
