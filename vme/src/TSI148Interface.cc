#include "TSI148Interface.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Exception.hh"
#include <iostream>

extern "C" {
#include "vmedrv.h"
}

#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

using eudaq::to_string;

namespace {
  static const int MAX_CHANNEL = 7;

  static void CheckDriver() {
    vmeInfoCfg_t VmeInfo;
    memset(&VmeInfo, 0, sizeof VmeInfo);
    int fd = open("/dev/vme_ctl",O_RDONLY);
    //std::cout << "DEBUG: cme_ctl fd = " << fd << std::endl;
    int status = -1;
    if (fd != -1) status = ioctl(fd, VME_IOCTL_GET_SLOT_VME_INFO, &VmeInfo);
    if (status == -1) {
      if (fd != -1) close(fd);
      EUDAQ_THROW("Unable to talk to VME driver. "
                  "Check that the kernel module is loaded "
                  "and the device files exist "
                  "with the correct properties.");
    }
    close(fd);
  }

  static dataWidth_t ToDataWidth(int width) {
    return static_cast<dataWidth_t>(width);
  }
  static addressMode_t ToAddrSpace(int width) {
    return width == VMEInterface::A16 ? VME_A16 :
           width == VMEInterface::A24 ? VME_A24 :
           width == VMEInterface::A32 ? VME_A32 :
           width == VMEInterface::A64 ? VME_A64 :
      VME_CRCSR;
  }
  static int ToProto(int proto) {
    return proto == VMEInterface::PSCT ? VME_SCT :
           proto == VMEInterface::PBLT ? VME_BLT :
           proto == VMEInterface::PMBLT ? VME_BLT :
           proto == VMEInterface::P2eVME ? VME_BLT :
           proto == VMEInterface::P2eSST ? VME_BLT :
           proto == VMEInterface::P2eSSTB ? VME_BLT :
      0;
  }
  static vme2esstRate_t ToSSTRate(int rate) {
    return static_cast<vme2esstRate_t>(rate);
  }
}

TSI148SingleInterface::TSI148SingleInterface(unsigned long base, unsigned long size,
                                             int awidth, int dwidth,
                                             int proto, int sstrate)
  : VMEInterface(base, size, awidth, dwidth, proto, sstrate),
    m_chan(-1), m_fd(-1)
{
  CheckDriver();
  OpenDevice();
  if (m_fd == -1) {
    EUDAQ_THROW("Unable to open VME device file,"
                "maybe too many VME channels are open.");
  }
  //std::cout << "DEBUG: VME channel = " << m_chan << std::endl;
  SetWindowParameters();
}

void TSI148SingleInterface::OpenDevice() {
  std::string devfile = "/dev/vme_dma0";
  // start from high end, so that old programs using vme_m0 still function
  for (m_chan = MAX_CHANNEL; m_chan >= 0; --m_chan) {
    devfile = "/dev/vme_m" + to_string(m_chan);
    m_fd = open(devfile.c_str(), O_RDWR);
    //std::cout << "DEBUG: VME trying channel " << m_chan << ", fd = " << m_fd << std::endl;
    if (m_fd != -1) break;
  }
}

void TSI148SingleInterface::SetWindowParameters() {
  //std::cout << "DEBUG: SetWindowParameters" << std::endl;
  vmeOutWindowCfg wincfg;
  memset(&wincfg, 0, sizeof wincfg);
  wincfg.windowNbr = m_chan;
  wincfg.windowEnable = 1;
  wincfg.windowSizeL = m_size;
  wincfg.xlatedAddrL = m_base;
  wincfg.xferRate2esst = ToSSTRate(m_sstrate);
  wincfg.addrSpace = ToAddrSpace(m_awidth);
  wincfg.maxDataWidth = ToDataWidth(m_dwidth);
  wincfg.xferProtocol = ToProto(m_proto);
  wincfg.userAccessType = VME_USER;
  wincfg.dataAccessType = VME_DATA;
  //std::cout << "DEBUG: setting window parameters" << std::endl;
  if (ioctl(m_fd, VME_IOCTL_SET_OUTBOUND, &wincfg) < 0) {
    EUDAQ_THROW("Error setting up VME window (code = " + to_string(errno) + ")");
  }
}

