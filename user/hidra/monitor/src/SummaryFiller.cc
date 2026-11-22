#include "SummaryFiller.hh"

SummaryFiller::SummaryFiller(HistogramRegistry& reg, unsigned int prescale)
    : IHistogramFiller("SummaryFiller"),
      m_prescale(prescale > 0 ? static_cast<double>(prescale) : 1.0) {
  // Single-bin counter of the events the monitor actually processed
  // (sampled, i.e. post-prescale). TH1I is intentional: it counts only
  // 1/EVENT_PRESCALE of the stream and caps at INT_MAX (~2.1e9), which is
  // ample for a run. The true pre-prescale total lives in events_received
  // below (TH1D, to keep full-rate headroom on long runs).
  m_h_event_count = reg.Add(std::make_unique<TH1I>("event_count", "Event count", 1, 0, 1));
  // Single-bin counter holding the true (pre-prescale) number of events
  // received so far. TH1D (not TH1I) so it doesn't overflow on long runs.
  m_h_events_received = reg.Add(std::make_unique<TH1D>("events_received", "Events received (pre-prescale)", 1, 0, 1));
}

void SummaryFiller::Reset() {
  m_events_seen = 0;
}

void SummaryFiller::Fill(const HidraEvent&) {
  m_h_event_count->Fill(0);
  // We only see one of every EVENT_PRESCALE events, so the true count is
  // (sampled events) x prescale. Set (not Fill) since this is a running
  // total. Its *deltas* give the true rate regardless of the prescale.
  ++m_events_seen;
  m_h_events_received->SetBinContent(1, static_cast<double>(m_events_seen) * m_prescale);
}
