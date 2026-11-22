#pragma once

#include "HidraXdcEvent.hh"

#include <vector>
#include <cstdint>
#include <map>
#include <string>

namespace hidra {

class HidraXdcDecoder {
public:
  HidraXdcDecoder(std::map<int, std::string> vme_geo_map, uint8_t log_level=1);
  void decode(const std::vector<uint8_t>& payload, HidraXdcEvent& event, std::uint64_t trigger_n) const;

  int NADCChannels() const { return m_n_adc_channels; }

private:
  std::map<int, std::string> m_vme_geo_map;
  int m_n_adc_channels;
  int m_n_tdc_channels;
};

} // namespace hidra
