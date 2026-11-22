#include "HidraUtils.hh"

#include <eudaq/Exception.hh>

#include <chrono>
#include <cstdlib>
#include <sstream>
#include <utility>
#include <vector>
#include <cmath>
#include <map>
#include <string>

namespace hidra::utils {

std::string ExpandEnv(const std::string& value) {
  std::string out;
  out.reserve(value.size());
  for (std::size_t i = 0; i < value.size();) {
    if (value[i] == '$' && i + 1 < value.size() && value[i + 1] == '{') {
      const auto end = value.find('}', i + 2);
      if (end == std::string::npos) {
        EUDAQ_THROW("Unterminated ${...} in config value: " + value);
      }
      const std::string name = value.substr(i + 2, end - (i + 2));
      const char* env = std::getenv(name.c_str());
      if (env == nullptr) {
        EUDAQ_THROW("Environment variable '" + name + "' referenced in config value is not set: " + value);
      }
      out += env;
      i = end + 1;
    } else {
      out += value[i++];
    }
  }
  return out;
}

std::uint64_t getTimeus() {
  const auto now = std::chrono::system_clock::now();
  const auto us = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch());
  return static_cast<std::uint64_t>(us.count());
}

std::uint64_t getTimens() {
  const auto now = std::chrono::system_clock::now();
  const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch());
  return static_cast<std::uint64_t>(ns.count());
}

std::pair<long long, long long> ComputeMeanAndStdDev(const std::vector<long long> &values){
  // TOOD: implement this
  double mean = 0;
  double mean2 = 0;
  double stddev = 0;
  if (values.empty()) {
    return {mean, stddev};
  }
  for (const auto& v : values) {
    long long v_safe = v - values[0]; // to avoid overflow, we compute mean and stddev of (v - v0), which has the same stddev as v, and mean that is shifted by v0.
    mean += v_safe;
    mean2 += v_safe * v_safe;
  }
  mean /= values.size();
  mean2 /= values.size();
  stddev = std::sqrt(mean2 - mean * mean);
  return {(long long)(values[0] + mean), (long long)stddev};
}

bool isXDCEmpty(const std::vector<double>& values) {
  if (values.empty()) return true;
  return std::all_of(values.begin(), values.end(), [](double value) { return value < 0.0; });
}

std::map<std::string, std::string> parseConfigMap(const std::string& configstring) {

  std::map<std::string, std::string> out_map{};

  if (configstring.empty()) {
    return out_map;
  }

  std::stringstream ss(configstring);
  std::string token;

  while (std::getline(ss, token, ',')) {

    std::stringstream pairStream(token);

    std::string key;
    std::string val;

    if (std::getline(pairStream, key, ':') && std::getline(pairStream, val)) {
        out_map[key] = val;
      }
  }
  
  return out_map;
}

int computeADCchannelFromGeo(const std::map<int, std::string>& vme_geo_map, int geo, int channel) {
  int channel_index = channel;

  for (const auto& pair : vme_geo_map) {
    int module_geo = pair.first;
    const std::string& module_type = pair.second;
    if (module_geo == geo) {
      return channel_index;
    }

    const auto spec = hidra::utils::VMESpec.find(module_type);
    if (spec == hidra::utils::VMESpec.end()) {
      HIDRA_ERROR("Unknown VME module type {} for geo {}. Cannot compute ADC channel index. Returning channel {}", module_type, module_geo, channel);
      return channel;
    }

    const auto is_qdc_it = spec->second.find("is_qdc");
    if (is_qdc_it == spec->second.end() || is_qdc_it->second == 0) {
      continue; // if the module is not a QDC, it does not contribute to the ADC channel index
    }

    const auto nchannels = spec->second.find("nchannels");

    channel_index += nchannels->second;
  }

  HIDRA_ERROR("Geo {} not found in VME map. Cannot compute ADC channel index. Returning channel {}", geo, channel);
  return channel;
}

