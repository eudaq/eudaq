#ifndef EUDAQ_INCLUDED_VMEInterface_hh
#define EUDAQ_INCLUDED_VMEInterface_hh

#include "eudaq/counted_ptr.hh"
#include "eudaq/Utils.hh"
#include <vector>

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
    return data;
  }
  unsigned long Read(unsigned long offset) {
    unsigned long data = 0;
    return Read(offset, data);
  }
  template <typename T>
  std::vector<T> & Read(unsigned long offset,
                        std::vector<T> & data) {
    DoRead(offset, eudaq::uchar_cast(data), data.size() * sizeof (T));
    return data;
  }
  template <typename T> void Write(unsigned long offset, T data) {
    DoWrite(offset, eudaq::constuchar_cast(&data), sizeof data);
  }
  template <typename T>
  void Write(unsigned long offset, const std::vector<T> & data) {
    DoWrite(offset, eudaq::constuchar_cast(data), data.size() * sizeof (T));
  }

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
  static VMEptr Create(unsigned long base, unsigned long size,
                       int awidth = VMEInterface::A32, int dwidth = VMEInterface::D32,
                       int proto = VMEInterface::PSCT, int sstrate = VMEInterface::SSTNONE);
};

#endif // EUDAQ_INCLUDED_VMEInterface_hh
