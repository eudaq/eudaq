#include "FERSFiller.hh"
#include "HidraUtils.hh"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <memory>
#include <vector>

FERSFiller::FERSFiller(HistogramRegistry& reg,
                       unsigned int n_boards,
                       unsigned int channels_per_board,
                       int value_max,
                       int channel_nbins,
                       int saturation_threshold,
                       bool per_channel_distributions,
                       std::string name_prefix,
                       HidraFersEvent HidraEvent::*field)
    : IHistogramFiller(name_prefix + "Filler"),
      m_prefix(std::move(name_prefix)),
      m_field(field),
      m_n_boards(n_boards),
      m_channels_per_board(channels_per_board),
      m_n_channels(0),
      m_saturation_threshold(saturation_threshold),
      m_per_channel(per_channel_distributions) {
  if (m_n_boards == 0) {
    HIDRA_WARN("{} n_boards=0 is invalid, forcing 1.", IHistogramFiller::Name());
    m_n_boards = 1;
  }
  if (m_channels_per_board == 0) {
    HIDRA_WARN("{} channels_per_board=0 is invalid, forcing 64.", IHistogramFiller::Name());
    m_channels_per_board = 64;
  }
  if (channel_nbins < 1) {
    HIDRA_WARN("{} channel_nbins={} is invalid, forcing 1024.", IHistogramFiller::Name(), channel_nbins);
    channel_nbins = 1024;
  }
  if (value_max < 1) {
    HIDRA_WARN("{} value_max={} is invalid, forcing 4096.", IHistogramFiller::Name(), value_max);
    value_max = 4096;
  }
  if (saturation_threshold < 0 || saturation_threshold >= value_max) {
    HIDRA_WARN("{} saturation_threshold={} outside [0,{}); clamping.", IHistogramFiller::Name(), saturation_threshold,
               value_max);
    saturation_threshold = std::clamp(saturation_threshold, 0, value_max - 1);
    m_saturation_threshold = saturation_threshold;
  }
  m_n_channels = m_n_boards * m_channels_per_board;

  // All histogram names and titles carry the detector prefix (`FERS`/`MAXICC`) so
  // one filler class can serve both detectors without name collisions.
  const std::string& P = m_prefix;
  auto hname = [&](const std::string& suffix) { return P + "_" + suffix; };

  // Per-channel means. The ";channel;<y>" axis titles mark them as channel-indexed
  // for the frontend (channel-numbered hover, used by the detector heatmap).
  m_hg_mean = reg.Add(std::make_unique<TProfile>(
      hname("HG_mean").c_str(), hidra::utils::format("Mean {} high gain;channel;mean HG", P).c_str(), m_n_channels, 0,
      m_n_channels));
  m_hg_mean_physics = reg.Add(std::make_unique<TProfile>(
      hname("HG_mean_physics").c_str(), hidra::utils::format("Mean {} high gain (physics);channel;mean HG", P).c_str(),
      m_n_channels, 0, m_n_channels));
  m_hg_mean_pedestal = reg.Add(std::make_unique<TProfile>(
      hname("HG_mean_pedestal").c_str(), hidra::utils::format("Mean {} high gain (pedestal);channel;mean HG", P).c_str(),
      m_n_channels, 0, m_n_channels));
  m_lg_mean = reg.Add(std::make_unique<TProfile>(
      hname("LG_mean").c_str(), hidra::utils::format("Mean {} low gain;channel;mean LG", P).c_str(), m_n_channels, 0,
      m_n_channels));
  m_lg_mean_physics = reg.Add(std::make_unique<TProfile>(
      hname("LG_mean_physics").c_str(), hidra::utils::format("Mean {} low gain (physics);channel;mean LG", P).c_str(),
      m_n_channels, 0, m_n_channels));
  m_lg_mean_pedestal = reg.Add(std::make_unique<TProfile>(
      hname("LG_mean_pedestal").c_str(), hidra::utils::format("Mean {} low gain (pedestal);channel;mean LG", P).c_str(),
      m_n_channels, 0, m_n_channels));

  // Inclusive distributions over all channels (physics only).
  m_hg_inclusive_physics = reg.Add(std::make_unique<TH1I>(
      hname("HG_inclusive_physics").c_str(),
      hidra::utils::format("Inclusive {} high gain (physics);HG [ADC];entries", P).c_str(), channel_nbins, 0,
      value_max));
  m_lg_inclusive_physics = reg.Add(std::make_unique<TH1I>(
      hname("LG_inclusive_physics").c_str(),
      hidra::utils::format("Inclusive {} low gain (physics);LG [ADC];entries", P).c_str(), channel_nbins, 0, value_max));

  // Per-channel distributions: one TH2I per gain/trigger (x = channel, y = ADC),
  // physics and pedestal only (no "total" copy, to save memory). One TH2 instead
  // of one TH1I per channel keeps the registered-object count small; the frontend
  // reads a single channel via a server-side ProjectionY (issue #138). Skipped
  // entirely when disabled (the pointers stay null and Fill guards on m_per_channel).
  auto book_dist = [&](const char* gain, const char* trig) {
    return reg.Add(std::make_unique<TH2I>(
        hidra::utils::format("{}_{}_dist_{}", P, gain, trig).c_str(),
        hidra::utils::format("{} {} ({});channel;{} [ADC]", P, gain, trig, gain).c_str(),
        m_n_channels, 0, m_n_channels, channel_nbins, 0, value_max));
  };
  if (m_per_channel) {
    m_hg_dist_physics = book_dist("HG", "physics");
    m_hg_dist_pedestal = book_dist("HG", "pedestal");
    m_lg_dist_physics = book_dist("LG", "physics");
    m_lg_dist_pedestal = book_dist("LG", "pedestal");
  } else {
    m_hg_dist_physics = m_hg_dist_pedestal = m_lg_dist_physics = m_lg_dist_pedestal = nullptr;
  }

  // Per-channel saturation fraction (TProfile of a 0/1 indicator). Total and
  // physics only — pedestal events don't saturate. The ";channel;" x-axis title
  // marks them as channel-indexed for the frontend.
  m_hg_saturation = reg.Add(std::make_unique<TProfile>(
      hname("HG_saturation").c_str(),
      hidra::utils::format("Saturation fraction per {} HG channel;channel;saturation fraction", P).c_str(),
      m_n_channels, 0, m_n_channels));
  m_hg_saturation_physics = reg.Add(std::make_unique<TProfile>(
      hname("HG_saturation_physics").c_str(),
      hidra::utils::format("Saturation fraction per {} HG channel (physics);channel;saturation fraction", P).c_str(),
      m_n_channels, 0, m_n_channels));
  m_lg_saturation = reg.Add(std::make_unique<TProfile>(
      hname("LG_saturation").c_str(),
      hidra::utils::format("Saturation fraction per {} LG channel;channel;saturation fraction", P).c_str(),
      m_n_channels, 0, m_n_channels));
  m_lg_saturation_physics = reg.Add(std::make_unique<TProfile>(
      hname("LG_saturation_physics").c_str(),
      hidra::utils::format("Saturation fraction per {} LG channel (physics);channel;saturation fraction", P).c_str(),
      m_n_channels, 0, m_n_channels));

  // Per-board aggregates.
  m_hg_board_mean = reg.Add(std::make_unique<TProfile>(
      hname("HG_board_mean").c_str(),
      hidra::utils::format("Mean {} high gain over board channels;board;mean HG", P).c_str(), m_n_boards, 0,
      m_n_boards));
  m_lg_board_mean = reg.Add(std::make_unique<TProfile>(
      hname("LG_board_mean").c_str(),
      hidra::utils::format("Mean {} low gain over board channels;board;mean LG", P).c_str(), m_n_boards, 0,
      m_n_boards));
  m_hg_board_allzero = reg.Add(std::make_unique<TProfile>(
      hname("HG_board_allzero").c_str(),
      "Fraction of events with all HG channels zero;board;all-zero fraction", m_n_boards, 0, m_n_boards));

  // Board-time compatibility: per-board offset from the per-event median.
  m_board_time_offset = reg.Add(std::make_unique<TProfile>(
      hname("board_time_offset").c_str(), "Board time offset vs median;board;rel. time offset [#mus]", m_n_boards, 0,
      m_n_boards));

  // Per-event trigger-ID consistency: one count per event into "consistent"
  // (bin 1) or "mismatch" (bin 2). ROOT bin labels for the JSROOT/snapshot view;
  // the Dash frontend labels the bars itself via the panel `bin_labels` key.
  m_trigger_id_consistency = reg.Add(std::make_unique<TH1I>(
      hname("trigger_id_consistency").c_str(),
      hidra::utils::format("{} trigger ID consistency;;events", P).c_str(), 2, 0, 2));
  m_trigger_id_consistency->SetCanExtend(TH1::kNoAxis);
  m_trigger_id_consistency->GetXaxis()->SetBinLabel(1, "consistent");
  m_trigger_id_consistency->GetXaxis()->SetBinLabel(2, "mismatch");
}

