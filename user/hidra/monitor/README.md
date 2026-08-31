# HidraHttpMonitor

This folder contains the HiDRA EUDAQ `Monitor` implementation that publishes
online histograms over HTTP.

The monitor receives merged EUDAQ events from `HidraDataCollector` (a sampled
fraction, see `EVENT_PRESCALE`), decodes the detector payloads, fills a set of
ROOT histograms and exposes them through a ROOT `THttpServer` so they can be
browsed live with JSROOT at `http://<host>:<HTTP_PORT>`.

Run it with:

```sh
euCliMonitor -n HidraHttpMonitor
```

## Components

- `HidraHttpMonitor.*` - the EUDAQ monitor plugin: lifecycle hooks, event
  decoding and dispatch to the filler chain.
- `HistogramRegistry.*` - owns the histograms (one `TH1`-derived object per
  name), detached from any ROOT directory. Provides in-place `Reset()` and
  `SaveToFile()`.
- `HistogramPublisher.*` - owns the `THttpServer`, registers the histograms and
  drains the HTTP request queue from a dedicated pump thread. Owns the mutex
  that protects histogram content.
- `FillerChain.*` - ordered list of `IHistogramFiller`s, called under the
  histogram-content lock for every received event.
- `SummaryFiller.*` - run-level histograms (event count, events vs time).
- `XDCFiller.*` - XDC ADC/TDC histograms. The ADC views (`ADC_mean`,
  `ADC_inclusive`, `ADC_saturation`) are additionally filled split by trigger
  type into `*_physics` / `*_pedestal` copies (selected per event via
  `meta.isPhysics()` / `isPedestal()`); TDC stays inclusive only. The
  per-channel ADC distributions are one `TH2I` per trigger copy (`ADC_dist`,
  `ADC_dist_physics`, `ADC_dist_pedestal`; x = channel, y = ADC) — a single
  `TH2` instead of one `TH1` per channel keeps the registered-object count
  small so `THttpServer` stays responsive, and the frontend reads one channel
  via a server-side `ProjectionY` (issue #138). The TDC per-channel
  distributions (`TDC_channel_<N>`) are still one `TH1` each (not shown in the
  frontend). It also publishes the per-channel pedestal noise (one
  bin per channel) with two estimators: `ADC_noise_pedestal` = `IQR/1.349`
  (robust: equals 1σ for a Gaussian but insensitive to outlier tails) and
  `ADC_noise_std_pedestal` = standard deviation. Both are recomputed together
  from each channel's pedestal column of `ADC_dist_pedestal` (`ProjectionY`)
  every
  `PEDESTAL_NOISE_UPDATE_EVENTS` pedestal events (config key in the
  monitor `.ini`, default 200).
- `FERSFiller.*` - FERS histograms (`FERS_NBOARDS` boards × 64 channels). For
  both gains (`HG`, `LG`) it publishes: the per-channel mean (`FERS_HG_mean` /
  `FERS_LG_mean`, `TProfile` indexed by channel) split into `*_physics` /
  `*_pedestal` copies via the trigger mask; an inclusive physics distribution
  (`FERS_HG_inclusive_physics`, `FERS_LG_inclusive_physics`); and per-channel
  distributions as one `TH2I` per gain/trigger (`FERS_HG_dist_physics` /
  `_pedestal`, and `FERS_LG_…`), x = channel, y = ADC — only the
  physics/pedestal copies are kept (no "total"), binned `FERS_CHANNEL_NBINS` on
  the y axis over `[0, FERS_VALUE_MAX)`. A single `TH2` (instead of one `TH1I`
  per channel) keeps the registered-object count tiny so `THttpServer` stays
  responsive; the frontend reads one channel via a **server-side `ProjectionY`**
  (the publisher allows `ProjectionX`/`ProjectionY` on every `TH2`, keeping all
  other methods denied — see #138). It also fills
  the per-channel saturation fraction `FERS_HG_saturation` /
  `FERS_LG_saturation` (and `*_physics`; no pedestal copy — pedestal events don't
  saturate), a `TProfile` of a 0/1 indicator that is 1 when the value exceeds
  `FERS_SATURATION_THRESHOLD`. Per board (`TProfile` indexed by board) it also
  publishes the mean over the board's channels for each gain
  (`FERS_HG_board_mean` / `FERS_LG_board_mean`) and the fraction of events in
  which every HG channel of the board reads zero (`FERS_HG_board_allzero`, a
  board present but producing no signal). `FERS_board_time_offset`
  (`TProfile` indexed by board): each board's
  `rel_tsamp_us` minus the **median** over the boards present in the event, so a
  board out of time sync stands out. Finally, `FERS_trigger_id_consistency`
  (2-bin `TH1I`, one count per event): all boards of an event are triggered
  together, so every present channel should report the same `FERStrigger_id`;
  the event lands in `consistent` when they all agree, else in `mismatch` (a
  desynced board). The data is consumed from `HidraEvent::fers`
  regardless of which FERS decoder produced it (see `FERS_DECODER` below).
- `TrackerFiller.*` - tracker hit maps: one 2D occupancy histogram
  (`Tracker_station<i>`, X vs Y) per station, filled from `HidraEvent::tracker`
  (decoded by `HidraTrackerDecoder` from the tracker sub-event, detID 3). The
  number of stations and the per-station X/Y range and binning are configurable
  (`TRACKER_NSTATIONS`, `TRACKER_X_*`/`TRACKER_Y_*`, optional per-station
  `TRACKER_STATION<i>` range override — see Configuration). It also books a
  `Tracker_errors` counter (TH1): one labelled bin per station counting events
  where that station's X or Y is the producer's negative no-hit sentinel, plus a
  final `any` bin for the OR over all stations. Events without a tracker
  sub-event leave the tracker coordinates empty, so the filler is a no-op for
  them.
- `MetaFiller.*` - per-event metadata histograms (see "Event metadata").
- `ChannelSumFiller.*` - per-event SUM distributions over channel groups (see
  "Channel sums").

## Channel sums

`ChannelSumFiller` books 1D distributions of the per-event **sum** over a channel
group, for six groups, each in three trigger flavours (all / `_physics` /
`_pedestal`) — 18 histograms:

| Base name | Group | Source |
|-----------|-------|--------|
| `sum_PMT_S` | scintillator PMTs | ADC (V792) |
| `sum_PMT_C` | Cherenkov PMTs | ADC (V792) |
| `sum_SiPM_S_HG` / `sum_SiPM_S_LG` | scintillator SiPMs | FERS HG / LG |
| `sum_SiPM_C_HG` / `sum_SiPM_C_LG` | Cherenkov SiPMs | FERS HG / LG |

Raw values are summed (no pedestal subtraction); the physics-vs-pedestal split
is what shows the signal over the baseline. The S/C classification is **not**
known to the rest of the backend — the monitor parses it from the frontend
channel maps (`ADC_CHANNEL_MAP_JSON` / `SIPM_CHANNEL_MAP_JSON`): a PMT name
`M<digits>(S|C)` gives the type; element `[2]` of each SiPM entry gives it.
Unmapped channels (muon, Cherenkov detectors, MAXICC boards, …) are excluded. If
a map is missing/unset the corresponding groups book but stay empty. The sum
axis runs `[0, group_size × full_scale]` by default (`SUM_*_MAX` overrides focus
the range to better resolve the baseline).

## Event metadata

Besides the detector payloads, each event carries metadata (trigger mask, spill
number, timestamps, …) that is **not** in the binary blocks: the producers attach
it as EUDAQ tags. `HidraMetaDecoder` (in `../dc`) reads it from the EUDAQ event
into `HidraEvent::meta` (`HidraEventMeta`):

- event-level, from the merged event: run / event / trigger number, begin/end
  timestamps, `detector_mask`;
- `trigger_mask` and `spill_number`, from the **XDC sub-event** (detID 1 real / 6
  dry) where the producer sets them — the collector does not propagate them to the
  merged event. The raw trigger mask is stored; `isPhysics()` / `isPedestal()`
  derive physics vs pedestal (bit 0 / bit 1).

`MetaFiller` turns this into histograms:

| Histogram | Content |
|-----------|---------|
| `trigger_mask` | events per class: gate / physics / pedestal / both |
| `detectors_present` | one labelled bin per detID (0..7), from the detector mask |
| `events_per_spill` | events vs spill number (auto-extending axis) |
| `dt_between_events` | inter-event time (begin−begin), log-binned 1 µs..10 s |
| `spill_current` / `trigger_current` / `run_current` | latest value (single bin) |
| `trigger_mask_recent` | strip of the last `TRIGGER_MASK_STRIP_LENGTH` trigger classes (bin 1 = oldest, last bin = newest); stored as mask+1 so 0 = no data (1 gate, 2 physics, 3 pedestal, 4 both) |
| `events_between_pedestals` | distribution of the number of non-pedestal events between two consecutive pedestals; peaks at 10 for a 10-physics : 1-pedestal pattern |

The last two monitor the repeating trigger-mask pattern (10 physics then 1
pedestal). Both assume the monitor sees **consecutive** events, i.e.
`EUDAQ_DATACOL_SEND_MONITOR_FRACTION = 1`; with a fraction > 1 the sequence read is
not consecutive and the pattern view is misleading.

`dt_between_events` uses the merged-event begin timestamp, which is the
collector's wall-clock arrival time (`hidra::utils::getTimens()`, nanoseconds), so
it reflects software inter-arrival timing rather than hardware timestamps.

## HTTP server lifecycle (persistence across runs)

The HTTP server is **decoupled from the run lifecycle**: it is started once and
stays up across start/stop of runs, so the histograms of a finished run remain
browsable after the STOP. The long-lived state is bundled in `MonitorContext`
(registry, publisher/server, filler chain and decoders).

| Hook | Action |
|------|--------|
| `DoInitialise` | read init configuration (port, pump interval, prescale, output pattern). No server yet. |
| `DoConfigure` | first call: create `MonitorContext`, register fillers and **start the HTTP server** with empty histograms. Subsequent calls (reconfigure): keep the server, only rebuild the decoders. In both cases the histogram **contents** are cleared (see below). |
| `DoStartRun` | reset histograms and per-run state (event counter, telemetry, fillers' run-relative state such as the start-of-run time reference), then keep serving. |
| `DoStopRun` | log per-run telemetry and snapshot the histograms to a ROOT file. The server and the histograms stay alive and browsable. |
| `DoReset` | clear histogram contents and fillers' run-relative state. |
| `DoTerminate` | finalize the run (telemetry + ROOT snapshot) if it is still active, then destroy the context, which stops the HTTP server. |
| `DoReceive` | decode the sub-events and fill the histograms. |

Because the server stays up, histograms are **reset in place** (`TH1::Reset()`),
never re-created: the `THttpServer` keeps pointing at the same `TH1` objects, so
their registered pointers stay valid. This implies the histogram **objects** must
not be re-created at runtime (their binning may still grow in place — e.g.
`events_per_spill` uses an auto-extending axis); today the fillers book a fixed
set of histograms, so this holds.

### Histogram reset semantics

Two distinct things can be reset:

- `registry.Reset()` - zeroes the **histogram bin contents**.
- `chain.Reset()` - resets the fillers' **run-relative internal state** that is
  not stored in the histograms (e.g. `SummaryFiller`'s start-of-run timestamp,
  the time origin of the *events vs time* histogram).

They are applied as follows:

| Hook | `registry.Reset()` | `chain.Reset()` |
|------|:---:|:---:|
| `DoConfigure` | yes | no |
| `DoStartRun` | yes | yes |
| `DoReset` | yes | yes |

At `DoConfigure` only the histogram contents are cleared: the fillers'
run-relative state (e.g. the start-of-run time reference) is established at
`DoStartRun`, since a configure may happen well before the run actually starts.
No events are received between configure and start, so leaving the fillers'
state untouched at configure is safe.

## End-of-run ROOT snapshot

At `DoStopRun` — or at `DoTerminate` if the monitor is terminated while a run is
still active — the current histograms are written to a ROOT file via
`HistogramRegistry::SaveToFile()` (the histograms remain owned by the registry;
writing does not transfer ownership). The snapshot is taken at most once per run
(guarded by `MonitorContext::run_active`), so a STOP followed by a TERMINATE does
not save twice. This lets users keep and compare the histograms of past runs
offline. `SaveToFile()` creates the parent directory if it does not exist.

The output path is built from `HISTO_OUTPUT_PATTERN` with `eudaq::FileNamer`
(same mechanism as the data collector's `EUDAQ_FW_PATTERN`). Setting the pattern
to an empty string disables the snapshot.

## Configuration

Init configuration (`[Monitor.HidraHttpMonitor]` in the `.ini`), read in
`DoInitialise()`:

| Key | Default | Meaning |
|-----|---------|---------|
| `HTTP_PORT` | `9090` | TCP port of the HTTP server. |
| `PUMP_INTERVAL_MS` | `20` | Period of the pump thread draining the HTTP queue (clamped to >= 5 ms). |
| `EVENT_PRESCALE` | `1` | Process 1 event every N (>= 1) to reduce load. |
| `PEDESTAL_NOISE_UPDATE_EVENTS` | `200` | Recompute the per-channel pedestal noise (`ADC_noise_pedestal` / `ADC_noise_std_pedestal`) every N (>= 1) pedestal events. Lower = more responsive, higher = cheaper. |
| `HISTO_OUTPUT_PATTERN` | `out_data/monitor_run$6R_$12D$X` | `FileNamer` pattern for the end-of-run ROOT file. `$R` run number, `$D` timestamp, `$X` extension (`.root`). Empty disables saving. |

Run configuration (`[Monitor.HidraHttpMonitor]` in the `.conf`), read in
`DoConfigure()`:

| Key | Default | Meaning |
|-----|---------|---------|
| `VME_CRATE_1` | (empty) | `geo:module` map describing the XDC VME crate, passed to the XDC decoder. |
| `FERS_DECODER` | `real` | FERS decoder selection: `real` decodes the FERS payload; `random` ignores the input and generates fake per-channel data (**TEST ONLY**, to exercise the FERS histograms without real FERS data). |
| `FERS_NBOARDS` | `20` | Number of FERS boards (64 channels each); sizes the FERS histograms (only consumed on the first configure). |
| `FERS_VALUE_MAX` | `4096` | HG/LG ADC full scale (12-bit) — upper edge of the FERS distributions. |
| `FERS_CHANNEL_NBINS` | `1024` | Bins for the per-channel HG/LG distributions over `[0, FERS_VALUE_MAX)` (1024 → 4 ADC/bin). |
| `FERS_SATURATION_THRESHOLD` | `3800` | A channel is counted as saturated when its HG/LG value exceeds this (feeds `FERS_HG_saturation` / `FERS_LG_saturation`). Clamped to `[0, FERS_VALUE_MAX)`. |
| `FERS_PER_CHANNEL_DISTRIBUTIONS` | `1` | `1` books the per-channel HG/LG distributions (`FERS_{HG,LG}_channel_<N>_*`, needed for the frontend channel dropdown); `0` skips them to cut memory / startup / THttpServer load. Means, saturation, inclusive and per-board histograms are unaffected. |
| `TRACKER_NSTATIONS` | `3` | Number of tracker stations (planes); books one `Tracker_station<i>` 2D hit map (X vs Y) each. Sizing only consumed on the first configure. |
| `TRACKER_X_NBINS` / `TRACKER_Y_NBINS` | `110` | Bins on the X / Y axis of every station hit map. With the default `[0, 11]` cm range this gives 0.1 cm/bin. |
| `TRACKER_X_MIN` / `TRACKER_X_MAX` | `0` / `11` | Global X-axis range of the station hit maps, in cm (official tracker format). |
| `TRACKER_Y_MIN` / `TRACKER_Y_MAX` | `0` / `11` | Global Y-axis range of the station hit maps, in cm. |
| `TRACKER_STATION<i>` | (empty) | Optional per-station range override `xmin,xmax,ymin,ymax` (binning stays the global `TRACKER_*_NBINS`); e.g. `TRACKER_STATION0 = 0,50,0,50`. |
| `TRIGGER_MASK_STRIP_LENGTH` | `200` | Number of recent raw trigger-mask values shown in the `trigger_mask_recent` strip (>= 1). Sizing only consumed on the first configure. |
| `TRIGGER_PATTERN_GAP_MAX` | `30` | Upper edge (and bin count) of `events_between_pedestals`; gaps beyond it land in the overflow bar (>= 1). Sizing only consumed on the first configure. |
| `ADC_CHANNEL_MAP_JSON` | in-repo `adc_channels.json` | Path to the PMT channel map for the `sum_PMT_*` histograms; `${VAR}` is expanded from the environment. Defaults to `${REPO_ROOT}/user/hidra/monitor/frontend/hidra_frontend/mapping/adc_channels.json`, so no run-config entry is needed. Empty/unreadable leaves the PMT sums empty. |
| `SIPM_CHANNEL_MAP_JSON` | in-repo `sipm_channels.json` | Path to the SiPM channel map for the `sum_SiPM_*` histograms; `${VAR}` expanded. Defaults to the in-repo frontend map (as above). Empty/unreadable leaves the SiPM sums empty. |
| `SUM_NBINS` | `1024` | Bins on every channel-sum axis. Sizing only consumed on the first configure. |
| `SUM_PMT_MAX` | `0` | Upper edge of the PMT sum axes; `0` = auto (`group_size × full_scale`). |
| `SUM_SIPM_HG_MAX` / `SUM_SIPM_LG_MAX` | `3000000` | Upper edge of the SiPM HG / LG sum axes (auto `group_size × full_scale` ≈ 2.6M is too coarse to resolve the baseline). `0` = auto. |

## Threading & locking model

Threads:

- `T_ctrl` - RunControl thread running the lifecycle hooks.
- `T_recv` - DataReceiver thread calling `DoReceive()`.
- `T_pump` - publisher thread periodically flushing the HTTP queue.
- `T_http` - civetweb HTTP I/O thread (does not modify histograms).

There are two locks, and they protect two different things. They are always
taken in the order **`m_state_mutex` first, then `publisher.Mutex()`**, never the
reverse (taking them in the opposite order in two different threads could
deadlock).

### `publisher.Mutex()` — protects histogram *content*

This is a plain `std::mutex`, so it has only one mode: **exclusive**. Every
acquisition excludes every other; there is no "shared" variant.

It guards the bin contents and ROOT-internal state of the `TH1` objects. It is
held (exclusively) by:

- the pump thread while serialising the histograms to JSON (`ProcessRequests`);
- the fillers while updating histograms (`FillerChain::Fill`);
- the histogram resets (`registry.Reset()` / `chain.Reset()`);
- the end-of-run ROOT save (`registry.SaveToFile()`);
- the telemetry reset/log.

A plain exclusive mutex (rather than a shared one) is the right choice here. The
primary reason is that the pump thread *reads* the histograms (serialising them
to JSON) while a filler may *write* to them — an unambiguous read/write race that
must be serialised. On top of that, ROOT's `TH1` does not even guarantee safe
concurrent *read/read*, for two reasons:

- some apparently-const operations mutate internal state: e.g. statistics and
  serialisation may flush a histogram's internal fill buffer (`BufferEmpty()`),
  turning a "read" into a write;
- ROOT is not thread-safe by default — serialisation drives process-global
  machinery (the `TClass`/`TStreamerInfo` caches, the `TBuffer`, global error
  handling) that two threads can corrupt unless `ROOT::EnableThreadSafety()` is
  used.

Since every party is effectively a writer, a shared mode would buy nothing. (In
this monitor there is anyway a single histogram reader — the pump thread — so
read/read never actually occurs; the lock chiefly serialises pump-read vs
filler-write.)

### `m_state_mutex` — protects the context *structure*

This is a `std::shared_mutex`, and here the shared/unique distinction is
meaningful. It guards **the existence of `MonitorContext` and the identity of the
decoders** (the things `DoReceive` dereferences), *not* the histogram contents.
The rule is the classic readers/writer rule:

- **shared lock = "I only *use* the context as it is"**: I read the decoders,
  call into the filler chain, etc., without changing which objects exist. Several
  such users may proceed concurrently. Taken by `DoReceive`, `DoStartRun`,
  `DoStopRun`, `DoReset`.
- **unique lock = "I *change* the structure other users depend on"**: I create or
  destroy the context, or swap the decoders for new ones. This must exclude every
  shared user, otherwise a `DoReceive` could be decoding with a decoder that is
  being replaced or destroyed (data race / use-after-free). Taken by
  `DoConfigure` (creates the context / swaps decoders) and `DoTerminate`
  (destroys it).

Note that `DoStartRun`/`DoStopRun`/`DoReset` take `m_state_mutex` only **shared**
even though they reset or save histograms: they do not change the structure
(the context keeps existing, the decoders keep their identity), they only touch
histogram *content* — and that content is serialised by the *separate*
`publisher.Mutex()`. Holding `m_state_mutex` shared is just enough to guarantee
the context is not destroyed underneath them by a concurrent `DoTerminate`. This
is the whole point of the two-layer design: structural lifetime on one lock,
content access on the other.

A unique lock also waits for all in-flight shared holders to finish. That is what
makes `DoConfigure` (decoder swap) and `DoTerminate` (context destruction) safe
against a `DoReceive` that is still running.

### Per-hook locking summary

| Hook | `m_state_mutex` | `publisher.Mutex()` |
|------|-----------------|---------------------|
| `DoReceive` | shared | exclusive (inside `FillerChain::Fill`, decoding stays outside it) |
| `DoConfigure` | unique | exclusive, only for the `registry.Reset()` step (the decoder swap happens earlier under `m_state_mutex` alone) |
| `DoStartRun` | shared | exclusive (reset histograms + telemetry) |
| `DoStopRun` | shared | exclusive (log telemetry + ROOT save) |
| `DoReset` | shared | exclusive (reset histograms) |
| `DoTerminate` | unique | exclusive, briefly, inside `FinalizeRun()` (telemetry + ROOT save for a still-active run) — released **before** destroying the context. The teardown calls `publisher.Stop()`, which joins the pump thread, and the pump thread itself needs `publisher.Mutex()`, so the mutex must **not** be held during the join or it would deadlock |
