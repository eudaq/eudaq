"""Polling callback: fetch histograms, build Plotly figures, write them into the DOM.

Two callbacks live here:

  1. `update_tab_content(tab_id)` — rebuilds the tab body whenever the
     user clicks a different tab. It writes both the new DOM and a
     small `tab-mount-state` Store. That Store is what `poll()` listens
     to (instead of `tabs.value` directly): Dash applies Store changes
     only after the DOM output is committed, so the poll always sees
     the rebuilt DOM, not the previous tab's.

  2. `poll(n_intervals, tab_mount_state, ...)` — the heart of the
     dashboard. Triggered by `dcc.Interval` ticks and by tab switches.
     For each tick:
       * collect the histogram names the active panels want,
       * fetch them all in one `POST /multi.json` call,
       * build Plotly figures (with optional overlay),
       * push the figures into the `panel-graph` slots via Dash's
         pattern-matching outputs.

Pattern-matching IDs (the curly-brace dicts you'll see below) are how
Dash addresses multiple components with one callback: every panel
graph carries an ID like `{"type": "panel-graph", "panel": <id>,
"index": <i>}`, and the callback's Output uses `ALL` to mean "every
component matching this shape".
"""

from __future__ import annotations

import logging

from dash import ALL, Dash, Input, Output, State, ctx, html, no_update

from .. import theme
from ..backend_client import BackendClient
from ..config import Config
from ..decoders import Decoder
from ..figure_builder import to_figure
from ..overlay import OverlayStore
from ..panels.base import Panel
from ..perf import PERF, Phase

logger = logging.getLogger(__name__)

# How often to dump the perf summary to the log (in number of polls).
# 20 polls × 500 ms = roughly every 10 seconds with the default rate.
PERF_LOG_EVERY_POLLS = 20

# Total event counter shown in the header on every tab. Fetched every poll
# regardless of the active tab (same histogram the Summary tab's count card uses).
HEADER_COUNT_HIST = "event_count"


def _event_count_text(payload) -> str:
    """Format the single-bin event counter for the header (e.g. "1,234").

    Reads the bin content straight from the TBufferJSON payload (same trick as
    the rate/metric panels); shows an em dash when it's missing/unavailable."""
    if payload and "_typename" in payload:
        arr = payload.get("fArray") or []
        nbins = payload.get("fXaxis", {}).get("fNbins", 0)
        if arr and nbins >= 1:
            return f"{int(sum(arr[1:nbins + 1])):,}"
    return "—"