void FERSFiller::Fill(const HidraEvent& event) {
  // Split copies are chosen once per event from the trigger mask (physics = bit
  // 0, pedestal = bit 1); a "both" event feeds both. The per-channel
  // distributions exist only for physics/pedestal; the mean profiles also keep
  // an inclusive (total) copy.
  const bool is_physics = event.meta.isPhysics();
  const bool is_pedestal = event.meta.isPedestal();
  const HidraFersEvent& fers = event.*m_field;

  const std::size_t n_hg = std::min<std::size_t>(fers.FERShg.size(), m_n_channels);
  for (std::size_t i = 0; i < n_hg; ++i) {
    const double hg = fers.FERShg[i];
    // The decoder leaves absent channels at the -1 sentinel; skip them.
    if (hg >= 0) {
      m_hg_mean->Fill(i, hg);
      m_hg_board_mean->Fill(i / m_channels_per_board, hg);
      if (is_physics) {
        m_hg_mean_physics->Fill(i, hg);
        m_hg_inclusive_physics->Fill(hg);
        if (m_per_channel) {
          m_hg_dist_physics->Fill(i, hg);
        }
      }
      if (is_pedestal) {
        m_hg_mean_pedestal->Fill(i, hg);
        if (m_per_channel) {
          m_hg_dist_pedestal->Fill(i, hg);
        }
      }
      const int saturated = hg > m_saturation_threshold ? 1 : 0;
      m_hg_saturation->Fill(i, saturated);
      if (is_physics) {
        m_hg_saturation_physics->Fill(i, saturated);
      }
    }
  }

  const std::size_t n_lg = std::min<std::size_t>(fers.FERSlg.size(), m_n_channels);
  for (std::size_t i = 0; i < n_lg; ++i) {
    const double lg = fers.FERSlg[i];
    if (lg >= 0) {
      m_lg_mean->Fill(i, lg);
      m_lg_board_mean->Fill(i / m_channels_per_board, lg);
      if (is_physics) {
        m_lg_mean_physics->Fill(i, lg);
        m_lg_inclusive_physics->Fill(lg);
        if (m_per_channel) {
          m_lg_dist_physics->Fill(i, lg);
        }
      }
      if (is_pedestal) {
        m_lg_mean_pedestal->Fill(i, lg);
        if (m_per_channel) {
          m_lg_dist_pedestal->Fill(i, lg);
        }
      }
      const int saturated = lg > m_saturation_threshold ? 1 : 0;
      m_lg_saturation->Fill(i, saturated);
      if (is_physics) {
        m_lg_saturation_physics->Fill(i, saturated);
      }
    }
  }

  FillBoardAllZeroHG(fers);
  FillBoardTime(fers);
  FillTriggerIdConsistency(fers);
}

