#pragma once
/**
 * HistogramPublisher exposes histograms over HTTP through ROOT THttpServer and JSROOT.
 *
 * Responsibilities:
 * - Create and manage THttpServer on the configured port.
 * - Register all histograms from the registry under "/Histograms".
 * - Drain the HTTP request queue from a dedicated pump thread because no
 *   TApplication event loop is running.
 * - Own the mutex shared by pump thread and filler thread, and expose it via
 *   Mutex() so FillerChain can lock around Fill().
 *
 * Non-responsibilities:
 * - It does not own the registry, which is passed by reference.
 * - It does not handle EUDAQ event logic or filler behavior.
 *
 * The mutex lives in this class because it protects histogram content used by
 * both ProcessRequests() reads and Fill() writes.
 */

#include "HistogramRegistry.hh"
#include "DurationAccumulator.hh"

#include <THttpServer.h>
#include <TNamed.h>

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

class HistogramPublisher {
public:
  explicit HistogramPublisher(HistogramRegistry& registry, int port, int pump_interval_ms = 20,
                              std::string http_output_dir = "");

  ~HistogramPublisher() { Stop(); }

  HistogramPublisher(const HistogramPublisher&) = delete;
  HistogramPublisher& operator=(const HistogramPublisher&) = delete;

  /**
   * Start THttpServer and the pump thread.
   *
   * This must be called after all fillers are constructed so the full set of
   * histograms is visible from the beginning.
   */
  void Start();

  /** Stop the pump thread and close THttpServer. Safe to call multiple times. */
  void Stop();

  /**
   * Mutex protecting histogram content.
   *
   * It must be held by all readers and writers of histogram bin data.
   */
  std::mutex& Mutex() { return m_mutex; }

  DurationAccumulator& ProcessRequestsTimer() { return m_process_requests; }

private:
  static constexpr const char* kFolder = "/Histograms";

  void PumpLoop();

  HistogramRegistry& m_registry;
  int m_port;
  int m_pump_interval_ms;
  // Absolute directory where the monitor saves its histogram snapshots
  // (HISTO_OUTPUT_PATTERN's folder). Empty = not exposed. Published over HTTP as
  // a dedicated TNamed (`m_output_dir_obj`) so the frontend's overlay can find
  // the reference files without per-deployment configuration.
  std::string m_http_output_dir;
  TNamed m_output_dir_obj;
  std::unique_ptr<THttpServer> m_server;
  std::mutex m_mutex;
  std::thread m_pump_thread;
  std::atomic<bool> m_pump_running{false};
  DurationAccumulator m_process_requests{"process_requests"};
};