def register(
    app: Dash,
    config: Config,
    panels_by_tab: dict[str, list[Panel]],
    client: BackendClient,
    overlay_store: OverlayStore,
    decoder: Decoder,
) -> None:
    # Monotonic counter. Each call to update_tab_content bumps it so
    # the tab-mount-state Store data is always different (different
    # "rev" value), which guarantees the poll callback fires — even if
    # the user clicks the same tab twice in a row.
    tab_mount_rev = 0

    @app.callback(
        Output("tab-content", "children"),
        Output("tab-mount-state", "data"),
        Input("tabs", "value"),
    )
    def update_tab_content(tab_id):
        nonlocal tab_mount_rev
        tab_mount_rev += 1
        panels = panels_by_tab.get(tab_id, [])
        children = html.Div([panel.layout() for panel in panels])
        return children, {"tab": tab_id, "rev": tab_mount_rev}

    @app.callback(
        # ALL means "address every component whose ID matches this
        # shape currently in the DOM". The callback must return a list
        # of figures with the same length as the number of matches.
        Output({"type": "panel-graph", "panel": ALL, "index": ALL}, "figure"),
        Output("status-bar", "children"),
        Output("header-event-count", "children"),
        Input("interval", "n_intervals"),
        Input("tab-mount-state", "data"),
        State("client-state", "data"),
        # Per-plot controls (see panels/graph_controls.py). Read as State
        # so toggling them doesn't itself trigger a poll — the change is
        # picked up on the next tick.
        State({"type": "graph-ctl-logy", "panel": ALL, "index": ALL}, "value"),
        State("graph-reset", "data"),
        prevent_initial_call=False,
    )
    def poll(n_intervals, tab_mount_state, client_state, _logy_values, reset_data):
        # Defensive: in some edge cases Dash may invoke the callback
        # with a stale or unexpected Store value (e.g. during hot
        # reload). Treat anything that isn't a dict as empty.
        if not isinstance(tab_mount_state, dict):
            tab_mount_state = {}
        tab_id = tab_mount_state.get("tab")
        client_state = client_state or {}

        with Phase("poll.total"):
            panels = panels_by_tab.get(tab_id, [])

            # Ask every panel of the active tab which histogram names
            # it needs. The de-dup is so two panels showing the same
            # histogram only cause one HTTP fetch.
            wanted: list[str] = []
            for panel in panels:
                for name in panel.histogram_names():
                    if name not in wanted:
                        wanted.append(name)

            # Always fetch the header event counter too, so it stays live on
            # every tab — but keep it out of `wanted` so the status-bar count and
            # the figure build still reflect only what the active tab asked for.
            fetch_names = wanted if HEADER_COUNT_HIST in wanted else [*wanted, HEADER_COUNT_HIST]
            with Phase("poll.fetch_multi"):
                data = client.fetch_multi(fetch_names) if fetch_names else {}
            header_count = _event_count_text(data.get(HEADER_COUNT_HIST))
            # Reachability is judged on the tab's own histograms only, not the
            # always-on header counter, so the status bar reads exactly as before.
            reachable = any(data.get(name) is not None for name in wanted)

            overlay_file = client_state.get("overlay_file")

            # Build a Plotly figure only for the histograms some panel
            # renders from `figs` (grid, channel_selector). Panels that
            # read the raw payload instead (metric, detector) declare no
            # figure_names, so we skip the construction cost for them.
            needed_figs: set[str] = set()
            for panel in panels:
                needed_figs.update(panel.figure_names())

            # Overlay lookup is cached inside OverlayStore so repeated
            # polls don't reopen the `.root` file.
            figs_by_name: dict[str, object] = {}
            with Phase("poll.to_figure_all"):
                for name, payload in data.items():
                    if name not in needed_figs:
                        continue
                    with Phase("poll.overlay_lookup"):
                        overlay_hist = overlay_store.get(overlay_file, name) if overlay_file else None
                    with Phase("poll.to_figure_one"):
                        figs_by_name[name] = to_figure(
                            decoder, payload, name,
                            overlay_hist=overlay_hist,
                            options=config.histogram_options.get(name),
                            normalize=config.overlay.normalize,
                        )

            # Each panel decides which figures land in its own graph
            # slots, in the order matching the `index` IDs it created
            # in `Panel.layout()`. We flatten the result across all
            # panels of the current tab.
            figures_out: list = []
            with Phase("poll.panel_render"):
                for panel in panels:
                    # Time each panel's render separately (nested under
                    # poll.panel_render) so the summary shows which panel type
                    # dominates — e.g. detector/fers_board build their heatmaps
                    # here, outside the `to_figure_one` path.
                    with Phase(panel.__class__.__name__):
                        figures_out.extend(panel.render(figs_by_name, data, client_state))

            # Race guard. When the user switches tab quickly, a poll
            # triggered for the previous tab may still be running when
            # the DOM has already been rebuilt for the new tab. If we
            # write our `figures_out` (sized for the old tab) into the
            # new tab's panel-graph slots, Dash raises
            # InvalidCallbackReturnValue. Detect the mismatch and skip
            # the write — the next poll, triggered by tab-mount-state,
            # will be coherent.
            expected = len(ctx.outputs_list[0]) if ctx.outputs_list else 0
            if len(figures_out) != expected:
                return [no_update] * expected, no_update, header_count

            # Apply per-plot controls (log-y toggle + reset-zoom counter)
            # to the figures of the controllable slots. Done here, after
            # every panel has rendered, so it stays generic across panel
            # types.
            with Phase("poll.apply_controls"):
                _apply_graph_controls(panels, figures_out, reset_data or {})

        # Build the status bar message: a coloured state dot + text. The
        # dot encodes backend health at a glance (green ok / red
        # unreachable / amber idle).
        pump_hint = config.polling.server_pump_ms_hint
        # Count only the histograms the active tab asked for (not the always-on
        # header counter), so the status reads the same as before.
        n_ok = sum(1 for name in wanted if data.get(name) is not None)
        n_total = len(wanted)
        overlay_text = f"  ·  overlay: {overlay_file}" if overlay_file else ""
        if not wanted:
            dot, text = theme.WARN, f"no histograms requested  ·  server pump ~{pump_hint} ms"
        elif not reachable:
            dot, text = theme.ERR, f"cannot reach backend at {config.backend.url}{overlay_text}"
        else:
            dot, text = theme.OK, (
                f"{n_ok}/{n_total} histograms  ·  poll #{n_intervals}  ·  "
                f"server pump ~{pump_hint} ms{overlay_text}"
            )
        status = [
            html.Span("●", style={"color": dot, "marginRight": "8px", "textShadow": f"0 0 8px {dot}"}),
            text,
        ]

        # Periodic perf summary so it's easy to spot when something
        # gets slow (e.g. lots of histograms added to a tab).
        if n_intervals and n_intervals % PERF_LOG_EVERY_POLLS == 0:
            logger.info("=== perf summary (window = last %d polls) ===", PERF_LOG_EVERY_POLLS)
            for line in PERF.summary_lines():
                logger.info(line)
            PERF.reset()

        return figures_out, status, header_count


