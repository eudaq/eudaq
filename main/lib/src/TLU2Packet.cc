#include <ostream>
#include <iostream>
#include <time.h>

#include "eudaq/BufferSerializer.hh"
#include "eudaq/TLU2Packet.hh"

using std::cout;

namespace eudaq {
  EUDAQ_DEFINE_PACKET(TLU2Packet, str2type("-TLU2-"));
}
