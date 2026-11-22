#include "HidraUtils.hh"
#include "Logger.hh"
#include "HidraXdcDecoder.hh"

#include <chrono>
#include <cstring>
#include <iterator>

namespace hidra {

using namespace std::chrono_literals;

struct ADCHeaderWord {
  uint32_t raw;
  uint8_t type() const { return (raw >> 24) & 0x7; }
  uint8_t geo() const { return (raw >> 27) & 0x1F; }
  uint8_t crate() const { return (raw >> 16) & 0xFF; }
  uint8_t cnt() const { return (raw >> 8) & 0x3F; }
};

struct ADCTrailerWord {
  uint32_t raw;
  uint32_t evt_cnt() const { return raw & 0xFFFFFF; }
  uint8_t type() const { return (raw >> 24) & 0x7; }
  uint8_t geo() const { return (raw >> 27) & 0x1F; }
};

struct V792Word {
  uint32_t raw;
  uint16_t value() const { return raw & 0xFFF; }
  uint8_t ov() const { return (raw >> 12) & 0x1; }
  uint8_t un() const { return (raw >> 13) & 0x1; }
  uint8_t channel() const { return (raw >> 16) & 0x1F; }
  uint8_t v792n_channel() const { return (raw >> 17) & 0xF; }
  uint8_t type() const { return (raw >> 24) & 0x7; }
  uint8_t geo() const { return (raw >> 27) & 0x1F; }
};

struct V775Word {
  uint32_t raw;
  uint16_t value() const { return raw & 0xFFF; }
  uint8_t ov() const { return (raw >> 12) & 0x1; }
  uint8_t un() const { return (raw >> 13) & 0x1; }
  uint8_t vd() const { return (raw >> 14) & 0x1; }
  uint8_t channel() const { return (raw >> 16) & 0x1F; }
  uint8_t v775n_channel() const { return (raw >> 17) & 0xF; }
  uint8_t type() const { return (raw >> 24) & 0x7; }
  uint8_t geo() const { return (raw >> 27) & 0x1F; }
};

HidraXdcDecoder::HidraXdcDecoder(std::map<int, std::string> vme_geo_map, uint8_t log_level)
    : m_vme_geo_map(std::move(vme_geo_map)) {
  EUDAQ_LOG_LEVEL(log_level);
  m_n_adc_channels = hidra::utils::computeMaxADCchannelFromGeoMap(m_vme_geo_map);
  m_n_tdc_channels = hidra::utils::computeMaxTDCchannelFromGeoMap(m_vme_geo_map);
  HIDRA_INFO("HidraXdcDecoder configured with {} ADC channels and {} TDC channels based on VME geo map", m_n_adc_channels, m_n_tdc_channels);
}

void HidraXdcDecoder::decode(const std::vector<uint8_t>& payload, HidraXdcEvent& event, std::uint64_t trigger_n) const {
  event = HidraXdcEvent{};
  event.trigger_n = trigger_n;

  const auto payload_size = payload.size();

  if (payload_size == 0) {
    HIDRA_ERROR("XDC payload is empty. Aborting");
    return;
  }

  if (payload_size % 4 != 0) {
    HIDRA_ERROR("XDC payload size {} is not a multiple of 4 bytes. Aborting", payload_size);
    return;
  }
  const std::size_t word_count = payload_size / 4;
  if (word_count == 0) {
    HIDRA_ERROR("XDC payload is too short or empty. Aborting");
    return;
  }

  std::vector<std::uint32_t> words(word_count);
  std::memcpy(words.data(), payload.data(), word_count * sizeof(std::uint32_t));

  std::vector<double> ADCvalues(m_n_adc_channels, -1);
  std::vector<double> ADCflags(m_n_adc_channels, -1);
  std::vector<double> TDCvalues(m_n_tdc_channels, -1);
  std::vector<double> TDCflags(m_n_tdc_channels, -1);
  std::vector<double> XDCTriggers(m_vme_geo_map.size(), -1);


  uint8_t expected_word_mask = 0b010; // 0b010 is header, 0b000 is channel, 0b100 is trailer
  uint8_t empty_datum_word_mask = 0b110; // Invalid datum

  for (auto it = words.begin(); it != words.end(); ++it) {

    auto word = *it;

    if ((word & 0xFE000000) == 0xFE000000) { // this is expected at the end of buffer
      continue;
    }

    else {

      expected_word_mask = 0b010;

      ADCHeaderWord W{word};
      const auto module_it = m_vme_geo_map.find(W.geo());

      if (W.type() != expected_word_mask) {
        if(W.type() == empty_datum_word_mask) {
          if(module_it == m_vme_geo_map.end()) {
            HIDRA_WARN("Event {}: Geo {}, unexpected XDC empty datum for unconfigured module. Skipping", event.trigger_n, W.geo());
          } else if(module_it->second != "V775N") {
            HIDRA_WARN("Event {}: Geo {}, Unexpected XDC word type: {:08X} - type {}. Should be Header Word type {}. Should be safe for TDC", event.trigger_n, W.geo(), word, W.type(), expected_word_mask);         
          }
          continue;
        } else {
          HIDRA_ERROR("Event {}: Geo {}. Unexpected XDC word type: {:08X} - type {}. Should be Header Word type {}. Aborting, may result in some ADC missing.", event.trigger_n, W.geo(), word, W.type(), expected_word_mask);
          return;
        }
      }

      int nchan = W.cnt();
      if (module_it == m_vme_geo_map.end()) {
        HIDRA_ERROR("No XDC module configured for crate {} geo {}. Aborting", W.crate(), W.geo());
        return;
      }
      const std::string& module_type = module_it->second;

      expected_word_mask = 0b000;
      for (int ichan = 0; ichan < nchan; ++ichan) {
        ++it;
        if (it == words.end()) {
          HIDRA_ERROR("No more words in the XDC data block, while payload data word is expected. Aborting");
          return;
        }
        word = *it;

        /// QDCs (aka ADCs) ///////////////////////
        if (module_type == "V792" || module_type == "V792N" || module_type == "V862") {
          V792Word V{word};
          if (V.type() != expected_word_mask) {
            HIDRA_ERROR("Geo {}. Unexpected (A)XDC word type: {:08X} - type {}. Should be Channel Word {}. Aborting", V.geo(), word, V.type(), expected_word_mask);
            return;
          }
          if (V.geo() != W.geo()) {
            HIDRA_ERROR("Mismatched geo in XDC words: header geo {} vs channel geo {}. Aborting", W.geo(), V.geo());
            return;
          }
          const int module_channel = module_type == "V792N" ? V.v792n_channel() : V.channel();
          int encoded_channel = hidra::utils::computeADCchannelFromGeo(m_vme_geo_map, V.geo(), module_channel);
          if (encoded_channel < 0 || encoded_channel >= m_n_adc_channels) {
            HIDRA_ERROR(
                "Event {}: Encoded ADC channel index {} is out of bounds (0, {}). Skipping", event.trigger_n, encoded_channel, m_n_adc_channels);
          } else {
            ADCvalues[encoded_channel] = V.value();
            ADCflags[encoded_channel] = (V.ov() << 1) | V.un();
          }

        } // if 792, 792N or 862
        ///// TDCs /////////////////////
        else if (module_type == "V775" || module_type == "V775N") {
          V775Word V{word};
          if (V.type() != expected_word_mask) {
            HIDRA_ERROR("Geo {}. Unexpected (T)XDC word type: {:08X} - type {}. Should be Channel Word {}. Aborting", V.geo(),  word, V.type(), expected_word_mask);
            return;
          }
          if (V.geo() != W.geo()) {
            HIDRA_ERROR("Mismatched geo in XDC words: header geo {} vs channel geo {}. Aborting", W.geo(), V.geo());
            return;
          }
          const int module_channel = module_type == "V775N" ? V.v775n_channel() : V.channel();
          int encoded_channel = hidra::utils::computeTDCchannelFromGeo(m_vme_geo_map, V.geo(), module_channel);
          if (encoded_channel < 0 || encoded_channel >= m_n_tdc_channels) {
            HIDRA_ERROR(
                "Event {}: Encoded TDC channel index {} is out of bounds (0, {}). Skipping", event.trigger_n, encoded_channel, m_n_tdc_channels);
          } else {
            TDCvalues[encoded_channel] = V.value();
            TDCflags[encoded_channel] = (V.ov() << 2) | (V.un() << 1) | V.vd();
          }
        } // if 775 or 775N
        else {
          HIDRA_ERROR("Unknown XDC module type {} for crate {} geo {}. Cannot decode channel word. Aborting",
                      module_type,
                      W.crate(),
                      W.geo());
          return;
        }

      } // loop over channels

      ++it;
      if (it == words.end()) {
        HIDRA_ERROR("No more words in the XDC data block, while trailer is expected. Aborting");
        return;
      }
      word = *it;

      expected_word_mask = 0b100;

      ADCTrailerWord T{word};

      if (T.type() != expected_word_mask) {
        HIDRA_ERROR("Event {}, Geo {}: Unexpected XDC word type: {:08X} -- type {}. Should be Trailer Word {}. Aborting", event.trigger_n, W.geo(), word, T.type(), expected_word_mask);
        return;
      }
      auto dist = std::distance(m_vme_geo_map.begin(), module_it);
      XDCTriggers[dist] = T.evt_cnt();

      if (T.evt_cnt() != trigger_n) {
        if(event.trigger_n <  T.evt_cnt()) {
          HIDRA_DEBUG("Event {}, Geo {}, Mismatched event count in XDC trailer vs trigger: {} vs {}. Aborting", event.trigger_n, W.geo(), T.evt_cnt(), trigger_n);
        } else {
          HIDRA_ERROR("Event {}, Geo {}, Mismatched event count in XDC trailer vs trigger: {} vs {}. Aborting", event.trigger_n, W.geo(), T.evt_cnt(), trigger_n);
        }
      }
    }
  }
  event.ADCvalues = std::move(ADCvalues);
  event.ADCflags = std::move(ADCflags);
  event.TDCvalues = std::move(TDCvalues);
  event.TDCflags = std::move(TDCflags);
  //event.XDCTriggers = std::move(XDCTriggers);
}

} // namespace hidra
