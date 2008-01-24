#include "VMEInterface.hh"
#include "TSI148Interface.hh"

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

VMEptr VMEFactory::Create(unsigned long base, unsigned long size, int awidth, int dwidth,
                          int proto, int sstrate) {
  if (proto == VMEInterface::PSCT) {
    return VMEptr(new TSI148SingleInterface(base, size, awidth, dwidth, proto));
  }
  return VMEptr(new TSI148DMAInterface(base, size, awidth, dwidth, proto, sstrate));
}
