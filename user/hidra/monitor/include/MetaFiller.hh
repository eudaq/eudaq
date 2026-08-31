#pragma once
#include "HistogramRegistry.hh"
#include "IHistogramFiller.hh"

#include <TH1.h>

#include <cstdint>
#include <vector>

/**
 * @brief Fills histograms from the per-event metadata (HidraEvent::meta).
 *
 * Produces the trigger-mask classification (gate / physics / pedestal / both),
 * the set of detectors present, the events-per-spill structure, the inter-event
 * time distribution, single-bin "current value" readouts for spill / trigger
 * / run number, and two trigger-mask-pattern monitors: a strip of the last N raw
 * trigger-mask values and the distribution of non-pedestal events between two
 * consecutive pedestals (nominal = 10, for a repeating 10-physics : 1-pedestal
 * pattern).
 */
class MetaFiller : public IHistogramFiller {
public:
  /**
   * @param strip_length number of recent trigger-mask values shown in the strip.
   * @param gap_max upper edge (and bin count) of the events-between-pedestals histogram.
   */
  MetaFiller(HistogramRegistry& reg, int strip_length, int gap_max);
  void Fill(const HidraEvent& ev) override;
  void Reset() override;

private:
  TH1I* m_h_trigger_mask;
  TH1I* m_h_detectors_present;
  TH1D* m_h_events_per_spill;
  TH1D* m_h_dt_between_events; // inter-event time, log-binned, in microseconds
  TH1D* m_h_spill_current;
  TH1D* m_h_trigger_current;
  TH1D* m_h_run_current;
  TH1D* m_h_trigger_mask_recent;       // strip of the last N raw trigger-mask values
  TH1I* m_h_events_between_pedestals;   // # non-pedestal events between consecutive pedestals

  // Run-relative state for the inter-event time: begin timestamp of the previous event.
  uint64_t m_last_begin_ns = 0;
  bool m_have_last = false;

  // Ring buffer backing the recent-trigger-mask strip. m_recent holds the last
  // m_strip_length raw mask values; m_recent_head is the next write slot and
  // m_recent_count how many slots are filled (< m_strip_length only at startup).
  std::vector<int> m_recent;
  int m_strip_length = 0;
  int m_recent_head = 0;
  int m_recent_count = 0;

  // Run-relative state for the events-between-pedestals gap. Counts non-pedestal
  // events since the last pedestal; m_seen_pedestal gates the first (incomplete)
  // segment, which starts mid-cycle and must not be recorded.
  int m_events_since_pedestal = 0;
  bool m_seen_pedestal = false;
};
