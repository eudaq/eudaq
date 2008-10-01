#include "VMEInterface.hh"

class TSI148SingleInterface : public VMEInterface {
public:
  TSI148SingleInterface(unsigned long base, unsigned long size, int awidth = A32, int dwidth = D32,
                        int proto = PSCT, int sstrate = SSTNONE);
  virtual void SysReset();
  ~TSI148SingleInterface();
private:
  virtual void SetWindowParameters();
  virtual void DoRead(unsigned long offset, unsigned char * data, size_t size);
  virtual void DoWrite(unsigned long offset, const unsigned char * data, size_t size);
  void OpenDevice();
  int m_chan, m_fd;
};

class TSI148DMAInterface : public VMEInterface {
public:
  TSI148DMAInterface(unsigned long base, unsigned long size, int awidth = A32, int dwidth = D32,
                     int proto = PMBLT, int sstrate = SSTNONE);
  virtual void SysReset();
  ~TSI148DMAInterface();
private:
  virtual void SetWindowParameters();
  virtual void DoRead(unsigned long offset, unsigned char * data, size_t size);
  virtual void DoWrite(unsigned long offset, const unsigned char * data, size_t size);
  void OpenDevice();
  int m_fd;
  class DMAparams;
  DMAparams * m_params;
};

