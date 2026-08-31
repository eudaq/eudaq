#include "HidraFersRandomDecoder.hh"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace hidra {

HidraFersRandomDecoder::HidraFersRandomDecoder(unsigned int n_boards,
                                               unsigned int channels_per_board,
                                               int value_max,
                                               std::uint32_t seed)
    : m_n_boards(n_boards == 0 ? 1 : n_boards),
      m_channels_per_board(channels_per_board == 0 ? 64 : channels_per_board),
      m_value_max(value_max < 1 ? 4096 : value_max),
      m_rng(seed) {}

void HidraFersRandomDecoder::decode(const std::vector<std::uint8_t>& /*payload*/, HidraFersEvent& event) const {
  const std::size_t n = static_cast<std::size_t>(m_n_boards) * m_channels_per_board;

  // Reset every per-channel vector to the -1 "no hit" sentinel, then fill all
  // channels (this synthetic source always provides every board/channel).
  event.FERStsamp_us.assign(n, -1.0);
  event.FERSrel_tsamp_us.assign(n, -1.0);
  event.FERStrigger_id.assign(n, -1.0);
  event.FERSboard_id.assign(n, -1.0);
  event.FERShg.assign(n, -1.0);
  event.FERSlg.assign(n, -1.0);
  event.FERStoa.assign(n, -1.0);
  event.FERStot.assign(n, -1.0);

  std::normal_distribution<double> stdnorm(0.0, 1.0);
  std::uniform_real_distribution<double> unit(0.0, 1.0);

  const double event_base_time = 1000.0 * unit(m_rng); // µs, common to all boards
  const double trig = static_cast<double>(m_trigger_id++);
  const double hi = static_cast<double>(m_value_max - 1);

  for (unsigned int b = 0; b < m_n_boards; ++b) {
    // Board timestamp: common base + small jitter; ~5% of boards are pushed a
    // few µs out of sync to exercise the board-time-compatibility plot.
    double board_time = event_base_time + 0.05 * stdnorm(m_rng);
    if (unit(m_rng) < 0.05) {
      board_time += 2.0 + 3.0 * unit(m_rng);
    }
    // ~2% of the time a board is "dead": still present (timestamp/board id set)
    // but every channel reads zero, to exercise FERS_HG_board_allzero.
    const bool dead = unit(m_rng) < 0.02;

    for (unsigned int ich = 0; ich < m_channels_per_board; ++ich) {
      const std::size_t idx = static_cast<std::size_t>(b) * m_channels_per_board + ich;

      // Per-channel pedestal mean varies smoothly so the (frontend) heatmap is
      // not flat, plus an occasional "signal" tail. A subset of channels ("hot")
      // fires more often and with larger HG amplitudes that cross the saturation
      // threshold (~3800), so FERS_HG_saturation shows structure. LG stays ~10×
      // smaller, so it effectively never saturates.
      const bool hot = (idx % 16u) < 2u; // ~12.5% of channels are "hot" in HG
      const double mean_hg = 150.0 + 40.0 * std::sin(0.05 * static_cast<double>(idx));
      double hg = mean_hg + 15.0 * stdnorm(m_rng);
      if (unit(m_rng) < (hot ? 0.30 : 0.05)) {
        hg = hot ? 3000.0 + 1500.0 * unit(m_rng)  // 3000..4500 → often > 3800 (saturates)
                 : 500.0 + 2500.0 * unit(m_rng);   // 500..3000
      }
      const double lg = 0.1 * hg + 5.0 * stdnorm(m_rng);

      event.FERShg[idx] = dead ? 0.0 : std::clamp(hg, 0.0, hi);
      event.FERSlg[idx] = dead ? 0.0 : std::clamp(lg, 0.0, hi);
      event.FERStsamp_us[idx] = event_base_time; // absolute (no per-board jitter)
      event.FERSrel_tsamp_us[idx] = board_time;  // relative (per-board jitter visible)
      event.FERStrigger_id[idx] = trig;
      event.FERSboard_id[idx] = static_cast<double>(b);
      event.FERStoa[idx] = 1000.0 * unit(m_rng);
      event.FERStot[idx] = 100.0 * unit(m_rng);
    }
  }
}

} // namespace hidra
