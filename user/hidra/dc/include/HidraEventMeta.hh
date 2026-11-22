#pragma once

#include <cstdint>

/**
 * @brief Per-event metadata decoded from the EUDAQ event tags / header.
 *
 * These values are not part of the detector binary payloads; they come from the
 * EUDAQ event (event-level fields) and from sub-event tags set by the producers
 * (e.g. triggerMask, spillNumber). See DataFormat.md and HidraMetaDecoder.
 */
struct HidraEventMeta {
  uint32_t run_number{0};
  uint32_t event_number{0};
  uint32_t trigger_number{0};
  uint64_t timestamp_begin_ns{0};
  uint64_t timestamp_end_ns{0};

  int detector_mask{-1};            ///< "detectorMask" tag of the merged event; -1 if absent.
  int trigger_mask{-1};             ///< Raw trigger mask (0 gate, 1 physics, 2 pedestal, 3 both); -1 if absent.
  uint32_t spill_number{0xFFFFFFFF}; ///< Spill number; 0xFFFFFFFF if absent.

  /** Physics trigger flagged (bit 0 of the trigger mask). False if the mask is absent. */
  bool isPhysics() const { return trigger_mask >= 0 && (trigger_mask & 0x1); }
  /** Pedestal trigger flagged (bit 1 of the trigger mask). False if the mask is absent. */
  bool isPedestal() const { return trigger_mask >= 0 && (trigger_mask & 0x2); }
};
