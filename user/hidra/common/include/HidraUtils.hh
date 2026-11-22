#ifndef HIDRA_UTILS_HH
#define HIDRA_UTILS_HH

#include <cstdint>
#include <fmt/core.h>

#include <limits>
#include <map>
#include <string>
#include <type_traits>
#include <vector>

#include <eudaq/Event.hh>
#include <eudaq/Logger.hh>

namespace hidra::utils {

struct HidraEventFlags {
  static constexpr std::uint32_t None = 0;

  static constexpr std::uint32_t FERSMisaligned = 1u << 1;
  static constexpr std::uint32_t FERSIncomplete = 1u << 2;
  // keep a few reserved for FERS 
  // keep a few reserved for XDC 
  // keep a few more 
  static constexpr std::uint32_t TimestampMismatch = 1u << 16;
  static constexpr std::uint32_t TimingCalibrationPending = 1u << 17;
  static constexpr std::uint32_t DuplicatedTrigger = 1u << 18;
  // max allowed is 24 to keep the flags within 32 bits (since Event class uses the first 8 bits)
};

inline void SetEventFlag(eudaq::Event& event, std::uint32_t flag) {
  event.SetFlagBit(flag << 8); // shift to keep the first 8 bits for Event class
}

inline void ClearEventFlag(eudaq::Event& event, std::uint32_t flag) {
  event.ClearFlagBit(flag << 8); // shift to keep the first 8 bits for Event class
}

inline bool HasEventFlag(const eudaq::Event& event, std::uint32_t flag) {
  return event.IsFlagBit(flag << 8); // shift to keep the first 8 bits for Event class
}

inline void SetEventFlagMask(eudaq::Event& event, std::uint32_t flags) {
  event.SetFlag(flags << 8); // shift to keep the first 8 bits for Event class
}

inline std::uint32_t GetEventFlagMask(const eudaq::Event& event) {
  return (event.GetFlag() & 0xFFFFFF00); // ignore the first 8 bits used by Event class
}


const std::map<std::string, std::map<std::string, int>> VMESpec{
  {"V792",  {{"nchannels", 32}, {"dummy", 0}, {"is_qdc", 1}}},
  {"V792N", {{"nchannels", 16}, {"dummy", 0}, {"is_qdc", 1}}},
  {"V862",  {{"nchannels", 32}, {"dummy", 0}, {"is_qdc", 1}}},
  {"V775",  {{"nchannels", 32}, {"dummy", 0}, {"is_qdc", 0}}},
  {"V775N", {{"nchannels", 16}, {"dummy", 0}, {"is_qdc", 0}}}
};

std::uint64_t getTimeus();
std::uint64_t getTimens();

std::string GetEventInfo(eudaq::Event* ev, int opt = 1);

std::map<std::string, std::string> parseConfigMap(const std::string& configstring);

// Expand ${VAR} references in a string from the process environment, so a path
// can be written machine-independently (e.g. ${REPO_ROOT}/...). Only the ${...}
// form is recognised; an unset or unterminated variable is a hard error.
std::string ExpandEnv(const std::string& value);

std::pair<long long, long long> ComputeMeanAndStdDev(const std::vector<long long>& values);


int computeADCchannelFromGeo(const std::map<int, std::string>& vme_geo_map, int geo, int channel);
int computeTDCchannelFromGeo(const std::map<int, std::string>& vme_geo_map, int geo, int channel);

int computeMaxADCchannelFromGeoMap(const std::map<int, std::string>& vme_geo_map);
int computeMaxTDCchannelFromGeoMap(const std::map<int, std::string>& vme_geo_map);

bool isXDCEmpty(const std::vector<double>& values);

template <typename... Args> std::string format(const std::string& fmt_str, Args&&... args) {
#if FMT_VERSION >= 80000
  return fmt::format(fmt::runtime(fmt_str), std::forward<Args>(args)...);
#else
  return fmt::format(fmt_str, std::forward<Args>(args)...);
#endif
}

inline std::string getTagOr(const eudaq::Event& ev, const std::string& tag, const std::string& default_value, bool doComplain = true) {

  if (!ev.HasTag(tag)) {
    if (doComplain) {
      EUDAQ_WARN("Returning default value for tag " + tag);
    }
    return default_value;
  }

  return ev.GetTag(tag);
}

template <typename T> T getTagOr(const eudaq::Event& ev, const std::string& tag, T default_value, bool doComplain = true) {
  static_assert(
      std::is_integral<T>::value,
      "getTagOr<T> only supports integral types");

  if (!ev.HasTag(tag)){
    if (doComplain) {
      EUDAQ_WARN("Returning default value for tag " + tag);
    }
    return default_value;
  }

  const std::string& s = ev.GetTag(tag);
  try {
    unsigned long long v = std::stoull(s);
    if (v > std::numeric_limits<T>::max()) {
      EUDAQ_WARN("Returning default value for tag " + tag);
      return default_value;
    }
    return static_cast<T>(v);
  } catch (...) {
    EUDAQ_WARN("Returning default value for tag " + tag);
    return default_value;
  }
}

inline bool is_little_endian(){
  std::uint16_t x = 0x0001;
  return *reinterpret_cast<unsigned char*>(&x) == 0x01;
}

  

} // namespace hidra::utils

#define HIDRA_DEBUG(fmt, ...) \
  do { \
    if (EUDAQ_IS_LOGGED("DEBUG")) { \
      EUDAQ_DEBUG(hidra::utils::format(fmt, ##__VA_ARGS__)); \
    } \
  } while (0)
#define HIDRA_INFO(fmt, ...) EUDAQ_INFO(hidra::utils::format(fmt, ##__VA_ARGS__))
#define HIDRA_WARN(fmt, ...) EUDAQ_WARN(hidra::utils::format(fmt, ##__VA_ARGS__))
#define HIDRA_ERROR(fmt, ...) EUDAQ_ERROR(hidra::utils::format(fmt, ##__VA_ARGS__))
#define HIDRA_THROW(fmt, ...) EUDAQ_THROW(hidra::utils::format(fmt, ##__VA_ARGS__))

#endif
