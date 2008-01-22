#include "TSI148Interface.hh"

int main() {
  TSI148Interface vme(0x30000000, 0x01000000);
  return 0;
}