def _apply_graph_controls(panels: list[Panel], figures_out: list, reset_data: dict) -> None:
    """Apply log-y and reset-zoom state to the controllable figures.

    Generic across panel types: a slot is controllable iff its panel
    lists its index in `control_indices()`. We map the figures back to
    their graph IDs through `ctx.outputs_list` (same order as
    `figures_out`) and the log-y state through `ctx.states_list`.
    """
    controllable: set[str] = set()
    for panel in panels:
        for idx in panel.control_indices():
            controllable.add(f"{panel.panel_id}|{idx}")
    if not controllable:
        return

    # Log-y checklist values, read from the State group(s).
    logy: dict[str, bool] = {}
    for group in ctx.states_list:
        items = group if isinstance(group, list) else [group]
        for state in items:
            sid = state.get("id") if isinstance(state, dict) else None
            if isinstance(sid, dict) and sid.get("type") == "graph-ctl-logy":
                key = f"{sid['panel']}|{sid['index']}"
                logy[key] = "logy" in (state.get("value") or [])

    out_ids = ctx.outputs_list[0] if ctx.outputs_list else []
    for fig, out in zip(figures_out, out_ids):
        oid = out.get("id") if isinstance(out, dict) else None
        if not isinstance(oid, dict):
            continue
        key = f"{oid['panel']}|{oid['index']}"
        if key not in controllable:
            continue
        # Direct attribute assignment, not fig.update_*(): the update
        # helpers validate/merge the whole subtree (~1 ms/fig), the direct
        # set is ~10x cheaper and all we need here.
        # uirevision preserves the user's zoom/pan across polls; folding in
        # the reset counter drops it on the tick after a reset click.
        rev = reset_data.get(key, 0)
        fig.layout.uirevision = f"{key}|{rev}"
        # Only force log when requested — a freshly built figure already
        # defaults to a linear y axis, so "off" needs no work.
        if logy.get(key):
            fig.layout.yaxis.type = "log"
