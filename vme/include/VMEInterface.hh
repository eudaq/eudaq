#ifndef EUDAQ_INCLUDED_VMEInterface_hh
#define EUDAQ_INCLUDED_VMEInterface_hh

#include "eudaq/counted_ptr.hh"
#include "eudaq/Utils.hh"
#include <vector>
#include <cstdlib>

#if !defined(NDEBUG) && !defined(VME_TRACE)
#define VME_TRACE 1
#endif

#if VME_TRACE
#include <cstdio>
  static FILE * tracefile() {
    static FILE * fp = 0;
    if (!fp) fp = fopen("vmetrace.txt", "w");
    return fp;
  }
  static void vmetrace(char * mode, int aw, int dw, unsigned long addr, unsigned long data) {
    fprintf(tracefile(), "%s A%d D%d %lx %lx\n", mode, aw, dw, addr, data);
    fflush(tracefile());
  }
#endif

// template <typename T = unsigned long>
// class DMABuffer {
// public:
//   DMABuffer(size_t capacity)
//     : m_size(0), m_capacity(capacity), m_buf((T*)malloc(capacity * sizeof (T)))
//     {
//       std::cout << "DEBUG: buffer = " << eudaq::hexdec((unsigned long)m_buf) << std::endl;
//     }

//   void clear() { m_size = 0; }
//   void resize(size_t size) { m_size = size; /* TODO: throw error if size > capacity */ }
//   size_t size() const { return m_size; }

//   const T & operator [] (size_t i) const { return m_buf[i]; }
//   T & operator [] (size_t i) { return m_buf[i]; }

//   size_t Bytes() const { return m_size * sizeof (T); }
//   const unsigned char * Buffer() const { return eudaq::constuchar_cast(m_buf); }
//   unsigned char * Buffer() { return eudaq::uchar_cast(m_buf); }

//   ~DMABuffer() { free(m_buf); }
// private:
//   size_t m_size, m_capacity;
//   T * m_buf;
// };

class VMEInterface {
public:
  enum AWIDTH { ANONE, A16 = 16, A24 = 24, A32 = 32, A64 = 64 };
  enum DWIDTH { DNONE, D16 = 16, D32 = 32, D64 = 64 };
  enum PROTO  { PNONE, PSCT, PBLT, PMBLT, P2eVME, P2eSST, P2eSSTB };
  enum SSTRATE { SSTNONE, SST160 = 160, SST267 = 267, SST320 = 320 };

  VMEInterface(unsigned long base, unsigned long size, int awidth = A32, int dwidth = D32,
               int proto = PSCT, int sstrate = SSTNONE);

  virtual void SetWindow(unsigned long base, unsigned long size = 0 /*0=unchanged*/);
  virtual void SetWindow(unsigned long base, unsigned long size, int awidth, int dwidth,
                         int proto = PSCT, int sstrate = SSTNONE);

  template <typename T> T Read(unsigned long offset, T def) {
    T data = def;
    DoRead(offset, eudaq::uchar_cast(&data), sizeof data);
#if VME_TRACE
    vmetrace(" R", m_awidth, m_dwidth, offset, data);
#endif
    return data;
  }
  unsigned long Read(unsigned long offset) {
    return Read(offset, 0UL);
  }
//   unsigned long Read(unsigned long offset) {
//     unsigned long data = 0;
//     return Read(offset, data);
//   }
  template <typename T>
  std::vector<T> & Read(unsigned long offset,
                        std::vector<T> & data) {
    DoRead(offset, eudaq::uchar_cast(&data[0]), data.size() * sizeof (T));
#if VME_TRACE
    vmetrace("BR", m_awidth, m_dwidth, offset, data[0]);
    for (size_t i = 1; i < data.size(); ++i) {
      vmetrace("  ", m_awidth, m_dwidth, offset + i*4, data[i]);
    }
#endif
    return data;
  }
  template <typename T> void Write(unsigned long offset, T data) {
    DoWrite(offset, eudaq::constuchar_cast(&data), sizeof data);
#if VME_TRACE
    vmetrace(" W", m_awidth, m_dwidth, offset, data);
#endif
  }
//   template <typename T>
//   void Write(unsigned long offset, const DMABuffer<T> & data) {
//     DoWrite(offset, data.Buffer(), data.Bytes());
// #if VME_TRACE
//     vmetrace("BW", m_awidth, m_dwidth, offset, data[0]);
//     for (size_t i = 1; i < data.size(); ++i) {
//       vmetrace("  ", m_awidth, m_dwidth, offset + i*4, data[i]);
//     }
// #endif
//   }
  template <typename T>
  void Write(unsigned long offset, const std::vector<T> & data) {
    DoWrite(offset, eudaq::constuchar_cast(&data[0]), data.size() * sizeof (T));
#if VME_TRACE
    vmetrace("BR", m_awidth, m_dwidth, offset, data[0]);
    for (size_t i = 1; i < data.size(); ++i) {
      vmetrace("  ", m_awidth, m_dwidth, offset + i*4, data[i]);
    }
#endif
  }

  virtual void SysReset();
  virtual ~VMEInterface() {}
protected:
  virtual void DoRead(unsigned long offset, unsigned char * data, size_t size) = 0;
  virtual void DoWrite(unsigned long offset, const unsigned char * data, size_t size) = 0;
  virtual void SetWindowParameters() = 0;
  unsigned long m_base, m_size;
  int m_awidth;
  int m_dwidth;
  int m_proto;
  int m_sstrate;
};

typedef counted_ptr<VMEInterface> VMEptr;

class VMEFactory {
public:
  static VMEptr Create(unsigned long base, unsigned long size, int awidth, int dwidth,
                       int proto, int sstrate = VMEInterface::SSTNONE);
  static VMEptr Create(unsigned long base, unsigned long size) {
    return Create(base, size, VMEInterface::A32, VMEInterface::D32, VMEInterface::PSCT);
  }
  static VMEptr Create(unsigned long base, unsigned long size, int proto) {
    return Create(base, size, VMEInterface::A32, VMEInterface::D32, proto);
  }
  static VMEptr Create(unsigned long base, unsigned long size, int awidth, int dwidth) {
    return Create(base, size, awidth, dwidth, VMEInterface::PSCT);
  }
};

#endif // EUDAQ_INCLUDED_VMEInterface_hh
