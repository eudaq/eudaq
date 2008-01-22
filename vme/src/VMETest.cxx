#include "TSI148Interface.hh"
#include "eudaq/Utils.hh"

int main() {
  TSI148Interface vme(0x38000000, 0x01000000);
  TSI148Interface vme2(0x28000000, 0x01000000);
  eudaq::mSleep(5000);
  return 0;
}
