#include <vector>
#include <iostream>
#include <usb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cstring>
#include <errno.h>

#define TLU_VENDOR_ID 0x165d
#define TLU_PRODUCT_ID 0x0001

int main() {
  int ret = 0;
  usb_init();
  usb_find_busses();
  usb_find_devices();
  std::vector<struct usb_device *> tlus;
  for (usb_bus * bus = usb_get_busses(); bus; bus = bus->next) {
    for (struct usb_device * dev = bus->devices; dev; dev = dev->next) {
      if (dev->descriptor.idVendor == TLU_VENDOR_ID && dev->descriptor.idProduct == TLU_PRODUCT_ID) {
        tlus.push_back(dev);
      }
    }
  }
  std::cout << "Found " << tlus.size() << " tlu(s)" << std::endl;
  for (size_t i = 0; i < tlus.size(); ++i) {
    std::string fname = std::string("/proc/bus/usb/") + tlus[i]->bus->dirname + "/" + tlus[i]->filename;
    std::string result = "Fixed";
    struct stat st;
    if (stat(fname.c_str(), &st) == 0 && (st.st_mode & 0x66) == 0x66) {
      result = "OK";
    } else if (chmod(fname.c_str(), 0x66) < 0) {
      result = std::string("Error ") + strerror(errno);
      ret = 1;
    }
    std::cout << "TLU " << (i+1) << " at " << fname << ": " << result << std::endl;
  }
  return ret;
}