int computeTDCchannelFromGeo(const std::map<int, std::string>& vme_geo_map, int geo, int channel) {
  int channel_index = channel;

  for (const auto& pair : vme_geo_map) {
    int module_geo = pair.first;
    const std::string& module_type = pair.second;
    if (module_geo == geo) {
      return channel_index;
    }

    const auto spec = hidra::utils::VMESpec.find(module_type);
    if (spec == hidra::utils::VMESpec.end()) {
      HIDRA_ERROR("Unknown VME module type {} for geo {}. Cannot compute TDC channel index. Returning channel {}", module_type, module_geo, channel);
      return channel;
    }

    const auto is_qdc_it = spec->second.find("is_qdc");
    if (is_qdc_it == spec->second.end() || is_qdc_it->second == 1) {
      continue; // if the module is a QDC, it does not contribute to the TDC channel index
    }

    const auto nchannels = spec->second.find("nchannels");

    channel_index += nchannels->second;
  }

  HIDRA_ERROR("Geo {} not found in VME map. Cannot compute TDC channel index. Returning channel {}", geo, channel);
  return channel;
}

int computeMaxADCchannelFromGeoMap(const std::map<int, std::string>& vme_geo_map) {
  int max_channel = 0;

  if (vme_geo_map.empty()) {
    HIDRA_ERROR("VME geo map is empty. Cannot compute max ADC channel index. Returning 1500");
    return 1500;
  }

  for (const auto& pair : vme_geo_map) {
    const std::string& module_type = pair.second;
    const auto spec = hidra::utils::VMESpec.find(module_type);
    if (spec == hidra::utils::VMESpec.end()) {
      HIDRA_ERROR("Unknown VME module type {}. Cannot compute max ADC channel index. Returning 1500", module_type);
      return 1500;
    }

    const auto is_qdc_it = spec->second.find("is_qdc");
      if (is_qdc_it == spec->second.end() || is_qdc_it->second == 0) {
        continue; // if the module is not a QDC, it does not contribute to the ADC channel index
    }

    const auto nchannels = spec->second.find("nchannels");
    max_channel += nchannels->second;
  }

  return max_channel;
}

int computeMaxTDCchannelFromGeoMap(const std::map<int, std::string>& vme_geo_map) {
  int max_channel = 0;

  if (vme_geo_map.empty()) {
    HIDRA_ERROR("VME geo map is empty. Cannot compute max TDC channel index. Returning 1500");
    return 1500;
  }

  for (const auto& pair : vme_geo_map) {
    const std::string& module_type = pair.second;
    const auto spec = hidra::utils::VMESpec.find(module_type);
    if (spec == hidra::utils::VMESpec.end()) {
      HIDRA_ERROR("Unknown VME module type {}. Cannot compute max TDC channel index. Returning 1500", module_type);
      return 1500;
    }

    const auto is_qdc_it = spec->second.find("is_qdc");
      if (is_qdc_it == spec->second.end() || is_qdc_it->second == 1) {
        continue; // if the module is a QDC, it does not contribute to the TDC channel index
    }

    const auto nchannels = spec->second.find("nchannels");
    max_channel += nchannels->second;
  }

  return max_channel;
}

std::string GetEventInfo(eudaq::Event* ev, int opt) {

  std::string info = "Event Info:";

  if (opt == 1) { // default

    if (ev->IsBORE() || ev->IsEORE()) {
      info += "BORE runN " + std::to_string(ev->GetRunN());
    } else if (ev->IsEORE()) {
      info += "EORE runN " + std::to_string(ev->GetRunN());
    } else {
      info += " evtN " + std::to_string(ev->GetEventN());
      info += " trgN " + std::to_string(ev->GetTriggerN());
      info += " start/stop " + std::to_string(ev->GetTimestampBegin()) + "/" + std::to_string(ev->GetTimestampEnd());
      info += " nblk " + std::to_string(ev->GetNumBlock());
      info += " totB " + ev->GetTag("detectorDataSize");
    }
  }

  if (opt == 2) {
    info += " n_source " + ev->GetTag("N_SOURCES");
    info += " trig " + std::to_string(ev->GetTriggerN());
    info += " trgmask " + getTagOr(*ev, "triggerMask", std::string("N/A"), false);
    info += " ts " + std::to_string(ev->GetTimestampBegin());
    info += " -- (s/tg/ev/ts) ";
    for (int isub = 0; isub < ev->GetNumSubEvent(); isub++) {
      info += "(" + ev->GetSubEvent(isub)->GetTag("Producer") + "/" +
              std::to_string(ev->GetSubEvent(isub)->GetTriggerN()) + "/" +
              std::to_string(ev->GetSubEvent(isub)->GetEventN()) + "/" +
              std::to_string(ev->GetSubEvent(isub)->GetTimestampBegin()) + ")";
    }

  }

  else {
    info += "PLEASE SPECIFY A VALID OPTION";
  }

  return info;
}

} // namespace hidra::utils
