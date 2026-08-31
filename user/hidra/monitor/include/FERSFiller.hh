#pragma once

#include "HidraEvent.hh"
#include "HistogramRegistry.hh"
#include "IHistogramFiller.hh"

#include <TH1.h>
#include <TH2.h>
#include <TProfile.h>

#include <string>

/**
 * @brief Fills the FERS monitoring histograms from HidraEvent::fers.
 *
 * Twin of XDCFiller, for the FERS detector (n_boards × 64 channels, flat
 * per-channel vectors with a -1 "no hit" sentinel). It publishes:
 *   - per-channel mean HG and LG (TProfile, x = channel), split into
 *     physics/pedestal via the trigger mask, plus an inclusive (physics)
 *     distribution over all channels;
 *   - per-channel HG and LG distributions as one TH2I per gain/trigger (x =
 *     channel, y = ADC), split physics/pedestal only (no standalone "total"
 *     copy, to save memory — physics vs pedestal is the useful overlay). The
 *     frontend reads a single channel via a server-side ProjectionY, so a single
 *     TH2 keeps the registered-object count tiny and THttpServer responsive
 *     (issue #138). Optional, disabled when `per_channel_distributions` is false;
 *   - per-channel saturation fraction HG and LG (TProfile of a 0/1 indicator,
 *     x = channel), total and physics only (pedestal events don't saturate): a
 *     channel is saturated when its value exceeds a configurable threshold;
 *   - per-board aggregates (TProfile, x = board): the mean over all channels of
 *     the board for each gain (`FERS_HG_board_mean` / `FERS_LG_board_mean`) and
 *     the fraction of events in which every HG channel of the board reads zero
 *     (`FERS_HG_board_allzero`, e.g. a board that stopped responding);
 *   - board-time compatibility: per board, the offset of its timestamp from the
 *     median over all boards present in the event (TProfile, x = board);
 *   - trigger-ID consistency: a per-event 2-bin counter
 *     (`FERS_trigger_id_consistency`, consistent / mismatch). All boards of an
 *     event are triggered together, so every present channel should carry the
 *     same `FERStrigger_id`; a board reading a different trigger id (desync)
 *     lands the event in the "mismatch" bin.
 *
 * The data source is identical regardless of how `fers` was produced (the real
 * HidraFersDecoder or the random test decoder).
 *
 * The same filler serves the MAXICC crystal calorimeter, which is just another
 * set of FERS boards in its own sub-event: pass `name_prefix = "MAXICC"` and
 * `field = &HidraEvent::maxicc` so the histograms are named `MAXICC_*` and the
 * data is read from `HidraEvent::maxicc` instead of `::fers`.
 */
class FERSFiller : public IHistogramFiller {
public:
  explicit FERSFiller(HistogramRegistry& reg,
                      unsigned int n_boards = 20,
                      unsigned int channels_per_board = 64,
                      int value_max = 4096,
                      int channel_nbins = 1024,
                      int saturation_threshold = 3800,
                      bool per_channel_distributions = true,
                      std::string name_prefix = "FERS",
                      HidraFersEvent HidraEvent::*field = &HidraEvent::fers);
  void Fill(const HidraEvent&) override;

private:
  void FillBoardTime(const HidraFersEvent& fers);
  void FillBoardAllZeroHG(const HidraFersEvent& fers);
  void FillTriggerIdConsistency(const HidraFersEvent& fers);

  // Histogram-name prefix ("FERS"/"MAXICC") and which HidraEvent sub-event this
  // filler reads (::fers / ::maxicc), so one class serves both detectors.
  std::string m_prefix;
  HidraFersEvent HidraEvent::*m_field;

  unsigned int m_n_boards;
  unsigned int m_channels_per_board;
  unsigned int m_n_channels;
  double m_saturation_threshold;
  // When false, the per-channel TH1I distributions are neither booked nor filled
  // (the per-channel vectors stay empty) — saves memory/startup for routine ops.
  bool m_per_channel;

  // Per-channel mean (x = channel). The ";channel;" x-axis title marks these as
  // channel-indexed so the frontend labels the hover with the channel number.
  TProfile* m_hg_mean;
  TProfile* m_hg_mean_physics;
  TProfile* m_hg_mean_pedestal;
  TProfile* m_lg_mean;
  TProfile* m_lg_mean_physics;
  TProfile* m_lg_mean_pedestal;

  // Inclusive distribution over all channels (physics only).
  TH1I* m_hg_inclusive_physics;
  TH1I* m_lg_inclusive_physics;

  // Per-channel saturation fraction (TProfile of a 0/1 indicator, x = channel).
  // Total and physics only — pedestal events don't saturate.
  TProfile* m_hg_saturation;
  TProfile* m_hg_saturation_physics;
  TProfile* m_lg_saturation;
  TProfile* m_lg_saturation_physics;

  // Per-channel distributions as one TH2I per gain/trigger (x = channel,
  // y = ADC), physics and pedestal only (no "total" copy, to save memory). A
  // single TH2 keeps the registered-object count tiny (4 instead of one TH1I
  // per channel): the frontend reads a single channel via a server-side
  // ProjectionY, so the THttpServer stays responsive (see issue #138). TH1I/TH2I
  // (Int_t, 4 B per bin) since the contents are integer counts.
  TH2I* m_hg_dist_physics;
  TH2I* m_hg_dist_pedestal;
  TH2I* m_lg_dist_physics;
  TH2I* m_lg_dist_pedestal;

  // Per-board aggregates (x = board).
  TProfile* m_hg_board_mean;    // mean HG over all channels of the board
  TProfile* m_lg_board_mean;    // mean LG over all channels of the board
  TProfile* m_hg_board_allzero; // fraction of events with every HG channel == 0

  // Board time offset vs the per-event median (x = board).
  TProfile* m_board_time_offset;

  // Per-event trigger-ID consistency: 2-bin counter (bin 1 "consistent", bin 2
  // "mismatch"). All present channels of an event should share one FERStrigger_id.
  TH1I* m_trigger_id_consistency;
};
