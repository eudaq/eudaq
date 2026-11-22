#include "TimeAlignmentCalibrator.hh"
#include "HidraUtils.hh"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <numeric>

namespace hidra::timealignment {

std::vector<int> makeOffsetScanOrder(int maxAbsTrgOffset) {
  std::vector<int> offsets;

  // TODO to be enabled once understood
  /* 
  offsets.reserve(2 * maxAbsTrgOffset + 1);

  offsets.push_back(0);

  for (int k = 1; k <= maxAbsTrgOffset; ++k) {
    offsets.push_back(+k);
    offsets.push_back(-k);
  }
  */

  offsets.push_back(0);

  return offsets;

}

std::pair<double, double> getMeanStd(std::vector<long long> v, bool safeoffset = false){

  if (v.size() == 0){
    return {-999., -999.};
  }
  else if (v.size() == 1){
    return {(double)v[0], 0.};
  }

  long long commonOffset = 0;
  if (safeoffset){
    commonOffset = v[0];
    std::transform(v.begin(), v.end(), v.begin(),
               [commonOffset](long long value) {
                   return value - commonOffset;
               });
  }


  const double sum = std::accumulate(v.begin(), v.end(), 0.0);
  const double mean = sum / static_cast<double>(v.size());

  double sumResidual2 = 0.0;

  for (long long x : v) {
      const double dx = static_cast<double>(x) - mean;
      sumResidual2 += dx * dx;
  }

  const double stddev = std::sqrt(sumResidual2 / static_cast<double>(v.size()));

  return {safeoffset ? mean + static_cast<double>(commonOffset) : mean, stddev};
}


TriggerAlignmentResult alignOneMapToReference(const std::map<long long, long long>& reference,
                                              const std::map<long long, long long>& other,
                                              size_t mapIndex,
                                              const TriggerAlignmentConfig& cfg) {
  TriggerAlignmentResult best;
  best.mapIndex = mapIndex;

  long long bestRange = std::numeric_limits<long long>::max();
  int bestMatches = -1;

  const auto offsets = makeOffsetScanOrder(cfg.maxAbsTrgOffset);

  
  for (int offset : offsets) {

    std::vector<long long> deltaTs;
    deltaTs.reserve(std::min(reference.size(), other.size()));

    for (const auto& [trigRef, timeRef] : reference) {

      const long long trigOther = trigRef + offset;

      auto itOther = other.find(trigOther);
      if (itOther == other.end()) {
        continue;
      }

      const long long timeOther = itOther->second;
      const long long deltaT = timeOther - timeRef;

      deltaTs.push_back(deltaT); 
    }

    if (deltaTs.size() < cfg.minMatches) {
      continue;
    }

    auto minmax = std::minmax_element(deltaTs.begin(), deltaTs.end());

    const long long minDeltaT = *minmax.first;
    const long long maxDeltaT = *minmax.second;
    const long long range = maxDeltaT - minDeltaT;

    // deciding if this is the best alignment so far

    auto meanstd = getMeanStd(deltaTs, true);
    const int nMatches = static_cast<int>(deltaTs.size());
    const bool betterRange = range < bestRange;
    const bool sameRange = range == bestRange;
    
    const bool betterMatches = sameRange && nMatches > best.nMatches;
    const bool sameMatches = sameRange && nMatches == best.nMatches;
    

    if (betterRange /*|| betterMatches*/ ) {
      bestRange = range;
      bestMatches = nMatches;

      // reapplying the common offset to the best offset found, to have a more meaningful value to return
      best.bestOffset = offset;
      best.nMatches = nMatches;
      best.minDeltaT = minDeltaT ;
      best.maxDeltaT = maxDeltaT;
      best.deltaTRange = range;
      best.meanDeltaT = (long long)meanstd.first;
      best.stddevDeltaT = meanstd.second;
    }

    if (cfg.stopAtFirstValid && range <= cfg.maxDeltaTRange && bestMatches >= cfg.minMatches) {
      break;
    }
  } // loop over all offsets

  HIDRA_DEBUG("Alignment map index {}: bestMatches {}, cfg.minMatches {}, best.deltaTRange {}, cfg.maxDeltaTRange {} "
              "--> valid {}",
              mapIndex,
              bestMatches,
              cfg.minMatches,
              best.deltaTRange,
              cfg.maxDeltaTRange,
              bestMatches >= cfg.minMatches && best.deltaTRange <= cfg.maxDeltaTRange);

  if (bestMatches < cfg.minMatches) {
    best.valid = false;
    return best;
  }

  best.valid = best.deltaTRange <= cfg.maxDeltaTRange;

  return best;
}

std::vector<TriggerAlignmentResult>
alignTriggerMapsToFirstReference(const std::vector<std::map<long long, long long>>& maps,
                                 const TriggerAlignmentConfig& cfg) {
  if (maps.empty()) {
    throw std::runtime_error("Input vector of maps is empty");
  }

  const auto& reference = maps.front();

  std::vector<TriggerAlignmentResult> results;
  results.reserve(maps.size());

  TriggerAlignmentResult refResult;
  refResult.mapIndex = 0;
  refResult.valid = true;
  refResult.bestOffset = 0;
  refResult.nMatches = static_cast<int>(reference.size());
  refResult.minDeltaT = 0;
  refResult.maxDeltaT = 0;
  refResult.deltaTRange = 0;
  refResult.meanDeltaT = 0.0;
  refResult.stddevDeltaT = 0.0;

  results.push_back(refResult);

  for (size_t i = 1; i < maps.size(); ++i) {
    results.push_back(alignOneMapToReference(reference, maps[i], i, cfg));
  }

  return results;
}

std::vector<long long> triggerOffsetsFromAlignments(const std::vector<TriggerAlignmentResult>& alignments) {
  std::vector<long long> offsets;
  offsets.reserve(alignments.size());

  for (const auto& res : alignments) {
    offsets.push_back(res.bestOffset);
  }

  return offsets;
}
std::vector<long long> meanDeltaTsFromAlignments(const std::vector<TriggerAlignmentResult>& alignments) {
  std::vector<long long> meanDeltaTs;
  meanDeltaTs.reserve(alignments.size());

  for (const auto& res : alignments) {
    meanDeltaTs.push_back(res.meanDeltaT);
  }

  return meanDeltaTs;
}
std::vector<long long> deltaTRangesFromAlignments(const std::vector<TriggerAlignmentResult>& alignments) {
  std::vector<long long> deltaTRanges;
  deltaTRanges.reserve(alignments.size());

  for (const auto& res : alignments) {
    deltaTRanges.push_back(res.deltaTRange);
  }

  return deltaTRanges;
}

bool validateTriggerAlignments(const std::vector<TriggerAlignmentResult>& alignments) {
  for (const auto& res : alignments) {
    if (!res.valid) {
      return false;
    }
  }
  return true;
}

} // namespace hidra::timealignment
