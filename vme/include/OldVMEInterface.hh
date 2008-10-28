#include "VMEInterface.hh"

class OldVMEInterface : public VMEInterface {
public:
  OldVMEInterface(unsigned long base, unsigned long size, int awidth = A32, int dwidth = D32,
                  int proto = PSCT, int sstrate = SSTNONE);
  ~OldVMEInterface();
private:
  virtual void SetWindowParameters();
  virtual void DoRead(unsigned long offset, unsigned char * data, size_t size);
  virtual void DoWrite(unsigned long offset, const unsigned char * data, size_t size);
};

class OldDMAInterface : public VMEInterface {
public:
  OldDMAInterface(unsigned long base, unsigned long size, int awidth = A32, int dwidth = D32,
                  int proto = PMBLT, int sstrate = SSTNONE);
  ~OldDMAInterface();
private:
  virtual void SetWindowParameters();
  virtual void DoRead(unsigned long offset, unsigned char * data, size_t size);
  virtual void DoWrite(unsigned long offset, const unsigned char * data, size_t size);
};

