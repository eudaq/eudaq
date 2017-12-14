#include "tlu/TLUController.hh"
#ifdef WIN32

#include "lusb0_usb.h"
#else
#include <usb.h>

#endif
#include "ZestSC1.h"
#include "Local.h"

namespace tlu {

  int do_usb_reset(ZESTSC1_HANDLE Handle) {
    ZESTSC1_CHECK_HANDLE("ResetUSB", Handle);
    usb_reset(Struct->DeviceHandle);
    return 0;
  }
}
