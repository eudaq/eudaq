#ifndef HIDRA_TIMEALIGNMENT_CALIBRATOR_HH
#define HIDRA_TIMEALIGNMENT_CALIBRATOR_HH

#include <cstddef>
#include <map>
#include <vector>

namespace hidra::timealignment {

struct TriggerAlignmentConfig {
  int maxAbsTrgOffset = 100; // look for triggeroffsets in the range [-maxAbsTrgOffset, +maxAbsTrgOffset]
  int minMatches = 5;
  long long maxDeltaTRange = 100; // Validation: if max(dT) - min(dT) < maxDeltaTRange then the alignment is considered valid
  bool stopAtFirstValid = false; // if true, the scan over trigger offsets will stop as soon as a valid alignment is found. Otherwise, the best alignment will be found by scanning all offsets
};

struct TriggerAlignmentResult {
  size_t mapIndex = 0;
  bool valid = false;
  int bestOffset = 0;
  int nMatches = 0;
  long long minDeltaT = 0;
  long long maxDeltaT = 0;
  long long deltaTRange = 0;
  long long meanDeltaT = 0.0;
  double stddevDeltaT = 0.0;
};

std::vector<int> makeOffsetScanOrder(int maxAbsTrgOffset);

std::pair<double, double> getMeanStd(std::vector<long long> v, bool safeoffset);

TriggerAlignmentResult alignOneMapToReference(const std::map<long long, long long>& reference,
                                              const std::map<long long, long long>& other,
                                              size_t mapIndex,
                                              const TriggerAlignmentConfig& cfg);

std::vector<TriggerAlignmentResult>
alignTriggerMapsToFirstReference(const std::vector<std::map<long long, long long>>& maps,
                                 const TriggerAlignmentConfig& cfg = TriggerAlignmentConfig{});

std::vector<long long> triggerOffsetsFromAlignments(const std::vector<TriggerAlignmentResult>& alignments);
std::vector<long long> meanDeltaTsFromAlignments(const std::vector<TriggerAlignmentResult>& alignments);
std::vector<long long> deltaTRangesFromAlignments(const std::vector<TriggerAlignmentResult>& alignments);
bool validateTriggerAlignments(const std::vector<TriggerAlignmentResult>& alignments);

} // namespace hidra::timealignment

#endif
