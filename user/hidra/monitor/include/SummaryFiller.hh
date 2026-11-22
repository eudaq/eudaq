#pragma once
#include "HistogramRegistry.hh"
#include "IHistogramFiller.hh"

#include <TH1.h>

#include <cstdint>

class SummaryFiller : public IHistogramFiller {
public:
  // `prescale` is the monitor's EVENT_PRESCALE: Fill() runs once per
  // sampled event, so we scale our internal count by it to recover the
  // true (pre-prescale) number of events for the rate readout.
  explicit SummaryFiller(HistogramRegistry& reg, unsigned int prescale = 1);
  void Fill(const HidraEvent&) override;
  void Reset() override;

private:
  TH1I* m_h_event_count;
  // True (pre-prescale) cumulative count of events seen by the monitor.
  // The frontend derives the real event rate from its time deltas.
  TH1D* m_h_events_received;
  double m_prescale;          // EVENT_PRESCALE, as a multiplier
  uint64_t m_events_seen = 0; // sampled events this filler has processed
};