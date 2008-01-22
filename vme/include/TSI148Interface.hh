#include "VMEInterface.hh"

class TSI148Interface : public VMEInterface {
public:
  TSI148Interface(unsigned long base, unsigned long size, int awidth = A32, int dwidth = D32,
                  int proto = PSCT, int sstrate = SSTNONE);

  virtual unsigned long Read(unsigned long offset);
  virtual void Write(unsigned long offset, unsigned long data);
private:
  bool IsDMA();
  void OpenDevice();
  virtual void SetWindowParameters();
  int m_chan, m_fd;
};
