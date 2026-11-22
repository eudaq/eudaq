# HiDRA monitor frontend

A web dashboard that polls the C++ `HidraHttpMonitor` backend
(which exposes histograms over HTTP via ROOT's `THttpServer`) and
renders them as interactive Plotly charts. The layout is declared
in `config.yaml`; custom behaviour is plug-in (Python `Panel`
subclasses).

```
                                  ┌───────────────────────────────┐
   C++ HidraHttpMonitor           │  Browser (Dash app)           │
   ┌──────────────────┐  POST     │  ┌──────────────────────────┐ │
   │ THttpServer      │  /multi.  │  │ dcc.Tabs, dcc.Graph(s),  │ │
   │ /Histograms/...  │◄──json───►│  │ poll-rate dropdown, ...  │ │
   │ port 9090        │  every    │  └──────────────────────────┘ │
   └──────────────────┘  >100 ms  │           ▲                   │
                                  │           │ figures           │
                                  │  ┌──────────────────────────┐ │
                                  │  │ Python callbacks         │ │
                                  │  │  ├ poll() — fetch+draw   │ │
                                  │  │  ├ tab switch            │ │
                                  │  │  └ overlay / controls    │ │
                                  │  └──────────────────────────┘ │
                                  │           port 8050           │
                                  └───────────────────────────────┘
```

## Run (one command)

```sh
cd user/hidra/monitor/frontend
./run.sh                            # open http://localhost:8050
```

`run.sh` creates `.venv/` on first launch (with
`--system-site-packages` so the venv inherits the system PyROOT),
installs `requirements.txt`, and serves the Dash app via **gunicorn**
— a production WSGI server, so no "development server" warning.
Dependencies are re-synced automatically whenever `requirements.txt`
changes (or pass `--reinstall` to force it).

If you sourced `user/hidra/misc/setup.sh` from the repo root, the
shell function `run_frontend` is a shortcut for the same thing:

```sh
source user/hidra/misc/setup.sh
run_frontend                        # ./run.sh
run_frontend --port 8060            # forwards args to run.sh
```

Common options:

```sh
./run.sh --port 8060                # override port
./run.sh --host 127.0.0.1           # bind locally only
./run.sh --config other.yaml        # override config file
./run.sh --workers 2                # more gunicorn workers (default 1)
./run.sh --reinstall                # force reinstall of requirements
```

Note on workers: Dash keeps per-process state (backend client,
overlay cache, perf counters) in memory, so multiple gunicorn workers
each have their own copy. The default of 1 is the safe choice for
this dashboard.

Note on browser sessions: some interactive state also lives per
process and is therefore **shared across all connected browsers**, not
isolated per session. In particular the channel-selector's current
channel (`ChannelSelectorPanel._selected`) and the rate panel's
history/EMA (`RatePanel._history`/`_ema`/`_prev_count`) are global: if
two people open the dashboard at once, picking a channel in one tab
changes it for everyone, and the rate sparkline mixes both clients'
poll cadences. This is acceptable for the intended single-screen
control-room use; making it per-session would require routing that
state through per-session `dcc.Store`s instead of instance attributes.

### Dev server (Flask, with `--debug` hot-reload)

If you specifically want the Flask dev server (e.g. for `--debug`
auto-reload), the legacy entry point still works inside the venv:

```sh
source .venv/bin/activate
python app.py --debug
```

You will see the usual "development server" warning — that's by
design; use `./run.sh` instead unless you actually need the
auto-reloader. The reloader also watches the active `config.yaml` (via
`extra_files`), so editing the declared layout restarts the app too — not
just Python edits.

The backend must already be running and reachable on the URL set in
`config.yaml` (default `http://localhost:9090`).

## Project layout

```
app.py                       # entry point: load config, build app, run server
config.yaml                  # YOU edit this to add tabs/histograms
requirements.txt
reference/                   # drop .root files here for overlay (gitignored)

hidra_frontend/
  config.py                  # parses config.yaml into typed dataclasses
  backend_client.py          # HTTP wrapper around /h.json and /multi.json
  figure_builder.py          # DecodedHist + Plotly = go.Figure
  theme.py                   # colors + base Plotly layout
  perf.py                    # phase timing helpers used in the poll path
  layout.py                  # top-level Dash layout (header, tabs, stores)
  overlay.py                 # reads reference .root files via uproot

  decoders/                  # JSON payload → DecodedHist
    base.py                  #   shared types (DecodedHist, Decoder)
    pure.py                  #   default: pure numpy, no PyROOT
    pyroot.py                #   fallback: TBufferJSON.ConvertFromJSON

  panels/                    # how a tab's content is built
    base.py                  #   Panel ABC — implement this for custom layouts
    histogram_grid.py        #   "histograms" panel type (the default)
    channel_selector.py      #   stub for future per-channel TH1Ds

  callbacks/                 # Dash callbacks (one file per concern)
    poll.py                  #   the main one: fetch + render
    controls.py              #   pause + poll rate
    overlay.py               #   reference file dropdown
```

## Configuration

Everything is in `config.yaml`. Reference:

```yaml
backend:
  url: http://localhost:9090
  request_timeout_s: 2.0          # HTTP timeout per request

decoder: pure                     # pure (default) or pyroot

polling:
  default_ms: 500                 # initial poll period
  floor_ms: 50                    # UI dropdown never goes below this
  choices_ms: [100, 200, 500, 1000, 2000, 5000]
  server_pump_ms_hint: 20         # shown in status bar, mirrors backend
                                  # PUMP_INTERVAL_MS in dry.ini/hidra.ini

overlay:
  enabled: true                   # set false to hide overlay controls
  search_dir: reference           # where to look for .root files
  default_file: null
  normalize: area                 # area = each distribution / its own sum of
                                  #   counts (both integrate to 1, compare
                                  #   shapes); profiles overlay raw. none = raw.

fers:                             # FERS detector properties (hardware, not
                                  # per-plot); used by all FERS views.
  channels_per_board: 64          # fixed by the FERS board

histogram_options:                # per-histogram display options, keyed by
                                  # histogram name; applies on every tab.
  dt_between_events:
    logx: true                    # logarithmic x-axis (needs positive edges)
    density: true                 # divide each bin by its width

ui_effects:
  shower_hour: 0                  # local hour (0-23) for the daily "cosmic
                                  # shower" animation; null disables the
                                  # automatic trigger (triple-click the
                                  # title still works either way)

tabs:
  - id: summary                   # url-safe slug, must be unique
    label: Summary                # human readable, shown on the tab
    panels:
      - type: histograms          # see "panel types" below
        cols: 2
        histograms: [event_count, events_vs_time]
```

## Common tasks

### Add a histogram to an existing tab

Edit `config.yaml`, find the tab, append the histogram name to the
`histograms:` list of one of its panels. Save and reload the browser
— no restart needed if you re-run `python app.py` after the edit.

### Per-histogram display options (log-x, density)

Some histograms need more than the default linear bar chart. Add an
entry under the top-level `histogram_options:` map, keyed by histogram
name — it applies wherever that histogram is shown:

```yaml
histogram_options:
  dt_between_events:
    logx: true      # logarithmic x-axis
    density: true   # divide each bin by its (linear) width
```

- `logx` switches the x-axis to a log scale. The bars are laid out for
  it (centred on each bin's geometric mean, width in decades), so a
  non-uniform/log binning renders correctly instead of being squashed
  against the left edge. Only applied when **every** bin edge is
  positive; otherwise it silently falls back to linear.
- `density` divides each bin content by its bin width, so a histogram
  with non-uniform bins shows a comparable height per unit x instead of
  raw per-bin counts. The y-axis is relabelled accordingly (e.g.
  `events / µs`).
- `board_hover: true` enriches the hover of a **per-channel** histogram
  (one whose x-axis title is `channel`, e.g. the FERS saturation
  profiles) to also show the board number and the channel within the
  board, on top of the global channel index — e.g. `ch 70 · board 1 ·
  ch 6`. The board size comes from the top-level `fers:` config section
  (`channels_per_board`), not repeated here — it's a property of the
  detector, not of the plot.

- `show_flow: true` draws ROOT's underflow/overflow bins as extra bars
  just outside the histogram range (only the non-empty side is shown),
  with an `underflow`/`overflow` hover. Use it where out-of-range entries
  are meaningful — e.g. `trigger_mask`, whose underflow holds the events
  with no decoded trigger mask, so the bars then sum to the title's entry
  count.

`logx`/`density`/`board_hover`/`show_flow` all default to `false`, so
histograms without an entry are unchanged.
The canonical example is `dt_between_events` (log-binned inter-event
time from the backend's `MetaFiller`).

### Axis titles and units

Axis titles come straight from the ROOT histogram (the
`"title;x-title;y-title"` string the backend passes when booking it) —
the frontend reads `fXaxis.fTitle` / `fYaxis.fTitle` from the payload
and renders them, translating the common ROOT TLatex tokens to Unicode
(`#mu`→`µ`, `#Delta`→`Δ`, …). So to add/fix a unit, set it on the
**backend** histogram title; nothing is hard-coded in the frontend.
Histograms booked without axis titles (most ADC/TDC ones) simply show
none.

For **1D histograms** (`TH1*`) the plot title also gets the live total
entry count appended — `(entries: N)`, where `N` is ROOT's `fEntries`
(every `Fill`, including over/underflow). `TProfile` and 2D histograms
are excluded (their "entries" mean per-bin samples / 2D counts), so their
title is left unchanged. In a **distribution overlay** (the channel
selector's total/physics/pedestal step lines, ADC and FERS) the count
goes on each series' legend entry instead — e.g. `physics  (15,000)` —
so the three counts are visible at once; the per-channel comparison
overlay (e.g. the noise estimators) keeps plain labels.

### Add a new tab

Append an entry under `tabs:` in `config.yaml`:

```yaml
  - id: my_tab
    label: My tab
    panels:
      - type: histograms
        cols: 2
        histograms: [some_hist, other_hist]
```

### Built-in panel types

- `histograms` — fixed list of histograms in an N-column grid.
  Params: `histograms: [name1, name2, ...]`, `cols: 2` (default).
- `metric` — show each histogram's content as a single big number
  ("scorecard"). Good for counter-like histograms (e.g.
  `event_count`, a TH1I with one bin). The displayed number is the
  sum of all in-range bins. Params: `histograms: [name1, ...]`.
- `channel_selector` — show one channel's distribution at a time with a
  dropdown to switch channel. Two data-source modes:

  - **Projection mode** (`projection_templates:`) — used by ADC and FERS.
    The per-channel data lives in one `TH2` per trigger/gain copy
    (`ADC_dist*`, `FERS_HG_dist_*`, …; x = channel, y = value) and a single
    channel is fetched as a **server-side `ProjectionY` slice** instead of
    transferring the whole 2D histogram (issue #138). Give the TH2 *base*
    names (e.g. `["ADC_dist"]`, or `["FERS_HG_dist", "FERS_LG_dist"]` for two
    stacked plots) plus `split_suffixes:` (`["", "_physics", "_pedestal"]`
    for ADC; `["_physics", "_pedestal"]` for FERS). The channel count is read
    once from the TH2's x axis (`GetNbinsX`), so it always matches the current
    VME geo map without any config to keep in sync. **Projection mode does not
    auto-refresh**: the server-side `exe.json ProjectionY` is an interpreted
    call whose repeated use grows the backend's memory over long runs (issue
    #153), so a channel is fetched only on **channel change / tab (re)open / the
    ↻ refresh button**, and the last figures are kept until then.
  - **Name mode** (`template:` / `templates:`) — for a backend that exposes
    one histogram per channel (`<name>_<N>`). The channel list is
    auto-discovered from the backend; `discover_suffix:` matches
    `template + suffix` when there is no bare per-channel histogram.

  Common params: `show_trigger_split: true` overlays the selected channel's
  configured `split_suffixes` series in one plot (click a legend entry to
  hide/show a series; missing ones are skipped); `templates:`/
  `projection_templates:` as a list drives **several** stacked plots of the
  *same* channel from one dropdown. `fixed_channel: <N>` pins the panel to one
  channel and hides the dropdown (a dedicated single-channel view, e.g. the
  muon counter on ADC channel 193: `projection_templates: ["ADC_dist"]`,
  `split_suffixes: ["", "_pedestal"]` to overlay total + pedestal); `title:`
  overrides the auto plot title for such a panel. `fixed_channels: [a, b, …]`
  instead shows **one plot per channel** side by side (titles taken from the
  calo mapping, e.g. `Cher1 · ch 194`); `cols:` sets how many per row (default
  1 = stacked), so `cols: 3` puts the three Cherenkov chambers on one row.
  `threshold: <ADC>` adds a
  dashed vertical marker at that ADC value and writes the **fraction of entries
  above it** into the title; `threshold_series:` picks which copy the fraction
  is computed on (e.g. `"_physics"` to count only physics events, even when that
  copy is not one of the displayed `split_suffixes`). Dropdown labels are enriched with the calo
  module name (e.g. `ch 5 · M105S`); `board_labels: true` instead labels as
  `board B · ch L` (FERS). The selected channel updates the plot on the next
  poll tick.
  Dropdown labels: by default the channel number is enriched with the calo
  module name from the ADC mapping (e.g. `ch 5 · M105S`); set
  `board_labels: true` to instead label as `board B · ch L` (board size from
  the top-level `fers:` config section) and skip the calo mapping — used by
  FERS, whose channel indices would otherwise pick up unrelated ADC module
  names.
  Each plot's hover toolbar carries a **segmented rebin control**
  (`×1`/`×2`/`×4`/`×8`, default `×1`): it merges that many adjacent bins in the
  frontend (no backend change), per slot and independent across slots. The
  figure is rebuilt from the coarser bins, so the y-axis adapts and the x range
  stays put; in projection mode a change triggers one refetch (like switching
  channel).

- `detector` — a 2D calorimeter map: one cell per module at its (row,
  column) position, coloured by a per-channel value. Emits two heatmaps
  (S and C PMTs). The value is read from a `TProfile` (the bin mean) or a
  plain per-channel `TH1` (the bin content, e.g. `ADC_noise_pedestal`).
  Params: `histogram: ADC_mean` (default); `label:` colorbar/hover label;
  `title_tag:` extra word in the title to disambiguate two maps of the
  same histogram family in one tab (e.g. `noise`); `height:`; `link_tab:`
  makes modules clickable, opening that tab's `channel_selector` on the
  clicked channel.

- `fers_board` — the FERS counterpart of `detector`, with a **synthetic**
  geometry (no calo mapping): it draws **one heatmap per board**, tiled on a
  `columns:`-wide grid (default 2 boards per row). Each board shows its
  `channels_per_board` channels (default 64) in a `board_cell_rows` ×
  `board_cell_cols` block (default 16 × 4). All boards share a common colour
  scale (global min/max over the channels with data) so they are directly
  comparable. The number of boards is `n_boards:` (default 20, matching the
  backend `FERS_NBOARDS`; one graph slot per board is created at layout time,
  so set this if the backend uses a different count). The value comes from a
  `TProfile` (bin mean) or a per-channel `TH1` (bin content, e.g. a
  saturation fraction). Params: `histogram:`; `label:`/`title_tag:`;
  `height:` (**per board**); `columns:` (boards per row); `link_tab:` opens
  that tab's `channel_selector` of the **same gain** (HG/LG, parsed from the
  histogram name) on the clicked channel. With `mode_toggle: true` the panel
  shows a **physics/pedestal** radio and appends the active suffix to
  `histogram` (so `histogram: FERS_HG_mean` → `_physics` / `_pedestal`),
  fetching **only the shown variant** from the backend; without it,
  `histogram` is used verbatim (e.g. `FERS_HG_saturation_physics`).

- `maxicc_detector` — geometric ADC maps of the MAXICC crystal calorimeter
  (the 3 new FERS boards). One gain per panel, drawn as **three heatmaps side
  by side**, one per readout plane (front 15 µm / rear 15 µm / rear 50 µm): each
  crystal sits at its `(col, row)` position (row 0 at the top), coloured by its
  mean ADC, same three stacked heatmaps as the SiPM map. The channel → position
  map is `mapping/maxicc_channels.json` (board/channel → `[layer, col, row]`).
  The MAXICC boards are read out after the other FERS, so the global histogram
  index is `(board_offset + board) * 64 + ch`. Params: `histogram:`
  (`FERS_HG_mean` / `FERS_LG_mean`); `label:`; `mode_toggle: true` (physics/
  pedestal radio, like `fers_board`); `board_offset:` (default 20); `height:`
  (per map). Until the DAQ reads 23 boards (`FERS_NBOARDS=23`) those channels are
  not in the histograms yet, so every cell shows grey "no data" — the detector
  outline is still drawn. Two tabs (`maxicc_hg`, `maxicc_lg`) host the two gains.

- `tracker` — one **2D hit map** (`go.Heatmap`) per tracker station, from
  the backend `Tracker_station<i>` `TH2`s, plus (by default) two **projection
  overlays** below them: the X distribution and the Y distribution, each with
  one step-line per station superimposed (legend toggles them). The projections
  are computed in the panel as the `TH2` projections (sum over the other axis),
  so they need no extra backend histograms; the overlay assumes a shared axis
  range across stations. Like `detector` it reads the raw ROOT `TH2` buffers
  directly (the 1D figure builder doesn't handle 2D), so it never goes through
  `to_figure`. Params: `histograms: [...]` (one heatmap each, in order; keep in
  sync with the backend `TRACKER_NSTATIONS`); `cols:` (heatmaps per row, default
  2); `height:` (per map); `equal_aspect: true` locks a 1:1 x/y pixel ratio;
  `projections:` (default `true`) toggles the X/Y overlay row;
  `projection_height:` its height. The X/Y range and binning are fixed by the
  backend (`TRACKER_X_*`/`TRACKER_Y_*`), not here.

- `overlay` — a fixed list of histograms superimposed in one graph (one
  line trace each, legend toggles them). Params: `histograms: [...]`;
  `labels: [...]` (optional); `title:` (optional); `per_channel: true`
  when x is a channel index (gives the "ch N" hover and a marker per
  point). Used for the pedestal-noise estimator comparison; for
  overlaying the total/physics/pedestal of one *selected* channel use
  `channel_selector` with `show_trigger_split` instead.

The per-channel **noise** is computed on the backend, not the frontend.
Two estimators are published, one value per channel: `ADC_noise_pedestal`
= `IQR/1.349` (robust to outliers; on a Gaussian it equals 1σ) and
`ADC_noise_std_pedestal` = standard deviation (outlier-sensitive). The
"ADC noise" tab overlays both per channel (`overlay` panel) and maps the
robust one (`detector` panel). The backend refresh cadence is set by
`PEDESTAL_NOISE_UPDATE_EVENTS` in the monitor `.ini`.

### Add a custom panel (custom layout / widgets)

When `histograms` isn't enough — e.g. you want a slider, a multi-row
custom layout, or special interactions — write a Panel subclass.

1. Create `hidra_frontend/panels/my_panel.py`:

   ```python
   from dash import dcc, html
   from .base import Panel

   class MyPanel(Panel):
       def histogram_names(self):
           # Histograms this panel needs on each poll. Can come
           # from self.params (read from config.yaml).
           return self.params.get("histograms", [])

       def layout(self):
           # Build the Dash component tree. Use IDs of the form
           # {"type": "panel-graph", "panel": self.panel_id, "index": i}
           # for any dcc.Graph slot you want the poll callback to fill.
           return html.Div([...])

       def render(self, figs, client_state):
           # Called once per poll. Return one Plotly figure per
           # panel-graph slot, in the same order as layout().
           return [figs.get(n) for n in self.histogram_names()]

       def register_callbacks(self, app):
           # OPTIONAL — only if your panel has its own widgets
           # (sliders, dropdowns, etc.) that need callbacks.
           pass
   ```

2. Register the panel type in `hidra_frontend/panels/__init__.py`:

   ```python
   from .my_panel import MyPanel
   PANEL_TYPES["my_panel"] = MyPanel
   ```

3. Reference it from `config.yaml`:

   ```yaml
   - id: my_tab
     label: My tab
     panels:
       - type: my_panel
         histograms: [foo, bar]
   ```

### Use the overlay (reference histograms from .root files)

Drop one or more `*.root` files into `reference/` (a snapshot of a
previous run works — e.g. `run/out_data/monitor_run*.root`). Reload the
dashboard, click **Refresh files**, pick a file from the dropdown, and a
dashed reference trace appears on top of the live histogram.

The reference trace is drawn on every shown **1D** plot (TH1 *and*
TProfile) whose name exists in the file. The TH2 channel slices (the
projection channel selector) never get one — there is no single reference
curve for a 2D distribution.

`normalize` controls the scaling:

- `area` (the default) divides each **genuine distribution** (1D spectra:
  ADC/TDC/FERS inclusive, `dt_between_events`, `trigger_mask`, …) by its
  own sum of counts, so the live and reference both integrate to 1 and the
  **shapes** compare regardless of how much data each run collected (the
  y-axis then reads "fraction of entries"). **Profiles** (means,
  saturation, board) and **per-channel value plots** (e.g. the pedestal
  noise, whose x-axis is "channel") always overlay in their **raw** units
  — normalizing a mean would be meaningless.
- `none` keeps raw counts everywhere (only meaningful when the two runs
  have comparable statistics).

### Change colors / spacing

Edit `hidra_frontend/theme.py`. All colors and the base Plotly
layout used by every figure are defined there.

### Change polling rate at runtime

Use the dropdown in the top bar. Change the available choices via
`polling.choices_ms` in `config.yaml`.

### Switch decoder

`decoder: pyroot` in `config.yaml` falls back to ROOT's
`TBufferJSON.ConvertFromJSON` for decoding payloads. Useful if the
pure decoder doesn't understand a new histogram type yet.

## Troubleshooting

| Symptom | Likely cause | Fix |
|---|---|---|
| Status bar: "cannot reach backend" | Backend not running, wrong URL, firewall | `curl http://localhost:9090/h.json` and check `backend.url` in `config.yaml`. |
| A graph shows "missing on server" | Histogram name in `config.yaml` doesn't match what the backend exposes | `curl http://localhost:9090/h.json` to see the real names. |
| "decode error" annotation | Decoder raised an exception | Look at the python log; try `decoder: pyroot` as a fallback. |
| Dashboard is sluggish at 100 ms polling | Too many large histograms per poll | Increase the polling period; or check the perf summary in the log (printed every 20 polls) — usually `poll.to_figure_one`, or `poll.panel_render > <PanelClass>` for heatmap panels (`detector`, `fers_board`). |
| Browser tab flashes white when switching | You're on an older version without `placeholder_figure` | Pull the latest, or check that `theme.placeholder_figure(name)` is used as the initial `dcc.Graph.figure`. |
| `InvalidCallbackReturnValue` in the log | Race between tab switch and a still-in-flight poll | Already guarded in `poll.py` (`expected vs len(figures_out)`). If it reappears, leave the dashboard up for 1 s and the next poll fixes it. |
| Overlay dropdown is empty | `reference/` doesn't exist or is empty | Create the directory and drop a `.root` file in it. Then click **Refresh files**. |

## Performance notes

The poll callback is the only hot path. It does, in order:

1. `poll.fetch_multi` — one `POST /multi.json` batched request. Cost:
   ~15 ms for 6 histograms, dominated by the backend's
   `PUMP_INTERVAL_MS` (default 20 ms — that's the floor). Only the
   histograms the active tab actually shows are requested.
2. `poll.to_figure_all` — for the histograms some panel renders from the
   pre-built `figs` dict (grid, channel_selector non-split): decode
   (`decode.live`, ~0.2 ms/hist pure / ~2 ms pyroot), Plotly trace build
   (`trace_build.*`, ~1-3 ms/hist) and overlay lookup (~0.1 ms/hist,
   cached).
3. `poll.panel_render` — each panel's `render()`, **timed per panel
   class** (nested under `poll.panel_render`). Panels that build their own
   figures from the raw payload live here, *not* in `to_figure_one`:
   `detector` and `fers_board` (one `go.Heatmap` per board — a FERS tab
   with 20 boards builds 20 heatmaps/poll, so this is often the dominant
   phase on those tabs) and `channel_selector` in split mode (its
   `decode.overlay` shows up nested here).
4. `poll.apply_controls` — log-y / reset-zoom bookkeeping, ~0.1 ms.

Everything the poll does is inside `poll.total`, so the children always
sum to it. Total at the default summary config (6 histograms, no overlay)
is ~70 ms per poll; the FERS board tabs add the per-board heatmap cost
under `poll.panel_render`. With the default polling rate of 500 ms there's
plenty of headroom.

The phase timer prints a summary tree every 20 polls to the python log:

```
=== perf summary (window = last 20 polls) ===
  poll.total                total= 1450.0 ms  n=20  mean= 72.500 ms
  ├─ poll.to_figure_all     total= 1100.0 ms  n=20  mean= 55.000 ms  ( 75.9%)
  ├─ poll.panel_render      total=  250.0 ms  n=20  mean= 12.500 ms  ( 17.2%)
  │  └─ FERSBoardPanel      total=  240.0 ms  n=20  mean= 12.000 ms  ( 96.0%)
  ├─ poll.fetch_multi       total=  300.0 ms  n=20  mean= 15.000 ms  ( 20.7%)
  ...
```

Numbers are aggregated across all phases inside `with Phase("..."):`
context managers in `poll.py` and `figure_builder.py`.

## Branch / development workflow

The frontend lives on the `monitor_frontend` branch. The frontend
touches only files under `user/hidra/monitor/frontend/`, so it
never conflicts with backend changes.

Stay in sync with master:

```sh
git fetch
git rebase origin/master
```

Python is **not** wired into CMake. The setup.sh helpers
(`cmake_config`, `build_hidra`, `runhidra`) ignore this folder.

## Roadmap (grep for `TODO(...)`)

- `TODO(monitor_info)` — read `pump_interval_ms` from a backend
  endpoint instead of mirroring it in `config.yaml`.
- `TODO(reset)` — wire a "Reset histograms" button when the backend
  exposes a reset endpoint.
- `TODO(event_display)` — single-event panel type; needs a
  non-cumulative backend feed.
- `TODO(remote_overlay)` — fetch reference files over HTTP instead
  of through the shared filesystem.
