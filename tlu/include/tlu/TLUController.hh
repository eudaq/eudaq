#ifndef H_TLUController_hh
#define H_TLUController_hh

#include "ZestSC1.h"
#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>
#include "eudaq/Utils.hh"

namespace tlu {

  static const int TLU_TRIGGER_INPUTS = 4;
  double Timestamp2Seconds(unsigned long long t);

  class TLUException : public std::runtime_error {
  public:
    TLUException(const std::string & msg, int status = 0, int tries = 0)
      : std::runtime_error(make_msg(msg, status, tries).c_str()),
        m_status(status),
        m_tries(tries)
    {}
    int GetStatus() const { return m_status; }
    int GetTries() const { return m_tries; }
  private:
    static std::string make_msg(const std::string & msg, int status, int tries);
    int m_status, m_tries;
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

  struct TLUAddresses;

  class TLUController {
  public:
    //typedef void (*ErrorHandler)(const char * function,
    //                             ZESTSC1_HANDLE,
    //                             ZESTSC1_STATUS,
    //                             const char * msg);
    enum ErrorHandler { // What to do if a usb access returns an error
      ERR_ABORT,        // Abort the program (used for debugging)
      ERR_THROW,        // Throw a TLUException
      ERR_RETRY1,       // Retry once before throwing
      ERR_RETRY2        // Retry twice
    };

    TLUController(int errormech = ERR_RETRY1);
    ~TLUController();

    void SetVersion(unsigned version); // default (0) = auto detect from serial number
    void SetFirmware(const std::string & filename); // can be just version number
    void SetDUTMask(unsigned char mask);
    void SetVetoMask(unsigned char mask);
    void SetAndMask(unsigned char mask);
    void SetOrMask(unsigned char mask);
    unsigned char GetVetoMask() const;
    unsigned char GetAndMask() const;
    unsigned char GetOrMask() const;

    void SetTriggerInterval(unsigned millis);

    void Configure();
    void Update();
    void Start();
    void Stop();
    void ResetTriggerCounter();
    void ResetScalers();
    void ResetUSB();

    size_t NumEntries() const { return m_buffer.size(); }
    TLUEntry GetEntry(size_t i) const { return m_buffer[i]; }
    unsigned GetTriggerNum() const { return m_triggernum; }
    unsigned long long GetTimestamp() const { return m_timestamp; }

    unsigned char GetTriggerStatus() const ;

    bool InhibitTriggers(bool inhibit = true); // returns previous value

    void Print(std::ostream & out = std::cout) const;

    unsigned GetVersion() const;
    std::string GetFirmware() const;
    unsigned GetFirmwareID() const;
    unsigned GetSerialNumber() const;
    unsigned GetLibraryID(unsigned ver = 0) const;
    void SetLEDs(int left, int right);
    //unsigned GetLEDs() const;
    void SetLemoLEDs(unsigned);
    //void SetLemoADCVoltage(unsigned mask, double voltage);

    unsigned GetScaler(unsigned) const;
    unsigned GetParticles() const;
  private:
    void OpenTLU();
    void LoadFirmware();
    void Initialize();

    void WriteRegister(unsigned long offset, unsigned char val);
    unsigned char ReadRegister8(unsigned long offset) const;
    unsigned short ReadRegister16(unsigned long offset) const;
    unsigned long ReadRegister32(unsigned long offset) const;
    unsigned long long ReadRegister64(unsigned long offset) const;
    unsigned long long * ReadBlock(unsigned entries);
    unsigned char ReadRegisterRaw(unsigned long offset) const;

    void WriteI2C(unsigned hw, unsigned bus, unsigned device, unsigned data);
    void WriteI2C16(unsigned device, unsigned command, unsigned data);
    void WriteI2Cbyte(unsigned data);
    bool WriteI2Clines(bool scl, bool sda);

    std::string m_filename;
    ZESTSC1_HANDLE m_handle;
    unsigned char m_mask, m_vmask, m_amask, m_omask;
    unsigned m_triggerint, m_serial;
    bool m_inhibit;

    unsigned m_vetostatus, m_fsmstatus, m_ledstatus;
    unsigned m_triggernum;
    unsigned long long m_timestamp;
    std::vector<TLUEntry> m_buffer;
    unsigned long long * m_oldbuf;
    unsigned m_scalers[TLU_TRIGGER_INPUTS];
    unsigned m_particles;
    mutable unsigned long long m_lasttime;
    int m_errorhandler;
    unsigned m_version;
    TLUAddresses * m_addr;
  };

  inline std::ostream & operator << (std::ostream & o, const TLUController & t) {
    t.Print(o);
    return o;
  }

  inline std::ostream & operator << (std::ostream & o, const TLUEntry & t) {
    t.Print(o);
    return o;
  }

}

#endif
