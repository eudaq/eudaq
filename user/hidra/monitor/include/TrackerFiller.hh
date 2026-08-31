#pragma once

#include "HistogramRegistry.hh"
#include "IHistogramFiller.hh"

#include <TH1.h>
#include <TH2.h>

#include <vector>

/**
 * @brief Per-station range/binning for one tracker 2D hit map.
 *
 * One entry per station; resolved in HidraHttpMonitor::DoConfigure() from the
 * `.conf` keys (global `TRACKER_X_*` / `TRACKER_Y_*` defaults, with an optional
 * per-station `TRACKER_STATION<i>` range override). The filler is "dumb": it
 * just books a TH2 with these numbers.
 */
struct TrackerStationConfig {
  int x_nbins = 100;
  double x_min = 0.0;
  double x_max = 1000.0;
  int y_nbins = 100;
  double y_min = 0.0;
  double y_max = 1000.0;
};

/**
 * @brief Fills the tracker monitoring histograms from HidraEvent::tracker.
 *
 * Books one 2D hit map (X vs Y occupancy) per tracker station, where a "station"
 * is a tracker plane: station i is filled with `(tracker.X[i], tracker.Y[i])`,
 * the coordinates produced by HidraTrackerDecoder. Histograms are named
 * `Tracker_station<i>`; the number of stations and the per-station X/Y range and
 * binning are configurable (see TrackerStationConfig).
 *
 * It also books a single `Tracker_errors` counter: a negative coordinate is the
 * producer's "no hit" sentinel, counted here as an error. One labelled bin per
 * station counts the events where that station has a negative X or Y, plus a
 * final `any` bin counting the events where at least one station is in error
 * (the OR over all stations).
 *
 * Events without a tracker sub-event leave `tracker.X` empty, so Fill() is a
 * no-op for them.
 */
class TrackerFiller : public IHistogramFiller {
public:
  explicit TrackerFiller(HistogramRegistry& reg, const std::vector<TrackerStationConfig>& stations);
  void Fill(const HidraEvent& ev) override;

private:
  // One occupancy map per station (x = X coordinate, y = Y coordinate).
  std::vector<TH2I*> m_station_hist;
  // Negative-coordinate (no-hit) counter: one bin per station + a final "any"
  // bin for the OR over all stations. Null when no stations are configured.
  TH1I* m_error_hist = nullptr;
};
