"""ChannelSelectorPanel — one per-channel histogram with a channel dropdown.

The backend publishes one TH1D per ADC channel (``ADC_channel_0``,
``ADC_channel_1``, ...). There are far too many to show at once, so this
panel shows a *single* channel's histogram and lets the user switch
channel with a dropdown.

Config (in ``config.yaml``)::

    - type: channel_selector
      template: "ADC_channel_{ch}"     # optional, this is the default

The channel list is normally **auto-discovered** from the live backend
at startup: every histogram whose name matches ``template`` becomes one
dropdown entry, in numeric order. To pin an explicit set instead, give a
``range: [lo, hi]`` (config.py expands ``template`` + ``range`` into a
``names`` list) or a ``names`` list directly.

Dropdown labels are enriched with the detector name from the calo
mapping when the channel is known (e.g. ``ch 5  ·  M105S``); otherwise
just the channel number is shown.

The currently-shown channel is kept in per-process instance state
(``self._selected``), updated by a callback registered in
``register_callbacks()``. The poll callback reads it through
``histogram_names()`` on the next tick, so the plot follows the dropdown
within one poll period. This relies on the single-worker deployment
documented in the README (per-process state is shared across callbacks).
"""

from __future__ import annotations

import logging
import re
from typing import Optional

import numpy as np
from dash import ALL, Dash, Input, Output, ctx, dcc, html

from .. import theme
from ..decoders import DecoderError, get_decoder
from ..figure_builder import overlay_figure
from ..mapping import default_mapping
from .base import Panel
from .graph_controls import controls_overlay

logger = logging.getLogger(__name__)

DEFAULT_TEMPLATE = "ADC_channel_{ch}"

# When `show_trigger_split` is on, the panel overlays these series of the
# selected channel: by default the inclusive histogram plus its
# physics/pedestal copies. Each tuple is (name suffix, legend label, colour).
TRIGGER_SPLIT_SERIES = [
    ("", "total", theme.PRIMARY),
    ("_physics", "physics", theme.SECONDARY),
    ("_pedestal", "pedestal", theme.REFERENCE),
]

# Label + colour for a split series, keyed by name suffix. Lets a panel pick a
# subset/order of series via the `split_suffixes` config param (e.g. FERS has
# only physics/pedestal per channel, no inclusive "total").
_SPLIT_SUFFIX_INFO = {suffix: (label, color) for suffix, label, color in TRIGGER_SPLIT_SERIES}


def _update_rebin_state(rebin: dict[int, int], inputs_list: list) -> bool:
    """Fold the rebin radios' values (Dash `ctx.inputs_list[0]`) into `rebin`.

    Each item is `{"id": {..., "index": i}, "value": factor}`. Updates the
    per-slot factor in place and returns True iff anything changed (so the caller
    knows whether to trigger a refetch). Defensive against malformed items.
    """
    changed = False
    for item in inputs_list:
        if not isinstance(item, dict):
            continue
        iid = item.get("id")
        val = item.get("value")
        if not (isinstance(iid, dict) and "index" in iid and val is not None):
            continue
        try:
            idx = int(iid["index"])
            factor = int(val)
        except (TypeError, ValueError):
            # Non-numeric index/value (shouldn't happen from Dash): skip it.
            continue
        if rebin.get(idx, 1) != factor:
            rebin[idx] = factor
            changed = True
    return changed


def _template_regex(template: str) -> re.Pattern:
    """Turn ``"ADC_channel_{ch}"`` into a regex capturing the channel number.

    Everything around the ``{ch}`` placeholder is matched literally.
    """
    head, _, tail = template.partition("{ch}")
    return re.compile(f"^{re.escape(head)}(\\d+){re.escape(tail)}$")


