#include "tlu/TLUController.hh"
#include "tlu/TLU_address_map.h"
#include "eudaq/Utils.hh"
#include <unistd.h>
#include <iostream>
#include <ostream>
#include <iomanip>

using eudaq::mSleep;

#ifndef INTERNAL_TRIGGER_INTERVAL_ADDRESS
#define INTERNAL_TRIGGER_INTERVAL_ADDRESS INTERNAL_TRIGGER_INTERVAL
#endif

//#define TLUDEBUG
static double TLUFREQUENCY = 48e6;

void TLUEntry::Print(std::ostream & out) const {
  out << m_eventnum
      << ", 0x" << std::hex << m_timestamp << std::dec
      << " (" << (m_timestamp / TLUFREQUENCY) << ")";
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

TLUController::TLUController(const std::string & filename, ErrorHandler err) :
  m_filename(filename != "" ? filename : "TLU_Toplevel.bit"),
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
  m_oldbuf(new unsigned long long [BUFFER_DEPTH])
{
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
  SetLEDs(0);
  InhibitTriggers(true);
}

void TLUController::ResetTriggerCounter() {
  WriteRegister(RESET_REGISTER_ADDRESS, 1<<TRIGGER_COUNTER_RESET_BIT);
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

int TLUController::ReadFirmwareID() {
  return ReadRegister(FIRMWARE_ID_ADDRESS);
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
    WriteRegister(INITIATE_READOUT_ADDRESS, 0xFF);
    for (int i = 0; i < 10; ++i) {
      mSleep(1);
      m_dmastatus = ReadRegister(DMA_STATUS_ADDRESS);
      if (!m_dmastatus) break;
    }
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
  out << "FSM Status:      " << m_fsmstatus << std::endl
      << "Veto Status:     " << m_vetostatus << std::endl
      << "DMA Status:      " << m_dmastatus << std::endl
      << "Trigger counter: " << m_triggernum << std::endl
      << "Timestamp:       0x" << std::hex
      << m_timestamp << std::dec
      << " (" << (m_timestamp / TLUFREQUENCY) << ")" << std::endl
      << "Buffer level:    " << m_buffer.size() << std::endl;
  
  for (size_t i = 0; i < m_buffer.size(); ++i) {
    out << "  " << m_buffer[i] << std::endl;
  }
}

unsigned TLUController::getFirmwareID() const {
  return ReadRegister(FIRMWARE_ID_ADDRESS);
}

void TLUController::SetLEDs(unsigned int val) {
  WriteRegister(DUT_LED_ADDRESS, m_ledstatus = val);
}

unsigned TLUController::GetLEDs() const {
  return m_ledstatus;
}
