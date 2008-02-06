#include "eudaq/Exception.hh"
#include "usb.h"
#include "ZestSC1.h"
#include "../linux/Local.h"

// Very nasty hack! Don't do this at home kids.
#define private public
#include "tlu/TLUController.hh"
#undef private

int main() {
  tlu::TLUController tlu;
  
  ZESTSC1_HANDLE Handle = tlu.m_handle;
  ZESTSC1_CHECK_HANDLE("main", Handle);

  usb_reset(Struct->DeviceHandle);

  return 0;
}
