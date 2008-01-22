#include "VMEInterface.hh"

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

std::vector<unsigned long> & VMEInterface::Read(unsigned long offset,
                                                std::vector<unsigned long> & data) {
  for (size_t i = 0; i < data.size(); ++i) {
    data[i] = Read(offset + i*m_dwidth/8);
  }
  return data;
}

void VMEInterface::Write(unsigned long offset, const std::vector<unsigned long> & data) {
  for (size_t i = 0; i < data.size(); ++i) {
    Write(offset + i*m_dwidth/8, data[i]);
  }
}
