#include "VMEInterface.hh"
#include "TSI148Interface.hh"
#include "OldVMEInterface.hh"
#include "eudaq/Exception.hh"
#include <cstdlib>

VMEInterface::VMEInterface(unsigned long base, unsigned long size,
                           int awidth, int dwidth, int proto, int sstrate)
  : m_base(base), m_size(size), m_awidth(awidth), m_dwidth(dwidth),
    m_proto(proto), m_sstrate(sstrate)
{
}

void VMEInterface::SetWindow(unsigned long base, unsigned long size) {
  m_base = base;
  if (size) m_size = size;
  SetWindowParameters();
}

void VMEInterface::SetWindow(unsigned long base, unsigned long size, int awidth, int dwidth,
                         int proto, int sstrate) {
  m_base = base;
  if (size) m_size = size;
  if (awidth) m_awidth = awidth;
  if (dwidth) m_dwidth = dwidth;
  if (proto) m_proto = proto;
  if (sstrate) m_sstrate = sstrate;
  SetWindowParameters();
}

void VMEInterface::SysReset() {
  EUDAQ_THROW("SysReset not implemented in this driver");
}

VMEptr VMEFactory::Create(unsigned long base, unsigned long size, int awidth, int dwidth,
                          int proto, int sstrate) {
  char * oldstr = std::getenv("EUDAQ_OLDVME");
  bool oldvme = oldstr && oldstr == std::string("1");
  if (proto == VMEInterface::PSCT) {
    return oldvme ? VMEptr(new OldVMEInterface(base, size, awidth, dwidth, proto))
                  : VMEptr(new TSI148SingleInterface(base, size, awidth, dwidth, proto));
  }
  return oldvme ? VMEptr(new OldDMAInterface(base, size, awidth, dwidth, proto, sstrate))
                : VMEptr(new TSI148DMAInterface(base, size, awidth, dwidth, proto, sstrate));
}
