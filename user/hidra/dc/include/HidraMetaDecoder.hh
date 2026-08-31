#pragma once

#include "HidraEventMeta.hh"

#include <eudaq/Event.hh>

namespace hidra {

/**
 * @brief Extracts per-event metadata (trigger mask, spill, timestamps, …) from an EUDAQ event.
 *
 * Unlike HidraXdcDecoder / HidraFersDecoder, which decode the binary detector
 * payload, this decoder reads information carried by the EUDAQ event itself:
 * event-level fields (run/event/trigger number, timestamps) from the merged
 * event, and tags set by the producers. triggerMask and spillNumber are set by
 * the producer on the XDC sub-event (detID 1 real / 6 dry), so they are read from
 * there; see DataFormat.md.
 */
class HidraMetaDecoder {
public:
  /** Fill @p meta from the merged EUDAQ event @p event and its sub-events. */
  void decode(const eudaq::Event& event, HidraEventMeta& meta) const;
};

} // namespace hidra
