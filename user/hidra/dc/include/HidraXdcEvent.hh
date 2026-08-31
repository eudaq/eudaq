#pragma once

#include <cstdint>
#include <vector>

struct HidraXdcEvent {
  std::uint64_t trigger_n = 0;
  std::vector<double> ADCvalues;
  std::vector<double> ADCflags;
  std::vector<double> TDCvalues;
  std::vector<double> TDCflags;
  std::vector<double> XDCTriggers;
};
