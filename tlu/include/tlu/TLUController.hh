#ifndef H_TLUController_h
#define H_TLUController_h

#include "ZestSC1.h"
#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>

class TLUException : public std::runtime_error {
public:
  TLUException(const std::string & msg) : std::runtime_error(msg.c_str()) {}
};

class TLUEntry {
public:
  TLUEntry(unsigned long long t = 0, unsigned long e = 0)
    : m_timestamp(t), m_eventnum(e) {}
  unsigned long long Timestamp() const { return m_timestamp; }
  unsigned long Eventnum() const { return m_eventnum; }
  void Print(std::ostream & out = std::cout) const;
private:
  unsigned long long m_timestamp;
  unsigned long m_eventnum;
};

class TLUController {
public:
  typedef void (*ErrorHandler)(const char * function,
                               ZESTSC1_HANDLE,
                               ZESTSC1_STATUS,
                               const char * msg);

  TLUController(const std::string & filename = "", ErrorHandler f = 0);
  ~TLUController();

  //void SetFilename(const std::string & filename) { m_filename = filename; }
  void SetDUTMask(unsigned char mask);
  void SetVetoMask(unsigned char mask);
  void SetAndMask(unsigned char mask);
  void SetOrMask(unsigned char mask);
  unsigned char GetVetoMask() const;
  unsigned char GetAndMask() const;
  unsigned char GetOrMask() const;


  int  ReadFirmwareID();
  void SetTriggerInterval(unsigned millis);

  //void Configure();
  void Update();
  void Start();
  void Stop();
  void ResetTriggerCounter();

  size_t NumEntries() const { return m_buffer.size(); }
  TLUEntry GetEntry(size_t i) const { return m_buffer[i]; }
  unsigned GetTriggerNum() const { return m_triggernum; }
  unsigned long long GetTimestamp() const { return m_timestamp; }

  unsigned char GetTriggerStatus() const ;

  void InhibitTriggers(bool inhibit = true);

  void Print(std::ostream & out = std::cout) const;

  unsigned getFirmwareID() const;
  void SetLEDs(unsigned);
  unsigned GetLEDs() const;
private:
  void Initialize();
  void WriteRegister(unsigned long offset, unsigned char val);
  unsigned char ReadRegister(unsigned long offset) const;
  unsigned short ReadRegister16(unsigned long offset) const;
  unsigned long ReadRegister32(unsigned long offset) const;
  unsigned long long ReadRegister64(unsigned long offset) const;
  unsigned long long * ReadBlock(unsigned entries);

  std::string m_filename;
  ZESTSC1_HANDLE m_handle;
  unsigned char m_mask, m_vmask, m_amask, m_omask;
  unsigned m_triggerint;
  bool m_inhibit;

  unsigned m_vetostatus, m_fsmstatus, m_dmastatus, m_ledstatus;
  unsigned m_triggernum;
  unsigned long long m_timestamp;
  std::vector<TLUEntry> m_buffer;
  unsigned long long * m_oldbuf;
};

inline std::ostream & operator << (std::ostream & o, const TLUController & t) {
  t.Print(o);
  return o;
}

inline std::ostream & operator << (std::ostream & o, const TLUEntry & t) {
  t.Print(o);
  return o;
}

#endif
