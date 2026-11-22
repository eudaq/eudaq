#pragma once

#include "HidraEvent.hh"
#include "IHistogramFiller.hh"

#include <memory>
#include <mutex>
#include <vector>

/**
 * @brief Chain of histogram fillers
 *
 * FillerChain keeps a sequence of IHistogramFiller and calls them in order
 * under a single lock for each received event.
 *
 * Mutex is borrowed from HistogramPublisher::Mutex() — FillerChain is not owner.
 * Adding fillers after Start() is not thread-safe. All Add() must happen before
 * DoReceive() can be called.
 */
class FillerChain {
public:
  explicit FillerChain(std::mutex& mutex)
      : m_mutex(mutex) {}

  // Add a filler to the chain. Transfers ownership.
  // Must be called before Start().
  void Add(std::unique_ptr<IHistogramFiller> filler) { m_fillers.push_back(std::move(filler)); }

  // Dispatches the decoded event to all fillers under the shared mutex.
  // Called by HttpMonitor::DoReceive after decoding.
  void Fill(const HidraEvent& ev);

  // Calls Reset() on every filler. Does NOT acquire the mutex — the caller
  // (DoReset) must already hold it, consistent with how registry.Reset() is called.
  void Reset();

  // Expose the lock wait timer for telemetry logging.
  DurationAccumulator& LockWaitTimer() { return m_lock_wait; }
  
  const std::vector<std::unique_ptr<IHistogramFiller>>& Fillers() const { return m_fillers; }

private:
  std::mutex& m_mutex; // non owned
  std::vector<std::unique_ptr<IHistogramFiller>> m_fillers;
  DurationAccumulator m_lock_wait{"lock_wait"};
};
