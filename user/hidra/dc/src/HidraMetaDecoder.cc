#include "HidraMetaDecoder.hh"
#include "HidraUtils.hh"

#include <cstdint>

namespace hidra {

void HidraMetaDecoder::decode(const eudaq::Event& event, HidraEventMeta& meta) const {
  meta = HidraEventMeta{};

  // Event-level metadata from the merged event.
  meta.run_number = event.GetRunNumber();
  meta.event_number = event.GetEventN();
  meta.trigger_number = event.GetTriggerN();
  meta.timestamp_begin_ns = event.GetTimestampBegin();
  meta.timestamp_end_ns = event.GetTimestampEnd();
  meta.detector_mask = hidra::utils::getTagOr<int>(event, "detectorMask", -1, false);

  // triggerMask / spillNumber are read forward-compatibly from the merged event first (the collector does not set them
  // there today), then from the XDC sub-event where the producer actually puts them (detID 1 real / 6 dry).
  meta.trigger_mask = hidra::utils::getTagOr<int>(event, "triggerMask", -1, false);
  meta.spill_number = hidra::utils::getTagOr<std::uint32_t>(event, "spillNumber", 0xFFFFFFFFu, false);

  for (std::uint32_t index = 0; index < event.GetNumSubEvent(); ++index) {
    const eudaq::EventSPC sub = event.GetSubEvent(index);
    if (!sub) {
      continue;
    }
    // Default to a sentinel (-1) so a sub-event without an explicit detID tag is never
    // mistaken for XDC just because its index happens to be 1 or 6.
    const int det_id = hidra::utils::getTagOr<int>(*sub, "detID", -1, false);
    const bool is_xdc = (det_id == 1 || det_id == 6);
    if (!is_xdc) {
      continue;
    }
    if (meta.trigger_mask < 0) {
      meta.trigger_mask = hidra::utils::getTagOr<int>(*sub, "triggerMask", -1, false);
    }
    if (meta.spill_number == 0xFFFFFFFFu) {
      meta.spill_number = hidra::utils::getTagOr<std::uint32_t>(*sub, "spillNumber", 0xFFFFFFFFu, false);
    }
  }
}

} // namespace hidra