void FERSFiller::FillTriggerIdConsistency(const HidraFersEvent& fers) {
  // All boards of an event are triggered together, so every present channel
  // should report the same FERStrigger_id. Take the first present channel as the
  // reference and check the rest; absent channels carry the -1 sentinel and are
  // skipped. One count per event: "consistent" (bin 1) or "mismatch" (bin 2).
  const std::size_t n = std::min<std::size_t>(fers.FERStrigger_id.size(), m_n_channels);
  double reference = -1.0;
  bool have_reference = false;
  bool mismatch = false;
  for (std::size_t i = 0; i < n; ++i) {
    const double trig = fers.FERStrigger_id[i];
    if (trig < 0) { // absent channel
      continue;
    }
    if (!have_reference) {
      reference = trig;
      have_reference = true;
    } else if (trig != reference) {
      mismatch = true;
      break;
    }
  }
  if (!have_reference) {
    return; // no FERS data this event -> nothing to check
  }
  m_trigger_id_consistency->Fill(mismatch ? 1.0 : 0.0);
}

void FERSFiller::FillBoardAllZeroHG(const HidraFersEvent& fers) {
  // Per board: 1 if every present HG channel reads exactly zero, else 0 — flags a
  // board that is present (has data) but stopped producing signal. Boards with no
  // present channel at all are skipped (absent, not "all zero").
  for (unsigned int b = 0; b < m_n_boards; ++b) {
    bool any_present = false;
    bool any_nonzero = false;
    for (unsigned int ich = 0; ich < m_channels_per_board; ++ich) {
      const std::size_t idx = static_cast<std::size_t>(b) * m_channels_per_board + ich;
      if (idx >= fers.FERShg.size()) {
        break;
      }
      const double v = fers.FERShg[idx];
      if (v >= 0) { // present (−1 is the absent sentinel)
        any_present = true;
        if (v > 0) {
          any_nonzero = true;
          break;
        }
      }
    }
    if (any_present) {
      m_hg_board_allzero->Fill(b, any_nonzero ? 0 : 1);
    }
  }
}

