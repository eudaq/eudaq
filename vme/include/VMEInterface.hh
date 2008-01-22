#include <vector>

class VMEInterface {
public:
  enum AWIDTH { ANONE, A16 = 16, A24 = 24, A32 = 32, A64 = 64 };
  enum DWIDTH { DNONE, D16 = 16, D32 = 32, D64 = 64 };
  enum PROTO  { PNONE, PSCT, PBLT, PMBLT, P2eVME, P2eSST, P2eSSTB };
  enum SSTRATE { SSTNONE, SST160, SST267, SST320 };

  VMEInterface(unsigned long base, unsigned long size, int awidth = A32, int dwidth = D32,
               int proto = PSCT, int sstrate = SSTNONE);

  virtual void SetWindow(unsigned long base, unsigned long size = 0 /*0=unchanged*/);
  virtual void SetWindow(unsigned long base, unsigned long size, int awidth, int dwidth,
                         int proto = PSCT, int sstrate = SSTNONE);

  virtual unsigned long Read(unsigned long offset) = 0;
  virtual std::vector<unsigned long> & Read(unsigned long offset,
                                            std::vector<unsigned long> & data);

  virtual void Write(unsigned long offset, unsigned long data) = 0;
  virtual void Write(unsigned long offset, const std::vector<unsigned long> & data);

  virtual ~VMEInterface() {}
protected:
  virtual void SetWindowParameters() = 0;
  unsigned long m_base, m_size;
  int m_awidth;
  int m_dwidth;
  int m_proto;
  int m_sstrate;
};
