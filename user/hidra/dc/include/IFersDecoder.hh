#pragma once

#include "HidraFersEvent.hh"

#include <cstdint>
#include <vector>

namespace hidra {

/**
 * @brief Interface for FERS payload decoders.
 *
 * Two implementations exist:
 *   - HidraFersDecoder: the real/official decoder, parses the CAEN spectroscopy
 *     blocks.
 *   - HidraFersRandomDecoder: a test-only decoder that ignores the payload and
 *     synthesises random data, so the monitor → filler → frontend chain can be
 *     exercised without real FERS data.
 *
 * This abstraction (and the random implementation) exist only for the monitor,
 * which picks one at configure time (FERS_DECODER in the run .conf) and calls
 * decode() once per event. With the real decoder and no FERS sub-event the
 * payload is empty and decode() yields all-sentinel vectors (the filler skips
 * them); the random decoder ignores the payload and always produces data.
 *
 * Other users (e.g. the DataCollector ROOT writer, HidraFersPayloadDecoder) hold
 * a concrete HidraFersDecoder and always use the real decoder; they are
 * unaffected by this interface.
 */
struct IFersDecoder {
  virtual ~IFersDecoder() = default;

  /** Decode the raw FERS @p payload into the per-channel vectors of @p event. */
  virtual void decode(const std::vector<std::uint8_t>& payload, HidraFersEvent& event) const = 0;
};

} // namespace hidra
