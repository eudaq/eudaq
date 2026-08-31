#pragma once

#include "HidraEvent.hh"
#include "DurationAccumulator.hh"

#include <string>
#include <string_view>

/**
 * @brief Interface for histogram fillers.
 *
 * Implementations of this interface are responsible for filling histograms based on unpacked EUDAQ events.
 * Each filler typically focuses on a specific subsystem or "view" of the data (e.g., summary histograms, special
 * channels, correlations).
 *
 * Responsibilities:
 * - Registers its histograms in the constructor (via HistogramRegistry::Add) and
 *   keeps pointers to them as members for direct use in Fill().
 * - Does NOT handle any locking. Synchronization is the responsibility of FillerChain,
 *   which acquires the shared mutex before calling Fill().
 * - Does NOT own the event: it reads from it and returns.
 */
class IHistogramFiller {
public:
  virtual void Fill(const HidraEvent& ev) = 0;

  std::string_view Name() const { return m_name; }

  // Called by FillerChain::Reset() when the histogram registry is cleared (e.g. DoReset).
  // Override to reset any filler-internal state that is relative to run start (e.g. time references).
  // The caller (DoReset) already holds the histogram mutex.
  virtual void Reset() {}

  virtual ~IHistogramFiller() = default;

  DurationAccumulator& Timer() { return m_timer; }

protected:
  explicit IHistogramFiller(std::string name)
      : m_name(name),
        m_timer(std::move(name)) {}

private:
  std::string m_name;
  DurationAccumulator m_timer;
};
