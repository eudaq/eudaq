#include "tlu/TLUController.hh"
#include "tlu/TLU_address_map.h"
#include "eudaq/Platform.hh"
#include "eudaq/Utils.hh"

#if EUDAQ_PLATFORM_IS(WIN32)
# include <cstdio>  // HK
# include <cstdlib>  // HK
#else
# include <unistd.h>
#endif

#include <iostream>
#include <ostream>
#include <iomanip>

using eudaq::mSleep;

//#define TLUDEBUG

#ifndef INTERNAL_TRIGGER_INTERVAL_ADDRESS
#define INTERNAL_TRIGGER_INTERVAL_ADDRESS INTERNAL_TRIGGER_INTERVAL
#endif

namespace tlu {

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
    std::string s = std::string("ZestSC1 function ") +
      function + " returned error: " + msg;
    throw TLUException(s);
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
    m_dmastatus(0),
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

    // Request information about the system
    unsigned long NumCards;
    unsigned long CardIDs[256];
    unsigned long SerialNumbers[256];
    ZESTSC1_FPGA_TYPE FPGATypes[256];
    ZestSC1CountCards(&NumCards, CardIDs, SerialNumbers, FPGATypes);

#ifdef TLUDEBUG
    std::cout << "DEBUG: NumCards: " << NumCards << std::endl;
    for (unsigned i = 0; i < NumCards; ++i) {
      std::cout << "DEBUG: Card " << i << std::hex
                << ", ID = 0x" << CardIDs[i]
                << ", SerialNum = 0x" << SerialNumbers[i]
                << ", FPGAType = " << std::dec << FPGATypes[i]
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
    ZestSC1OpenCard(CardIDs[found], &m_handle);

#ifdef TLUDEBUG
    std::cout << "DEBUG: TLU handle = " << m_handle << std::endl;
#endif

    Initialize();
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

  void TLUController::FullReset() {
    WriteRegister(RESET_REGISTER_ADDRESS, 0xff);
    WriteRegister(RESET_REGISTER_ADDRESS, 0);
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

    unsigned entries = ReadRegister16(REGISTERED_BUFFER_POINTER_ADDRESS_0);

    unsigned long long * timestamp_buffer = ReadBlock(entries);

    // Reset buffer pointer
    WriteRegister(RESET_REGISTER_ADDRESS, 1<<BUFFER_POINTER_RESET_BIT);
    WriteRegister(RESET_REGISTER_ADDRESS, 0);

    InhibitTriggers(oldinhibit);

#ifdef TLUDEBUG
    std::cout << "TLU::Update: entries=" << entries << std::endl;
#endif

    m_fsmstatus = ReadRegister(TRIGGER_FSM_STATUS_ADDRESS);
    m_vetostatus = ReadRegister(TRIG_INHIBIT_ADDRESS);

    m_triggernum = ReadRegister32(REGISTERED_TRIGGER_COUNTER_ADDRESS_0);
    m_timestamp = ReadRegister64(REGISTERED_TIMESTAMP_ADDRESS_0);

    for (int i = 0; i < TLU_TRIGGER_INPUTS; ++i) {
      m_scalers[i] = ReadRegister16(g_scaler_address[i]);
    }

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
    ZestSC1WriteRegister(m_handle, offset, val);
  }

  unsigned char TLUController::ReadRegister(unsigned long offset) const {
    unsigned char val;
    ZestSC1ReadRegister(m_handle, offset, &val);
    return val;
  }

  unsigned short TLUController::ReadRegister16(unsigned long offset) const {
    unsigned short val = ReadRegister(offset);
    val |= ReadRegister(offset+1) << 8;
    return val;
  }

  unsigned long TLUController::ReadRegister32(unsigned long offset) const {
    unsigned long val = ReadRegister16(offset);
    val |= ReadRegister16(offset+2) << 16;
    return val;
  }

  unsigned long long TLUController::ReadRegister64(unsigned long offset) const {
    unsigned long long val = ReadRegister32(offset);
    val |= static_cast<unsigned long long>(ReadRegister32(offset+4)) << 32;
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
      //std::cout << "DEBUG: ";
      WriteRegister(INITIATE_READOUT_ADDRESS, 0xFF);
      //int i;
      //for (i = 0; i < 10; ++i) {
      //  m_dmastatus = ReadRegister(DMA_STATUS_ADDRESS);
      //  std::cout << m_dmastatus << std::flush;
      //  if (!m_dmastatus) break;
      //  mSleep(1);
      //}
      //std::cout << ", " << i << std::endl;
      //mSleep(10);
      ZestSC1ReadData(m_handle, buffer, sizeof buffer);
      if (buffer[entries-1] != m_oldbuf[entries-1]) {
        for (unsigned i = 0; i < BUFFER_DEPTH; ++i) {
          m_oldbuf[i] = buffer[i];
        }
        return m_oldbuf;
      }
    }
    std::cout << (buffer[0] == m_oldbuf[0] ? "*" : "#") << std::flush;
    return 0;
  }

  void TLUController::Print(std::ostream &out) const {
    out << "Status:    FSM:" << m_fsmstatus << " Veto:" << m_vetostatus << " BUF:" << m_buffer.size() << std::endl
        << "Scalers:   ";
    for (int i = 0; i < TLU_TRIGGER_INPUTS; ++i) {
      std::cout << m_scalers[i] << (i < (TLU_TRIGGER_INPUTS - 1) ? ", " : "\n");
    }
    out << "Triggers:  " << m_triggernum << std::endl
        << "Timestamp: " << eudaq::hexdec(m_timestamp, 0)
        << " = " << Timestamp2Seconds(m_timestamp) << std::endl;
    for (size_t i = 0; i < m_buffer.size(); ++i) {
      unsigned long long d = m_buffer[i].Timestamp() - m_lasttime;
      out << " " << std::setw(8) << m_buffer[i] << ", diff=" << d << (d <= 0 ? "***" : "") << std::endl;
      m_lasttime = m_buffer[i].Timestamp();
    }
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
