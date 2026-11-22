#include "HidraUtils.hh"

#include "HidraFersDecoder.hh"

#include <array>
#include <algorithm>
#include <cstring>
#include <limits>

namespace hidra {

#pragma pack(push, 1)
struct FERS_spect_64_packed { // no padding
  uint16_t marker;            // 0xAAAA
  uint16_t block_size;
  uint32_t data_qualifier;
  uint8_t board_id;
  double tstamp_us;
  double rel_tstamp_us;
  uint64_t tstamp_clk;
  uint64_t Tref_tstamp;
  uint64_t trigger_id;
  uint64_t chmask;
  uint64_t qdmask;
  std::array<uint16_t, 64> energyHG;
  std::array<uint16_t, 64> energyLG;
  std::array<uint32_t, 64> tstamp;
  std::array<uint16_t, 64> ToT;
};
#pragma pack(pop)


HidraFersDecoder::HidraFersDecoder(std::size_t max_boards)
    : m_max_boards(std::max<std::size_t>(max_boards, 1)) {}

void HidraFersDecoder::decode(const std::vector<std::uint8_t>& payload, HidraFersEvent& event) const {
  static_assert(sizeof(FERS_spect_64_packed) == 705);

  int board_packet_size = (int)sizeof(FERS_spect_64_packed);

  if (payload.size() % board_packet_size != 0) {
    HIDRA_ERROR("Unexpected FERS payload size {}. Should be a multiple of {}. Only first packet will attempt decoding",
                payload.size(),
                board_packet_size);
  }

  const std::size_t FERS_N_CHAN_MAX = 64 * m_max_boards;

  std::vector<double> FERStsamp_us(FERS_N_CHAN_MAX, -1);
  std::vector<double> FERSrel_tsamp_us(FERS_N_CHAN_MAX, -1);
  std::vector<double> FERStrigger_id(FERS_N_CHAN_MAX, -1);
  std::vector<double> FERSboard_id(FERS_N_CHAN_MAX, -1);
  std::vector<double> FERShg(FERS_N_CHAN_MAX, -1);
  std::vector<double> FERSlg(FERS_N_CHAN_MAX, -1);
  std::vector<double> FERStoa(FERS_N_CHAN_MAX, -1);
  std::vector<double> FERStot(FERS_N_CHAN_MAX, -1);

  int nboards = payload.size() / board_packet_size;
  for (int iboard = 0; iboard < nboards; ++iboard) {

    if (payload.size() % board_packet_size != 0 &&
        iboard > 0) { // to avoid troubles in case of wrong payload size, but still trying to decode first block
      break;
    }

    const auto* block_ptr = payload.data() + iboard * board_packet_size;
    FERS_spect_64_packed boardblock;
    std::memcpy(&boardblock, block_ptr, sizeof(FERS_spect_64_packed));

    if (boardblock.board_id >= m_max_boards) {
      HIDRA_ERROR("FERS block {} has invalid board_id {}. Skipping", iboard, boardblock.board_id);
      continue;
    }
    uint64_t allchan_mask = std::numeric_limits<uint64_t>::max();

    if (boardblock.chmask != allchan_mask) { // TODO: improve this check by counting number of bits in mask, to allow for some disabled channels
      HIDRA_WARN("FERS block {} has chmask {:016X}. Refusing to decode if less then 64 channels are enabled",
                 iboard,
                 boardblock.chmask);
      continue;
    }

    for (int ichan = 0; ichan < 64; ++ichan) {
      const std::size_t index = static_cast<std::size_t>(boardblock.board_id) * 64u + static_cast<std::size_t>(ichan);
      FERStsamp_us[index] = boardblock.tstamp_us;
      FERSrel_tsamp_us[index] = boardblock.rel_tstamp_us;
      FERStrigger_id[index] = static_cast<double>(boardblock.trigger_id);
      FERSboard_id[index] = static_cast<double>(boardblock.board_id);
      FERShg[index] = static_cast<double>(boardblock.energyHG[ichan]);
      FERSlg[index] = static_cast<double>(boardblock.energyLG[ichan]);
      FERStoa[index] = static_cast<double>(boardblock.tstamp[ichan]);
      FERStot[index] = static_cast<double>(boardblock.ToT[ichan]);
    }

  } // loop over board blocks

  event.FERStsamp_us = std::move(FERStsamp_us);
  event.FERSrel_tsamp_us = std::move(FERSrel_tsamp_us);
  event.FERStrigger_id = std::move(FERStrigger_id);
  event.FERSboard_id = std::move(FERSboard_id);
  event.FERShg = std::move(FERShg);
  event.FERSlg = std::move(FERSlg);
  event.FERStoa = std::move(FERStoa);
  event.FERStot = std::move(FERStot);
}

} // namespace hidra
