#ifndef H_USBTracer_hh
#define H_USBTracer_hh

#if !defined(NDEBUG) && !defined(USB_TRACE)
#define USB_TRACE 2
#endif

#include <string>
#include "eudaq/Exception.hh"

namespace tlu {

#if USB_TRACE

  // for end users to control tracing
  void setusbtracefile(const std::string &);
  void setusbtracelevel(int newlevel);

  // the reset is for programs to do tracing
  int getusbtracelevel();
  void usbflushtracefile();
  void dousbtrace(const std::string & mode, unsigned long addr, const std::string & data, int status = -1);

#else

  inline void setusbtracefile(const std::string &) {
    EUDAQ_THROW("USB tracing not enabled in this build");
  }
  inline void setusbtracelevel(int) {
    EUDAQ_THROW("USB tracing not enabled in this build");
  }
  inline void usbflushtracefile() {}
  inline void dousbtrace(const std::string &, unsigned long, const std::string &, int = -1) {}
  inline int getusbtracelevel() { return 0; }

#endif

  template <typename T>
  inline void usbtrace(const std::string & mode, unsigned long addr, T data, int status) {
    if (status != 0 || getusbtracelevel() > 1) {
      dousbtrace(mode, addr, eudaq::to_string(eudaq::hexdec(data)), status);
    }
  }

  template <typename T>
  inline void usbtrace(const std::string & mode, unsigned long addr, T * data, int size, int status) {
    if (status != 0 || getusbtracelevel() > 1) {
      dousbtrace(mode, addr, "[" + eudaq::to_string(eudaq::hexdec(size)) + "]", status);
      if (getusbtracelevel() > 2) {
        for (int i = 0; i < size; ++i) {
          dousbtrace("  ", addr + i*sizeof(T), eudaq::to_string(eudaq::hexdec(data[i])));
        }
      }
    }
  }

}

#endif // H_USBTracer_hh
