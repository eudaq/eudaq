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
}

TSI148Interface::TSI148Interface(unsigned long base, unsigned long size, int awidth, int dwidth,
                                 int proto, int sstrate)
  : VMEInterface(base, size, awidth, dwidth, proto, sstrate),
    m_chan(-1), m_fd(-1)
{
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
  OpenDevice();
  if (m_fd == -1) {
    EUDAQ_THROW("Unable to open VME device file,"
                "maybe too many VME channels are open.");
  }
  //std::cout << "DEBUG: VME channel = " << m_chan << std::endl;
  SetWindowParameters();
}

void TSI148Interface::OpenDevice() {
  std::string devfile = "/dev/vme_dma0";
  if (IsDMA()) {
    m_chan = -1;
    m_fd = open(devfile.c_str(), O_RDWR);
  } else {
    // start from vme_m7, so that old programs using vme_m0 still function
    for (m_chan = MAX_CHANNEL; m_chan >= 0; --m_chan) {
      devfile = "/dev/vme_m" + to_string(m_chan);
      m_fd = open(devfile.c_str(), O_RDWR);
      //std::cout << "DEBUG: VME trying channel " << m_chan << ", fd = " << m_fd << std::endl;
      if (m_fd != -1) break;
    }
  }
}

bool TSI148Interface::IsDMA() {
  return m_proto != PSCT;
}

void TSI148Interface::SetWindowParameters() {
  //std::cout << "DEBUG: SetWindowParameters" << std::endl;
  if (m_fd != -1) {
    if ((IsDMA() && m_chan != -1) || (!IsDMA() && m_chan == -1)) {
      close(m_fd);
      m_fd = -1;
    }
  }
  if (m_fd == -1) {
    OpenDevice();
    if (m_fd == -1) {
      EUDAQ_THROW("Unable to open VME device file,"
                  "maybe too many VME channels are open.");
    }
  }
  if (!IsDMA()) {
    vmeOutWindowCfg wincfg;
    memset(&wincfg, 0, sizeof wincfg);
    wincfg.windowNbr = m_chan;
    wincfg.windowEnable = 1;
    wincfg.windowSizeL = m_size;
    wincfg.xlatedAddrL = m_base;
    wincfg.xferRate2esst = VME_SSTNONE; // TODO: get from m_rate
    wincfg.addrSpace = m_awidth == A16 ? VME_A16 : m_awidth == A24 ? VME_A24 :
      m_awidth == A32 ? VME_A32 : m_awidth == A64 ? VME_A64 : VME_CRCSR;
    wincfg.maxDataWidth = (dataWidth_t)m_dwidth;
    wincfg.xferProtocol = VME_SCT; // Other protocols use DMA
    wincfg.userAccessType = VME_USER;
    wincfg.dataAccessType = VME_DATA;
    //std::cout << "DEBUG: setting window parameters" << std::endl;
    if (ioctl(m_fd, VME_IOCTL_SET_OUTBOUND, &wincfg) < 0) {
      EUDAQ_THROW("Error setting up VME window (code = " + to_string(errno) + ")");
    }
  }
}

unsigned long TSI148Interface::Read(unsigned long offset) {
  // TODO: make this work with D16 etc.
  unsigned long result = 0;
  //std::cout << "DEBUG: seeking to offset " << offset << std::endl;
  lseek(m_fd, offset, SEEK_SET);
  //std::cout << "DEBUG: reading..." << std::endl;
  int n = read(m_fd, &result, sizeof result);
  //std::cout << "DEBUG: OK." << std::endl;
  if (n != sizeof result) {
    EUDAQ_THROW("Error: VME read failed at offset " + to_string(offset) +
                " (code = " + to_string(errno) + ")");
  }
  return result;
}

void TSI148Interface::Write(unsigned long offset, unsigned long data) {
  lseek(m_fd, offset, SEEK_SET);
  int n = write(m_fd, &data, sizeof data);
  if (n != sizeof data) {
    EUDAQ_THROW("Error: VME read failed at offset " + to_string(offset) +
                " (code = " + to_string(errno) + ")");
  }
}
