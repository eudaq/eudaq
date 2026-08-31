#pragma once

#include "HidraFersEvent.hh"
#include "IFersDecoder.hh"

#include <cstdint>
#include <random>
#include <vector>

namespace hidra {

/**
 * @brief Test-only FERS decoder: ignores the payload and generates random data.
 *
 * Fills @p event with random but plausible per-channel HG/LG values for
 * `n_boards × channels_per_board` channels. Board timestamps are correlated (a
 * common per-event base plus a small per-board jitter), with the occasional
 * board pushed a few µs out of sync so the board-time-compatibility plot has
 * something to show. Intended only to exercise the monitor → filler → frontend
 * chain when no real FERS data is available — never use in production.
 */
class HidraFersRandomDecoder : public IFersDecoder {
public:
  explicit HidraFersRandomDecoder(unsigned int n_boards = 20,
                                  unsigned int channels_per_board = 64,
                                  int value_max = 4096,
                                  std::uint32_t seed = 12345u);

  void decode(const std::vector<std::uint8_t>& payload, HidraFersEvent& event) const override;

private:
  unsigned int m_n_boards;
  unsigned int m_channels_per_board;
  int m_value_max;
  // decode() is logically const but advances the RNG / event counter, so both
  // are mutable. NOT thread-safe: a single instance must be used from one thread
  // at a time. In the monitor it is only called from DoReceive (the single
  // T_recv thread), so no synchronisation is needed; do not share one instance
  // across threads without external locking.
  mutable std::mt19937 m_rng;
  mutable std::uint64_t m_trigger_id{0};
};

} // namespace hidra