void TSI148SingleInterface::DoRead(unsigned long offset, unsigned char * data, size_t size) {
  //std::cout << "DEBUG: seeking to offset " << offset << std::endl;
  lseek(m_fd, offset, SEEK_SET);
  //std::cout << "DEBUG: reading..." << std::endl;
  size_t n = read(m_fd, data, size);
  //std::cout << "DEBUG: OK." << std::endl;
  if (n != size) {
    EUDAQ_THROW("Error: VME read failed at offset " + to_string(offset) +
                " (code = " + to_string(errno) + ")");
  }
}

void TSI148SingleInterface::DoWrite(unsigned long offset, const unsigned char * data, size_t size) {
  lseek(m_fd, offset, SEEK_SET);
  size_t n = write(m_fd, data, size);
  if (n != size) {
    EUDAQ_THROW("Error: VME write failed at offset " + to_string(offset) +
                " (code = " + to_string(errno) + ")");
  }
}

#define VMESET(var, val) read.var = val; write.var = val
struct TSI148DMAInterface::DMAparams {
  DMAparams() {
    memset(&read, 0, sizeof (vmeDmaPacket));
    memset(&write, 0, sizeof (vmeDmaPacket));

    VMESET(maxPciBlockSize, 4096);
    VMESET(maxVmeBlockSize, 2048);
    VMESET(srcVmeAttr.userAccessType, VME_USER);
    VMESET(srcVmeAttr.dataAccessType, VME_DATA);
    VMESET(dstVmeAttr.userAccessType, VME_USER);
    VMESET(dstVmeAttr.dataAccessType, VME_DATA);

    read.srcBus = VME_DMA_VME;
    read.dstBus = VME_DMA_USER;
    write.srcBus = VME_DMA_USER;
    write.dstBus = VME_DMA_VME;
  }
  vmeDmaPacket read, write;
};
#undef VMESET

TSI148DMAInterface::TSI148DMAInterface(unsigned long base, unsigned long size,
                                       int awidth, int dwidth,
                                       int proto, int sstrate)
  : VMEInterface(base, size, awidth, dwidth, proto, sstrate)
{
  CheckDriver();
  m_fd = open("/dev/vme_dma0", O_RDWR);
  if (m_fd == -1) {
    EUDAQ_THROW("Unable to open VME DMA device file.");
  }
  SetWindowParameters();
}

#define VMESET(var, val) m_params->read.srcVmeAttr.var = val; m_params->write.srcVmeAttr.var = val; \
                         m_params->read.dstVmeAttr.var = val; m_params->write.dstVmeAttr.var = val
void TSI148DMAInterface::SetWindowParameters() {
  VMESET(maxDataWidth, ToDataWidth(m_dwidth));
  VMESET(addrSpace, ToAddrSpace(m_awidth));
  VMESET(xferProtocol, ToProto(m_proto));
  VMESET(xferRate2esst, ToSSTRate(m_sstrate));
}
#undef VMESET

void TSI148DMAInterface::DoRead(unsigned long offset, unsigned char * data, size_t size) {
  m_params->read.srcAddr = m_base + offset;
  m_params->read.dstAddr = (unsigned)data;
  m_params->read.byteCount = size;
  int status = ioctl(m_fd, VME_IOCTL_START_DMA, &m_params->read);
  if (status < 0) {
    EUDAQ_THROW("Error while DMA reading VME (code = " + to_string(errno) + ")");
  }
  //m_params->read.vmeDmaElapsedTime
  //m_params->read.vmeDmaStatus
}

void TSI148DMAInterface::DoWrite(unsigned long offset, const unsigned char * data, size_t size) {
  m_params->write.srcAddr = (unsigned)data;
  m_params->write.dstAddr = m_base + offset;
  m_params->write.byteCount = size;
  int status = ioctl(m_fd, VME_IOCTL_START_DMA, &m_params->write);
  if (status < 0) {
    EUDAQ_THROW("Error while DMA reading VME (code = " + to_string(errno) + ")");
  }
}
