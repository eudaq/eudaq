#ifndef HIDRA_EVENT_SERIALIZER_HH
#define HIDRA_EVENT_SERIALIZER_HH

#include "eudaq/Event.hh"
#include <cstdint>
#include <string>
#include <vector>

namespace hidra {

class EventSerializer {
public:
  static std::vector<std::uint8_t> Serialize(const eudaq::Event& event);

  static int WriteToFile(const eudaq::Event& event, const std::string& filename);
  static int WriteToStream(const eudaq::Event& event, std::ostream& out);
};

} // namespace hidra

#endif
