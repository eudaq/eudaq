#ifndef HIDRA_FERS2_PAYLOAD_SERIALIZATION_H
#define HIDRA_FERS2_PAYLOAD_SERIALIZATION_H

#include <cstring>
#include <string>

#include "FERSTypes.h"
#include "FERSlib.h"

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

namespace hidra {
namespace fers2 {

inline bool SerializeFersEventPayload(void* event_ptr,
                                      int data_qualifier,
                                      int vendor_board_index,
                                      int logical_board_id,
                                      FERSEvent* out_event,
                                      std::string* error) {
  if (out_event == nullptr || event_ptr == nullptr) {
    if (error != nullptr) {
      *error = "SerializeFersEventPayload received null pointer.";
    }
    return false;
  }

  const int base_dq = (data_qualifier & 0x0F);

  out_event->data_qualifier = data_qualifier;
  // `board_id` is the logical board identifier assigned by the manager.
  // `board_index` is the vendor-provided index (from FERS_GetEvent) and
  // may differ when using concentrator/TDL reads. Populate both so callers
  // can access either mapping.
  out_event->board_id = logical_board_id;
  out_event->board_index = vendor_board_index;

  if (data_qualifier == DTQ_SERVICE) {
    const auto* ev = reinterpret_cast<const ServEvent_t*>(event_ptr);
    out_event->trigger_id = ev->TotTrg_cnt;
    out_event->payload.resize(sizeof(ServEvent_t));
    std::memcpy(out_event->payload.data(), ev, sizeof(ServEvent_t));
    return true;
  }

  /*if (base_dq == DTQ_SPECT || data_qualifier == DTQ_TSPECT) { */
  if (base_dq == DTQ_SPECT || base_dq == DTQ_TSPECT) { // TODO: clarify difference between base_dq and data_qualifier 
    const auto* ev = reinterpret_cast<const SpectEvent_t*>(event_ptr);
    out_event->trigger_id = ev->trigger_id;
    out_event->payload.resize(sizeof(SpectEvent_t));
    std::memcpy(out_event->payload.data(), ev, sizeof(SpectEvent_t));
    return true;
  }

  if (base_dq == DTQ_TIMING) {
    const auto* ev = reinterpret_cast<const ListEvent_t*>(event_ptr);
    out_event->trigger_id = ev->trigger_id;
    out_event->payload.resize(sizeof(ListEvent_t));
    std::memcpy(out_event->payload.data(), ev, sizeof(ListEvent_t));
    return true;
  }

  if (base_dq == DTQ_COUNT) {
    const auto* ev = reinterpret_cast<const CountingEvent_t*>(event_ptr);
    out_event->trigger_id = ev->trigger_id;
    out_event->payload.resize(sizeof(CountingEvent_t));
    std::memcpy(out_event->payload.data(), ev, sizeof(CountingEvent_t));
    return true;
  }

  if (base_dq == DTQ_TEST || data_qualifier == DTQ_TEST) {
    const auto* ev = reinterpret_cast<const TestEvent_t*>(event_ptr);
    out_event->trigger_id = ev->trigger_id;
    out_event->payload.resize(sizeof(TestEvent_t));
    std::memcpy(out_event->payload.data(), ev, sizeof(TestEvent_t));
    return true;
  }

  if (base_dq == DTQ_WAVE) {
    if (error != nullptr) {
      *error = "Waveform event serialization is not implemented yet.";
    }
    return false;
  }

  if (error != nullptr) {
    *error = "Unsupported data qualifier: " + std::to_string(data_qualifier);
  }
  return false;
}

} // namespace fers2
} // namespace hidra

#endif // HIDRA_FERS2_PAYLOAD_SERIALIZATION_H