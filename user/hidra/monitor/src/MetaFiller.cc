#include "MetaFiller.hh"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <vector>

namespace {
// Log-spaced bin edges for the inter-event time histogram, covering a wide range
// (1 us .. 10 s) so the (variable) event rate always lands on a sensible scale.
// Precondition: hi_us > lo_us and bins_per_decade > 0 (so n > 0).
std::vector<double> LogBinEdges(double lo_us, double hi_us, int bins_per_decade) {
  const double lo = std::log10(lo_us);
  const double hi = std::log10(hi_us);
  const int n = static_cast<int>(std::lround((hi - lo) * bins_per_decade));
  assert(n > 0 && "LogBinEdges requires hi_us > lo_us and bins_per_decade > 0");
  std::vector<double> edges(n + 1);
  for (int i = 0; i <= n; ++i) {
    edges[i] = std::pow(10.0, lo + (hi - lo) * i / n);
  }
  return edges;
}
} // namespace

MetaFiller::MetaFiller(HistogramRegistry& reg, int strip_length, int gap_max)
    : IHistogramFiller("MetaFiller") {
  // Trigger mask: one bin per value (0..3). Labels are cosmetic; events are filled
  // by numeric value, so events with no trigger mask (meta.trigger_mask < 0) are
  // simply not filled.
  m_h_trigger_mask = reg.Add(std::make_unique<TH1I>("trigger_mask", "Trigger mask;;events", 4, 0, 4));
  m_h_trigger_mask->SetCanExtend(TH1::kNoAxis);
  m_h_trigger_mask->GetXaxis()->SetBinLabel(1, "gate");     // 0
  m_h_trigger_mask->GetXaxis()->SetBinLabel(2, "physics");  // 1
  m_h_trigger_mask->GetXaxis()->SetBinLabel(3, "pedestal"); // 2
  m_h_trigger_mask->GetXaxis()->SetBinLabel(4, "both");     // 3

  // Detectors present: one bin per detID (0..7), filled from the set bits of the
  // detector mask. A detector dropping out shows fewer counts than the others.
  m_h_detectors_present =
      reg.Add(std::make_unique<TH1I>("detectors_present", "Detectors present;detID;events", 8, 0, 8));
  m_h_detectors_present->SetCanExtend(TH1::kNoAxis);
  for (int det = 0; det < 8; ++det) {
    m_h_detectors_present->GetXaxis()->SetBinLabel(det + 1, std::to_string(det).c_str());
  }

  // Events per spill: auto-extending axis so the (unbounded, slowly growing) spill
  // number does not need a fixed range.
  m_h_events_per_spill =
      reg.Add(std::make_unique<TH1D>("events_per_spill", "Events per spill;spill;events", 1, 0, 1));
  m_h_events_per_spill->SetCanExtend(TH1::kAllAxes);

  // Inter-event time (begin[i] - begin[i-1]), log-binned in microseconds.
  const std::vector<double> dt_edges = LogBinEdges(1.0, 1.0e7, 20); // 1 us .. 10 s, 20 bins/decade
  m_h_dt_between_events = reg.Add(std::make_unique<TH1D>(
      "dt_between_events", "Time between consecutive events;#Delta t [#mus];events",
      static_cast<int>(dt_edges.size()) - 1, dt_edges.data()));

  // Single-bin "current value" readouts (latest value, not a distribution).
  m_h_spill_current = reg.Add(std::make_unique<TH1D>("spill_current", "Current spill number", 1, 0, 1));
  m_h_trigger_current = reg.Add(std::make_unique<TH1D>("trigger_current", "Current trigger number", 1, 0, 1));
  m_h_run_current = reg.Add(std::make_unique<TH1D>("run_current", "Current run number", 1, 0, 1));

  // Recent-trigger-mask strip: the last `strip_length` values, oldest in bin 1,
  // newest in bin `strip_length`, re-written from a ring buffer every event. The
  // bin holds the trigger *class* encoded as mask+1, so 0 means "no data": this
  // coincides with ROOT's reset state (TH1::Reset zeroes the bins), so a freshly
  // reset or not-yet-filled strip reads as empty instead of as gate (mask 0).
  // Reading the strip assumes the monitor sees consecutive events (true when the
  // data-collector send-monitor fraction is 1).
  m_strip_length = strip_length;
  m_recent.assign(strip_length, -1);
  m_h_trigger_mask_recent = reg.Add(std::make_unique<TH1D>(
      "trigger_mask_recent", "Recent trigger masks;event index (newest at right);trigger class (0 = none)", strip_length,
      0, strip_length));
  m_h_trigger_mask_recent->SetCanExtend(TH1::kNoAxis);

  // Events between pedestals: count of non-pedestal events between two consecutive
  // pedestals. For a repeating 10-physics : 1-pedestal pattern this peaks at 10;
  // bins at 9/11 flag an early/missing pedestal. The overflow bin catches longer gaps.
  m_h_events_between_pedestals = reg.Add(std::make_unique<TH1I>(
      "events_between_pedestals", "Events between pedestals;# non-pedestal events between pedestals;occurrences",
      gap_max, 0, gap_max));
  m_h_events_between_pedestals->SetCanExtend(TH1::kNoAxis);
}