class ChannelSelectorPanel(Panel):
    def __init__(self, panel_id, params):
        super().__init__(panel_id, params)
        # Two data-source modes, both selected by a single channel index:
        #
        #  * name mode (`templates`/`template`): one per-channel histogram per
        #    channel exists on the backend (e.g. ADC_channel_<N>); fetched by
        #    name. `templates` can list several (HG+LG side by side).
        #  * projection mode (`projection_templates`): the per-channel data lives
        #    in one TH2 per gain/trigger (e.g. FERS_HG_dist_physics, x=channel,
        #    y=ADC). A single channel is fetched as a server-side ProjectionY,
        #    encoded as a `<th2>#projy=<xbin>` token resolved by BackendClient.
        proj = params.get("projection_templates")
        self._projection = bool(proj)
        if self._projection:
            templates = proj
        else:
            templates = params.get("templates") or [params.get("template", DEFAULT_TEMPLATE)]
        if isinstance(templates, str):
            # A single template written without YAML list syntax: treat it as a
            # one-element list, not a sequence of characters.
            templates = [templates]
        self._templates = [str(t) for t in templates]
        self._template = self._templates[0]
        self._regex = _template_regex(self._template) if not self._projection else None
        # Auto-discovery (name mode) matches `template + discover_suffix` and
        # captures the channel number; needed when the backend exposes no bare
        # per-channel histogram (only the `_physics`/`_pedestal` copies).
        self._discover_suffix = params.get("discover_suffix", "")
        self._discover_regex = (
            _template_regex(self._template + self._discover_suffix) if not self._projection else None
        )
        # Projection mode has no per-channel names to discover: the channel
        # count is the per-channel TH2's x-axis size, learned from the backend
        # (GetNbinsX) via the injected client. Resolved lazily and retried while
        # unknown, so it recovers if the backend wasn't up yet at startup. An
        # explicit `n_channels` (e.g. in tests) skips the backend lookup.
        self._client = params.get("_client")
        self._n_channels = int(params.get("n_channels", 0))
        # Dropdown label style. With `board_labels: true` the labels read
        # "board B · ch L" (FERS, no calo mapping; board size from the `fers:`
        # config section); otherwise enriched with the calo module name.
        self._channels_per_board = (
            int(params["channels_per_board"])
            if params.get("board_labels") and params.get("channels_per_board")
            else None
        )
        # `fixed_channel: N` pins the panel to one channel and hides the dropdown
        # (e.g. the muon counter on ADC channel 193). `fixed_channels: [a, b, …]`
        # instead shows one plot per channel side by side (with `cols`, e.g. the
        # three Cherenkov chambers on one row). Both disable the dropdown; the
        # rest of the projection/overlay/on-demand machinery is unchanged.
        fixed = params.get("fixed_channel")
        self._fixed_channel: Optional[int] = int(fixed) if fixed is not None else None
        fixed_list = params.get("fixed_channels")
        self._fixed_channels: Optional[list[int]] = [int(c) for c in fixed_list] if fixed_list else None
        if self._fixed_channel is not None:
            self._selected_ch: Optional[int] = self._fixed_channel
        elif self._fixed_channels:
            self._selected_ch = self._fixed_channels[0]
        else:
            self._selected_ch = None
        # Graph slots are laid out in rows of `cols` (default 1 = stacked, matching
        # the previous one-plot-per-template behaviour).
        self._cols = max(1, int(params.get("cols", 1)))
        # Optional fixed plot title (overrides the auto "ADC · ch N"); handy when
        # the channel has a meaningful name (e.g. "Muon counter").
        self._title: Optional[str] = params.get("title")
        # When on, each graph slot overlays several series of the selected
        # channel (one step line each), built from the raw payloads, so the
        # panel needs its own decoder. `split_suffixes` selects which series.
        self._split = bool(params.get("show_trigger_split", False))
        self._split_series = self._build_split_series(params.get("split_suffixes"))
        # Optional "fraction above an ADC threshold" annotation. When `threshold`
        # is set the panel also fetches the `threshold_series` copy (default the
        # displayed "total"; the muon counter uses "_physics" to count only
        # physics events), computes the fraction of its entries above the
        # threshold, writes it into the title and draws a vertical line there.
        thr = params.get("threshold")
        self._threshold: Optional[float] = float(thr) if thr is not None else None
        self._threshold_series = str(params.get("threshold_series", ""))
        # The overlay (split mode) and the fraction both need a decoder.
        self._decoder = (
            get_decoder(params.get("decoder", "pure"))
            if (self._split or self._threshold is not None)
            else None
        )
        # Emergency mitigation (issue #153): a projection-mode fetch is a
        # server-side exe.json ProjectionY, whose repeated *interpreted* calls
        # grow ROOT's process memory over time. So in projection mode we fetch
        # only on demand — channel change, tab (re)open, or an explicit Refresh —
        # and keep the last figures cached instead of polling continuously.
        self._needs_fetch = True
        self._last_figs: Optional[list] = None
        # Per-slot rebin factor (slot index -> 1/2/4/8) from the segmented control.
        # Server-side instance state (like _selected_ch), the source of truth used
        # by render(); a change triggers one refetch+rebuild via _needs_fetch.
        self._rebin: dict[int, int] = {}

    @staticmethod
    def _build_split_series(suffixes) -> list[tuple[str, str, str]]:
        if not suffixes:
            return TRIGGER_SPLIT_SERIES
        if isinstance(suffixes, str):
            # A single suffix written without YAML list syntax: treat it as a
            # one-element list, not a sequence of characters.
            suffixes = [suffixes]
        series: list[tuple[str, str, str]] = []
        for suffix in suffixes:
            label, color = _SPLIT_SUFFIX_INFO.get(suffix, (suffix.lstrip("_") or "total", theme.PRIMARY))
            series.append((suffix, label, color))
        return series

    def gain_tag(self) -> Optional[str]:
        """`"HG"`/`"LG"` parsed from the (first) template, used to pair this
        selector with the matching FERS board map. None when neither token is
        present, or when the panel spans several gains (multiple templates) — in
        that case any gain's board map links to this single selector."""
        if len(self._templates) > 1:
            return None
        if "HG" in self._template:
            return "HG"
        if "LG" in self._template:
            return "LG"
        return None

    def _projection_probe(self) -> str:
        """A per-channel TH2 name to read the channel count from (its x axis)."""
        suffix = self._split_series[0][0] if self._split_series else ""
        return f"{self._templates[0]}{suffix}"

    def _channels(self) -> list[int]:
        """Sorted list of selectable channel indices."""
        if self._fixed_channel is not None:
            return [self._fixed_channel]
        if self._projection:
            # Learn (and cache) the channel count from the TH2 x axis the first
            # time it is needed; retry while still unknown (backend not up yet).
            if self._n_channels <= 0 and self._client is not None:
                self._n_channels = self._client.nbins_x(self._projection_probe()) or 0
            return list(range(self._n_channels))
        params = self.params
        names = params.get("names")
        if names:
            chans = {c for n in names if (c := self._channel_of(n)) is not None}
        else:
            chans = {
                int(m.group(1))
                for n in (params.get("available_histograms") or [])
                if (m := self._discover_regex.match(n))
            }
        return sorted(chans)

    def _channel_of(self, name: str) -> Optional[int]:
        m = self._regex.match(name) if self._regex else None
        return int(m.group(1)) if m else None

    def select_channel(self, ch: int) -> None:
        """Select a channel by index (cross-panel navigation, e.g. clicking a
        cell on a detector / FERS board map). The next poll fetches it."""
        ch = int(ch)
        # In projection mode the channel count is learned once and cached. If the
        # target is beyond it (e.g. the backend grew from 20 to 23 FERS boards
        # after we learned the count, so a MAXICC click lands at index >= 1280),
        # drop the cache so _channels() re-queries it — otherwise the out-of-range
        # channel would be clamped back to channel 0.
        if self._projection and self._n_channels and ch >= self._n_channels:
            self._n_channels = 0
        self._selected_ch = ch
        self._needs_fetch = True

    # ---- slots / token / title helpers ----------------------------------

    @property
    def _has_dropdown(self) -> bool:
        """The channel dropdown is shown only in the interactive selector mode
        (neither a single nor a multi fixed-channel panel)."""
        return self._fixed_channel is None and self._fixed_channels is None

    def _slot_specs(self) -> list[tuple[str, Optional[int]]]:
        """The (template, channel) pair for each graph slot, in slot order.

        Two shapes: several templates of the *same* (current) channel — the
        interactive / single-fixed-channel case — or the *same* template across
        several fixed channels (`fixed_channels`, e.g. the Cherenkov chambers).
        """
        if self._fixed_channels is not None:
            return [(self._template, c) for c in self._fixed_channels]
        return [(t, self._selected_ch) for t in self._templates]

    def _token(self, template: str, suffix: str, ch: int) -> str:
        """Fetch token for one series of one slot at channel `ch`."""
        if self._projection:
            # `<th2>#projy=<xbin>` — ROOT bins are 1-indexed (channel c -> c+1);
            # resolved by BackendClient into a server-side ProjectionY.
            return f"{template}{suffix}#projy={ch + 1}"
        return f"{template.format(ch=ch)}{suffix}"

    def _slot_tokens(self, template: str, ch: int) -> list[str]:
        if self._split:
            return [self._token(template, suffix, ch) for suffix, _, _ in self._split_series]
        return [self._token(template, "", ch)]

    def _slot_title(self, template: str, ch: Optional[int]) -> str:
        if ch is None:
            return "no channel"
        # A configured title wins (single-slot fixed-channel panels, e.g. the
        # muon counter). With several slots it would be ambiguous, so only honour
        # it when there is one.
        if self._title and len(self._slot_specs()) == 1:
            return self._title
        # Multi fixed-channel panels (e.g. Cherenkov): prefer the detector name
        # from the calo mapping, so each plot reads "Cher1 · ch 194".
        if self._fixed_channels is not None:
            try:
                return f"{default_mapping().get_channel_name(ch)} · ch {ch}"
            except KeyError:
                pass
        if self._projection:
            # Prettify the TH2 base name: strip a trailing "_dist" and turn
            # underscores into spaces (FERS_HG_dist -> "FERS HG", ADC_dist -> "ADC").
            base = template[:-len("_dist")] if template.endswith("_dist") else template
            return f"{base.replace('_', ' ')} · ch {ch}"
        return template.format(ch=ch)

    # ---- Panel API -------------------------------------------------------

    def histogram_names(self) -> list[str]:
        specs = [(t, c) for t, c in self._slot_specs() if c is not None]
        if not specs:
            return []
        # Projection mode polls on demand only (see __init__ / issue #153): once
        # the selected channel has been fetched, request nothing until it changes.
        if self._projection and not self._needs_fetch:
            return []
        out: list[str] = []
        for template, ch in specs:
            out += self._slot_tokens(template, ch)
            # Also fetch the series the threshold fraction is computed on, if it
            # isn't already among the displayed series.
            if self._threshold is not None:
                tok = self._token(template, self._threshold_series, ch)
                if tok not in out:
                    out.append(tok)
        return out

    def figure_names(self) -> list[str]:
        # In split mode the panel builds its own overlaid figure from the
        # raw payloads, so it doesn't use the framework's per-name figures.
        return [] if self._split else self.histogram_names()

    def control_indices(self) -> list[int]:
        # One 1D plot per slot.
        return list(range(len(self._slot_specs())))

    def _options(self) -> list[dict]:
        chans = self._channels()
        if self._selected_ch not in chans:
            self._selected_ch = chans[0] if chans else None
        mapping = None if self._channels_per_board else default_mapping()
        opts: list[dict] = []
        for ch in chans:
            if self._channels_per_board:
                # FERS: board/local label, no calo mapping (those module names
                # belong to the ADC channels and would be misleading here).
                board, local = divmod(ch, self._channels_per_board)
                label = f"board {board}  ·  ch {local}"
            else:
                try:
                    label = f"ch {ch}  ·  {mapping.get_channel_name(ch)}"
                except KeyError:
                    label = f"ch {ch}"
            opts.append({"label": label, "value": ch})
        return opts

    def layout(self) -> html.Div:
        # (Re)opening the tab refreshes once; projection mode then goes quiet.
        self._needs_fetch = True
        # Control header. The dropdown only in interactive mode; a single fixed
        # channel shows a static label; multi fixed channels (Cherenkov) label
        # each plot individually, so the header carries only the refresh button.
        if self._has_dropdown:
            options = self._options()  # may set _selected_ch
            controls: list = [
                html.Span("Channel:", style={"color": theme.FG, "fontSize": "13px"}),
                dcc.Dropdown(
                    id={"type": "channel-select", "panel": self.panel_id},
                    options=options,
                    value=self._selected_ch,
                    clearable=False,
                    placeholder="(no channels on backend)" if not options else "select a channel",
                    style={"width": "320px"},
                ),
            ]
        elif self._fixed_channel is not None:
            controls = [
                html.Span(self._slot_title(self._template, self._fixed_channel),
                          style={"color": theme.FG, "fontSize": "13px", "fontWeight": "bold"})
            ]
        else:
            controls = []
        extra_children: list = []
        if self._projection:
            # Projection mode does not auto-refresh (issue #153); offer a manual one.
            controls.append(
                html.Button(
                    "↻ refresh",
                    id={"type": "channel-refresh", "panel": self.panel_id},
                    n_clicks=0,
                    style={"marginLeft": "8px", "fontSize": "12px", "cursor": "pointer"},
                )
            )
            extra_children.append(dcc.Store(id={"type": "channel-refresh-sink", "panel": self.panel_id}))

        # One graph slot per (template, channel) spec, arranged in rows of `cols`
        # (default 1 = stacked). `cols: 3` puts e.g. the three Cherenkov plots on
        # one row.
        slots = [
            html.Div(
                className="plot-cell",
                style={"flex": "1", "minWidth": "0"},
                children=[
                    dcc.Graph(
                        id={"type": "panel-graph", "panel": self.panel_id, "index": i},
                        figure=theme.placeholder_figure(self._slot_title(template, ch)),
                        style={"height": "420px"},
                        config={"displayModeBar": False},
                    ),
                    controls_overlay(self.panel_id, i, rebin=True, rebin_value=self._rebin.get(i, 1)),
                ],
            )
            for i, (template, ch) in enumerate(self._slot_specs())
        ]
        rows = [
            html.Div(
                style={"display": "flex", "gap": "12px", "marginBottom": "12px"},
                children=slots[start:start + self._cols],
            )
            for start in range(0, len(slots), self._cols)
        ]
        header = (
            [html.Div(
                style={"display": "flex", "alignItems": "center", "marginBottom": "12px", "gap": "8px"},
                children=controls,
            )]
            if controls else []
        )
        return html.Div(
            header
            + rows
            # Throwaway sinks: the selection / rebin callbacks must write to at
            # least one Output. The real state lives in self._selected_ch /
            # self._rebin.
            + [dcc.Store(id={"type": "channel-select-sink", "panel": self.panel_id})]
            + [dcc.Store(id={"type": "graph-rebin-sink", "panel": self.panel_id})]
            + extra_children
        )

    def render(self, figs, payloads, client_state):
        slot_specs = self._slot_specs()
        if all(ch is None for _, ch in slot_specs):
            return [theme.placeholder_figure("no channel selected") for _ in slot_specs]
        # Projection mode: if no fetch was requested this tick, reuse the cached
        # figures (we didn't poll exe.json) rather than rebuilding from empty data.
        if self._projection and not self._needs_fetch and self._last_figs is not None:
            return self._last_figs
        out = []
        for slot_index, (template, ch) in enumerate(slot_specs):
            title = self._slot_title(template, ch)
            if ch is None:
                out.append(theme.placeholder_figure(title))
                continue
            rebin = self._rebin.get(slot_index, 1)
            # Threshold annotation (independent of split mode): put the fraction
            # above the threshold into the title; the marker is added after the
            # figure is built. The raw threshold-series payload is fetched by
            # histogram_names() and so is present in `payloads` in either mode.
            if self._threshold is not None:
                frac = self._fraction_above(payloads.get(self._token(template, self._threshold_series, ch)))
                title = self._title_with_fraction(title, frac)
            if self._split:
                # Overlay the configured series (e.g. physics / pedestal) of this
                # slot into one figure. The Plotly legend toggles each series.
                specs = [
                    (payloads.get(self._token(template, suffix, ch)), color, label)
                    for suffix, label, color in self._split_series
                ]
                fig = overlay_figure(self._decoder, specs, title, rebin=rebin)
            else:
                fig = figs.get(self._token(template, "", ch))
                if fig is None:
                    fig = theme.placeholder_figure(title)
                elif self._threshold is not None:
                    fig.update_layout(title_text=title)
            if self._threshold is not None:
                # Vertical marker at the ADC threshold the fraction refers to.
                fig.add_vline(
                    x=self._threshold, line_dash="dash", line_color=theme.ACCENT, line_width=1,
                    annotation_text=f"{self._threshold:g}", annotation_position="top",
                    annotation_font_color=theme.ACCENT,
                )
            out.append(fig)
        if self._projection:
            self._last_figs = out
            self._needs_fetch = False
        return out

    def _fraction_above(self, payload) -> Optional[float]:
        """Fraction of the threshold-series entries with bin centre > threshold.

        Decoded from the raw projection payload; returns None when the payload is
        missing/unusable or the histogram is empty (so the title shows "n/a").
        """
        if not payload or self._decoder is None or self._threshold is None:
            return None
        try:
            decoded = self._decoder.decode(payload)
        except DecoderError:
            # Payload shape this decoder can't handle: a normal "n/a", stay quiet.
            return None
        except Exception:  # noqa: BLE001 — one bad payload must not break the poll
            logger.warning("threshold fraction: unexpected error decoding payload", exc_info=True)
            return None
        counts = np.asarray(decoded.counts, dtype=float)
        edges = np.asarray(decoded.edges, dtype=float)
        if counts.size == 0 or edges.size != counts.size + 1:
            return None
        total = counts.sum()
        if total <= 0:
            return None
        centres = 0.5 * (edges[:-1] + edges[1:])
        above = counts[centres > self._threshold].sum()
        return float(above / total)

    def _title_with_fraction(self, title: str, frac: Optional[float]) -> str:
        label = _SPLIT_SUFFIX_INFO.get(self._threshold_series,
                                       (self._threshold_series.lstrip("_") or "total", ""))[0]
        thr = f"{self._threshold:g}"
        if frac is None:
            return f"{title} — {label} frac > {thr}: n/a"
        return f"{title} — {label} frac > {thr} = {frac * 100:.1f}%"

    def register_callbacks(self, app: Dash) -> None:
        # Fixed-channel panels (single or multi) render no dropdown, so there is
        # no selection callback to register (its Input component would not exist).
        if self._has_dropdown:
            @app.callback(
                Output({"type": "channel-select-sink", "panel": self.panel_id}, "data"),
                Input({"type": "channel-select", "panel": self.panel_id}, "value"),
                prevent_initial_call=True,
            )
            def _on_select(value):
                # Persist the choice in instance state; the next poll picks it up via
                # histogram_names(). Returning `value` satisfies Dash's Output rule.
                if value is not None:
                    self._selected_ch = int(value)
                    self._needs_fetch = True   # projection mode fetches on change only
                return value

        if self._projection:
            @app.callback(
                Output({"type": "channel-refresh-sink", "panel": self.panel_id}, "data"),
                Input({"type": "channel-refresh", "panel": self.panel_id}, "n_clicks"),
                prevent_initial_call=True,
            )
            def _on_refresh(n_clicks):
                # Manual refresh: fetch the current channel once on the next poll.
                self._needs_fetch = True
                return n_clicks

        # Per-slot rebin control (segmented ×1/×2/×4/×8). One callback per panel
        # matches all its rebin radios via the index wildcard; we read each slot's
        # value from ctx so the per-slot factor is updated independently.
        @app.callback(
            Output({"type": "graph-rebin-sink", "panel": self.panel_id}, "data"),
            Input({"type": "graph-ctl-rebin", "panel": self.panel_id, "index": ALL}, "value"),
            prevent_initial_call=True,
        )
        def _on_rebin(values):
            inputs = ctx.inputs_list[0] if ctx.inputs_list else []
            if _update_rebin_state(self._rebin, inputs):
                # Projection mode fetches on change only: rebuild with the new
                # binning on the next poll (like a channel change / refresh).
                self._needs_fetch = True
            return values
