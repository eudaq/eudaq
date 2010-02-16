#include "eudaq/OptionParser.hh"
#include "eudaq/Exception.hh"
#include "eudaq/Utils.hh"
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

using eudaq::uchar_cast;

int main(int /*argc*/, const char ** argv) {
  eudaq::OptionParser op("EUDAQ EUDRB Resetter", "1.0", "Reset an EUDRB board");
  eudaq::OptionFlag sys(op, "s", "sysreset",
                        "Perform a SYSRESET on the crate");
  eudaq::OptionFlag loc(op, "l", "localreset",
                        "Perform a LOCALRESET");
  eudaq::OptionFlag dbg(op, "d", "debug",
                        "Print debug information");
  try {
    op.Parse(argv);
    if (!sys.IsSet() && !loc.IsSet() && !dbg.IsSet()) throw eudaq::OptionException("Must specify either SYS- or LOCAL-reset");
    int fd = open("/dev/vme_regs", O_RDWR);
    if (fd == -1) {
      EUDAQ_THROW("Unable to open VME registers device.");
    }
    lseek(fd, 0x238, SEEK_SET);
    unsigned long data;
    if (read(fd, uchar_cast(&data), sizeof data) < 0) {
      EUDAQ_THROW("Unable to read from TSI148 registers");
    }
    if (dbg.IsSet()) std::cout << "DEBUG: VCTRL register read " << eudaq::hexdec(data) << std::endl;
    if (sys.IsSet()) data |= 1 << 17;
    if (loc.IsSet()) data |= 1 << 16;
    lseek(fd, 0x238, SEEK_SET);
    if (dbg.IsSet()) std::cout << "DEBUG: VCTRL register writing " << eudaq::hexdec(data) << std::endl;
    int written = write(fd, uchar_cast(&data), sizeof data);
    close(fd);
    if (written < 0) {
      EUDAQ_THROW("Unable to write to TSI148 registers");
    }
    std::cout << "Done." << std::endl;
  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}
