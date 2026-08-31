#pragma once

#include "HidraTrackerEvent.hh"

#include <cstdint>
#include <vector>

namespace hidra {

// Decoder for the HidraTrackerProducer block payload.
//
// Mirrors HidraXdcDecoder / HidraFersDecoder: it takes the byte payload of a
// tracker sub-event and fills a HidraTrackerEvent. The payload is a flat array
// of `double` values (coordinates in cm), two per plane (x then y), in the
// producer's native byte order (no endianness conversion — same as the FERS
// payload), exactly the `coordinates` block built in HidraTrackerProducer::SendRow.
//
// The number of planes is inferred from the payload length, so the same
// decoder keeps working if the tracker format grows or shrinks the number of
// planes (it only requires a whole number of (x, y) pairs).
class HidraTrackerDecoder {
public:
  HidraTrackerDecoder();
  void decode(const std::vector<uint8_t>& payload, HidraTrackerEvent& event, std::uint64_t trigger_n) const;

  // Values per plane in the payload: one x and one y.
  static constexpr int kValuesPerPlane = 2;
};

} // namespace hidra
