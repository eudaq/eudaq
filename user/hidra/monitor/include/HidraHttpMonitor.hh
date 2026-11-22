#pragma once
/**
 * EUDAQ2 monitor plugin for HiDRA histogram publishing over HTTP.
 *
 * The monitor receives events in DoReceive(), decodes them, and forwards the
 * resulting HidraEvent to FillerChain, which updates the histogram registry.
 * Histograms are then exposed through HistogramPublisher via ROOT THttpServer.
 *
 * Event processing flow in DoReceive():
 * 1. Unpack the DataCollector sub-event envelope.
 * 2. Decode payload into HidraEvent outside the histogram-content lock.
 * 3. Call chain.Fill(HidraEvent) while holding the histogram-content lock.
 *
 * Threads:
 * - T_ctrl: RunControl thread executing lifecycle hooks.
 * - T_recv: DataReceiver thread invoking DoReceive() for each event.
 * - T_pump: HistogramPublisher internal thread periodically flushing HTTP queue.
 * - T_http: civetweb HTTP I/O thread, not modifying histograms.
 *
 * Locking model:
 * - m_state_mutex (std::shared_mutex) protects MonitorContext lifetime and the
 *   decoders that DoConfigure() may swap.
 * - publisher.Mutex() (std::mutex) protects histogram content.
 *
 * Lock order is always:
 * m_state_mutex (shared/unique) -> publisher.Mutex().
 * The reverse order must never be used.
 *
 * MonitorContext owns the long-lived monitoring infrastructure (HTTP server,
 * histogram registry and fillers). It is created once in DoConfigure() and lives
 * until DoTerminate(). The HTTP server therefore stays up across start/stop of
 * runs: at DoStartRun() the histograms are reset in place (the THttpServer keeps
 * pointing at the same TH1 objects) and at DoStopRun() they are snapshotted to a
 * ROOT file but remain browsable. This lets users inspect the data of a run once
 * it has finished.
 */

#include "FillerChain.hh"
#include "HistogramPublisher.hh"
#include "HistogramRegistry.hh"
#include "TrackerFiller.hh"
#include "ChannelSumFiller.hh"

#include <HidraXdcDecoder.hh>
#include <IFersDecoder.hh>
#include <HidraTrackerDecoder.hh>
#include <HidraMetaDecoder.hh>

#include <eudaq/Factory.hh>
#include <eudaq/Monitor.hh>

#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <atomic>

class HidraHttpMonitor : public eudaq::Monitor {
public:
  /** Construct the monitor plugin instance. */
  HidraHttpMonitor(const std::string& name, const std::string& runcontrol);

  /** EUDAQ lifecycle hook for initial setup before configuration. */
  void DoInitialise() override;
  /** EUDAQ lifecycle hook for creating the monitoring context (first call) and (re)building decoders. */
  void DoConfigure() override;
  /** EUDAQ lifecycle hook for resetting histograms and telemetry at the start of a run. */
  void DoStartRun() override;
  /** EUDAQ lifecycle hook for saving the run histograms while keeping the HTTP server alive. */
  void DoStopRun() override;
  /** EUDAQ lifecycle hook for resetting monitor state while keeping the process alive. */
  void DoReset() override;
  /** EUDAQ lifecycle hook that finalizes a still-active run (telemetry + ROOT snapshot) and shuts down resources. */
  void DoTerminate() override;
  /** Receive and process a single EUDAQ event. */
  void DoReceive(eudaq::EventSP ev) override;

  static const uint32_t m_id_factory;

private:
  /** Resolve the output path for the saved-histograms ROOT file of the current run. */
  std::string MakeHistoOutputFile() const;

  /**
   * Finalize the current run: log telemetry and snapshot the histograms to ROOT.
   *
   * Runs at most once per run (guarded by MonitorContext::run_active), so a STOP
   * followed by a TERMINATE saves exactly once. Called by both DoStopRun() and
   * DoTerminate(). The caller must hold m_state_mutex and guarantee m_ctx is set;
   * this method takes publisher.Mutex() itself.
   */
  void FinalizeRun();

