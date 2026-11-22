#include "HidraTrackerDecoder.hh"
#include "HidraUtils.hh"

#include <cstring>

namespace hidra {

HidraTrackerDecoder::HidraTrackerDecoder() = default;

void HidraTrackerDecoder::decode(const std::vector<uint8_t>& payload, HidraTrackerEvent& event,
                                 std::uint64_t trigger_n) const {
  event = HidraTrackerEvent{};
  event.trigger_n = trigger_n;

  const auto payload_size = payload.size();

  if (payload_size == 0) {
    HIDRA_ERROR("Tracker payload is empty. Aborting");
    return;
  }

  if (payload_size % sizeof(double) != 0) {
    HIDRA_ERROR("Tracker payload size {} is not a multiple of {} bytes. Aborting", payload_size, sizeof(double));
    return;
  }
  const std::size_t value_count = payload_size / sizeof(double);

  if (value_count % kValuesPerPlane != 0) {
    HIDRA_ERROR("Tracker payload has {} values, not a whole number of (x, y) plane pairs. Aborting", value_count);
    return;
  }

  std::vector<double> values(value_count);
  std::memcpy(values.data(), payload.data(), value_count * sizeof(double));

  const std::size_t n_planes = value_count / kValuesPerPlane;
  // Coordinates are in cm. Every value is decoded and kept as-is, including the
  // producer's large-negative "no hit" sentinels (e.g. -5000/-6000): the ROOT
  // writer records them all; only the monitor hit-map filler skips the negative
  // ones so a "no hit" isn't drawn as a hit at -5000.
  std::vector<double> X(n_planes, -1);
  std::vector<double> Y(n_planes, -1);

  for (std::size_t plane = 0; plane < n_planes; ++plane) {
    X[plane] = values[plane * kValuesPerPlane + 0];
    Y[plane] = values[plane * kValuesPerPlane + 1];
  }

  event.X = std::move(X);
  event.Y = std::move(Y);
}

} // namespace hidra