void FERSFiller::FillBoardTime(const HidraFersEvent& fers) {
  // Each board's timestamp is replicated across its channels. Take the time of
  // the first present channel of each board, then offset every present board
  // from the median over the boards present in this event — the median is robust
  // to a single board being far off (which is exactly what we want to flag).
  std::vector<double> board_time(m_n_boards, std::numeric_limits<double>::quiet_NaN());
  for (unsigned int b = 0; b < m_n_boards; ++b) {
    for (unsigned int ich = 0; ich < m_channels_per_board; ++ich) {
      const std::size_t idx = static_cast<std::size_t>(b) * m_channels_per_board + ich;
      if (idx >= fers.FERSrel_tsamp_us.size()) {
        break;
      }
      if (fers.FERSrel_tsamp_us[idx] >= 0) {
        board_time[b] = fers.FERSrel_tsamp_us[idx];
        break;
      }
    }
  }

  std::vector<double> present;
  present.reserve(m_n_boards);
  for (const double t : board_time) {
    if (!std::isnan(t)) {
      present.push_back(t);
    }
  }
  if (present.empty()) {
    return;
  }

  std::sort(present.begin(), present.end());
  const std::size_t mid = present.size() / 2;
  const double median =
      (present.size() % 2 == 0) ? 0.5 * (present[mid - 1] + present[mid]) : present[mid];

  for (unsigned int b = 0; b < m_n_boards; ++b) {
    if (!std::isnan(board_time[b])) {
      m_board_time_offset->Fill(b, board_time[b] - median);
    }
  }
}