  /**
   * Long-lived monitoring infrastructure.
   *
   * Created once in DoConfigure() and destroyed in DoTerminate(). It owns the
   * HTTP server (via HistogramPublisher), the histogram registry and the filler
   * chain, all of which survive individual runs. The decoders are part of the
   * context too but are run/config dependent: DoConfigure() may replace them on
   * a reconfigure while keeping the server and histograms.
   */
  struct MonitorContext {
    HistogramRegistry registry;
    HistogramPublisher publisher;
    FillerChain chain;

    /** Decoders carrying run/config-dependent state. Swapped by DoConfigure() under m_state_mutex. */
    hidra::HidraXdcDecoder xdc_decoder;
    /** Real or random FERS decoder, selected at configure time (FERS_DECODER). */
    std::unique_ptr<hidra::IFersDecoder> fers_decoder;
    /** FERS decoder for the MAXICC sub-event (det_id 4): its own boards, same FERS format. */
    std::unique_ptr<hidra::IFersDecoder> maxicc_decoder;

    /** Stateless tracker decoder (per-station X/Y coordinates from the tracker payload). */
    hidra::HidraTrackerDecoder tracker_decoder;

    /** Stateless decoder that extracts per-event metadata (trigger mask, spill, …) from the EUDAQ event. */
    hidra::HidraMetaDecoder meta_decoder;

    DurationAccumulator duration_xdc_decode{"decode_xdc"};
    DurationAccumulator duration_fers_decode{"decode_fers"};
    DurationAccumulator duration_maxicc_decode{"decode_maxicc"};
    DurationAccumulator duration_tracker_decode{"decode_tracker"};

    /**
     * FERS histogram sizing this context was built with. The FERS histograms are
     * sized once (here, at construction); these are kept so a reconfigure can
     * rebuild the FERS decoder to match, instead of re-reading possibly-changed
     * config values that no longer match the booked histograms.
     */
    int fers_nboards{20};
    int fers_value_max{4096};
    /** MAXICC board count this context's MAXICC histograms were sized with (kept like fers_nboards). */
    int maxicc_nboards{3};

    // Trigger-pattern histogram sizing this context was built with, kept so a
    // reconfigure can warn when a changed value is ignored (the histograms are
    // sized once, like the FERS ones above).
    int trigger_strip_length{200};
    int trigger_gap_max{30};

    int event_prescale{1};
    std::atomic<uint64_t> event_counter{0};

    /**
     * True while a run is active and not yet finalized. Guards FinalizeRun().
     * Read/written only under publisher.Mutex(); callers additionally hold
     * m_state_mutex (shared or unique), which keeps the context alive.
     */
    bool run_active{false};

    /** Build the context, register fillers and start the HTTP server. */
    MonitorContext(int port, int pump_interval_ms, int prescale, hidra::HidraXdcDecoder xdc_dec,
                   std::unique_ptr<hidra::IFersDecoder> fers_dec, std::unique_ptr<hidra::IFersDecoder> maxicc_dec,
                   int n_adc_channels, int noise_update_interval, int fers_nboards, int fers_value_max,
                   int fers_channel_nbins, int fers_saturation_threshold, bool fers_per_channel_distributions,
                   int maxicc_nboards, std::vector<TrackerStationConfig> tracker_stations, std::string http_output_dir,
                   int trigger_strip_length, int trigger_gap_max, ChannelSumConfig sum_config);
    ~MonitorContext() noexcept;
    /** Reset the per-run telemetry accumulators. Caller must hold publisher.Mutex(). */
    void ResetTelemetry();
    void LogTelemetry();
  };

  int m_port{9090};
  int m_pump_interval_ms{20}; /**< Pump interval in milliseconds (~50 Hz default). */
  int m_event_prescale{1}; /**< Process 1 event every N events (N>=1). */
  /** Recompute the per-channel pedestal noise every N pedestal events (N>=1). */
  int m_noise_update_interval{200};
  /** FileNamer pattern for the histograms saved at end-of-run. Empty disables saving. */
  std::string m_histo_output_pattern{"out_data/monitor_run$6R_$12D$X"};

  /** Protects MonitorContext lifetime and decoder swaps across lifecycle and receive paths. */
  mutable std::shared_mutex m_state_mutex;
  /** Monitoring context, created in DoConfigure() and reset in DoTerminate(). */
  std::unique_ptr<MonitorContext> m_ctx;
};
