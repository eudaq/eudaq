#include "tlu/USBTracer.hh"

#if USB_TRACE

#include <fstream>
#include "eudaq/Utils.hh"
#include "eudaq/Time.hh"
#include "ZestSC1.h"

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
    void Write(const std::string & mode, unsigned long addr, const std::string & data, int status) {
      eudaq::Time time = eudaq::Time::Current();
      if (addr == m_prev_addr && data == m_prev_data && mode == m_prev_mode && status == m_prev_stat) {
        m_prev_time = time;
        m_repeated++;
      } else {
        FlushRepeated();
        std::ostringstream s;
        s << time.Formatted("%T.%6") << " " << mode
          << " " << eudaq::hexdec(addr) << " " << data;
        if (status > 0) {
          char * msg = "Invalid error code";
          ZestSC1GetErrorMessage(static_cast<ZESTSC1_STATUS>(status), &msg);
          s << " {" << status << ": " << msg << "}";
        }
        s << "\n";
        WriteString(s.str());
        m_prev_time = time;
        m_prev_mode = mode;
        m_prev_data = data;
        m_prev_addr = addr;
        m_prev_stat = status;
      }
    }
    bool IsOpen() { return m_file.is_open(); }
    int GetLevel() const { return m_level; }
    void SetLevel(int level) { m_level = level; }
    void FlushRepeated(bool flushfile = false) {
      if (m_repeated) {
        std::ostringstream s;
        s << m_prev_time.Formatted("%T.%6") << " ## Repeated " << m_repeated << " time(s)\n";
        m_repeated = 0;
        WriteString(s.str());
        s.clear();
      }
      if (flushfile) m_file.flush();
    }
    ~USBTraceFile() {
      FlushRepeated(true);
      Close();
    }
  private:
    USBTraceFile() : m_repeated(0), m_prev_time(0), m_prev_addr(0), m_prev_stat(0), m_level(USB_TRACE) {}
    std::string m_filename;
    std::ofstream m_file;
    unsigned long m_repeated;
    eudaq::Time m_prev_time;
    std::string m_prev_mode, m_prev_data;
    unsigned long m_prev_addr;
    int m_prev_stat;
    int m_level;
  };

  void setusbtracefile(const std::string & filename) {
    USBTraceFile::Instance().Open(filename);
  }

  void setusbtracelevel(int newlevel) {
    USBTraceFile::Instance().SetLevel(newlevel);
  }

  int getusbtracelevel() {
    return USBTraceFile::Instance().GetLevel();
  }

  void usbflushtracefile() {
    USBTraceFile::Instance().FlushRepeated(true);
  }

  void dousbtrace(const std::string & mode, unsigned long addr, const std::string & data, int status) {
    if (!USBTraceFile::Instance().IsOpen()) return;
    USBTraceFile::Instance().Write(mode, addr, data, status);
  }

}

#endif