void MetaFiller::Reset() {
  m_last_begin_ns = 0;
  m_have_last = false;

  m_recent_head = 0;
  m_recent_count = 0;
  std::fill(m_recent.begin(), m_recent.end(), -1);

  m_events_since_pedestal = 0;
  m_seen_pedestal = false;
}

void MetaFiller::Fill(const HidraEvent& ev) {
  const HidraEventMeta& meta = ev.meta;

  // Fill unconditionally so the total matches the processed-event count: a
  // missing trigger mask (mask < 0) lands in ROOT's underflow bin and an
  // unexpected out-of-range value (>= 4) in overflow, instead of being dropped.
  m_h_trigger_mask->Fill(meta.trigger_mask);

  if (meta.detector_mask >= 0) {
    for (int det = 0; det < 8; ++det) {
      if (meta.detector_mask & (1 << det)) {
        m_h_detectors_present->Fill(det);
      }
    }
  }

  if (meta.spill_number != 0xFFFFFFFFu) {
    m_h_events_per_spill->Fill(static_cast<double>(meta.spill_number));
    m_h_spill_current->SetBinContent(1, static_cast<double>(meta.spill_number));
  }

  // Inter-event time from the begin timestamps of consecutive events. A missing/unset
  // begin timestamp (0) is skipped entirely so it neither produces a spurious large dt
  // nor breaks the chain: the next valid event measures dt against the last valid one.
  if (meta.timestamp_begin_ns != 0) {
    if (m_have_last && meta.timestamp_begin_ns > m_last_begin_ns) {
      const double dt_us = static_cast<double>(meta.timestamp_begin_ns - m_last_begin_ns) * 1e-3;
      m_h_dt_between_events->Fill(dt_us);
    }
    m_last_begin_ns = meta.timestamp_begin_ns;
    m_have_last = true;
  }

  m_h_trigger_current->SetBinContent(1, static_cast<double>(meta.trigger_number));
  m_h_run_current->SetBinContent(1, static_cast<double>(meta.run_number));

  // Recent-trigger-mask strip: push the raw mask into the ring buffer, then rewrite
  // all bins so bin 1 is the oldest and bin `m_strip_length` the newest value. The
  // stored class is mask+1 (0 = no data): unfilled ring slots and an absent mask
  // (mask < 0) both encode to 0 so they read as empty. Rewriting m_strip_length
  // bins per event is negligible (~200 ops).
  if (m_strip_length > 0) {
    m_recent[m_recent_head] = meta.trigger_mask;
    m_recent_head = (m_recent_head + 1) % m_strip_length;
    if (m_recent_count < m_strip_length) {
      ++m_recent_count;
    }
    const int oldest = (m_recent_head - m_recent_count + m_strip_length) % m_strip_length;
    for (int k = 0; k < m_strip_length; ++k) {
      double encoded = 0.0; // 0 = no data (unfilled slot or absent mask)
      if (k < m_recent_count) {
        const int mask = m_recent[(oldest + k) % m_strip_length];
        if (mask >= 0) {
          encoded = static_cast<double>(mask + 1);
        }
      }
      m_h_trigger_mask_recent->SetBinContent(k + 1, encoded);
    }
    // SetBinContent bumps fEntries on every call, so without this the strip would
    // report ~m_strip_length entries per event. Pin it to the number of real values.
    m_h_trigger_mask_recent->SetEntries(m_recent_count);
  }

  // Events between pedestals: only count events that carry a valid mask. A pedestal
  // (pedestal bit set, so also "both"=3) closes a segment and records its length;
  // the first segment is skipped (it starts mid-cycle, before the first pedestal).
  if (meta.trigger_mask >= 0) {
    if (meta.isPedestal()) {
      if (m_seen_pedestal) {
        m_h_events_between_pedestals->Fill(m_events_since_pedestal);
      }
      m_seen_pedestal = true;
      m_events_since_pedestal = 0;
    } else {
      ++m_events_since_pedestal;
    }
  }
}
