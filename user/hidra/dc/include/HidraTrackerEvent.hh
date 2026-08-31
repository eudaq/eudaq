#pragma once

#include <cstdint>
#include <vector>

// Decoded tracker event: one (X, Y) hit coordinate per plane.
//
// The HidraTrackerProducer ships block 0 as a flat little-endian array of
// uint32 values, two per plane (x then y). `HidraTrackerDecoder` turns that
// into the parallel `X`/`Y` vectors below, indexed by plane number.
//
// NOTE: the tracker file format is still provisional (see
// user/hidra/tracker/README.md). When it is finalised, revisit the producer's
// block layout *and* this struct together.
struct HidraTrackerEvent {
  std::uint64_t trigger_n = 0;
  std::vector<double> X; // one entry per plane
  std::vector<double> Y; // one entry per plane
};
