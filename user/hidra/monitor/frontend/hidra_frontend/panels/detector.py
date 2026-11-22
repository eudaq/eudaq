"""DetectorPanel — a 2D map of the calorimeter modules.

Unlike the other panels (which draw one histogram per graph slot),
this one reads a single per-channel histogram (`ADC_mean` by default)
and lays its values out *spatially*: each module sits at its (row,
column) position, coloured by the mean ADC of its PMT.

A module has two PMTs — one "S" and one "C" — so we emit **two**
figures, one per type, sharing the same geometry.

Config (in `config.yaml`):

    - type: detector
      histogram: ADC_mean          # optional, default "ADC_mean"

The channel -> (module, row, column, type) map comes from
`hidra_frontend.mapping.get_pmt_channel_info()`. Channels that are not
PMTs (e.g. the muon counter) are simply absent from that map and never
drawn.

The source histogram is a `TProfile` filled with `Fill(channel, adc)`,
so the value for channel `c` is the mean stored in fArray bin `c + 1`
(bin 0 is underflow). We read the buffers directly here — same trick
`MetricPanel` uses — instead of going through the full decoder, since
all we need is one number per channel.

Each figure is built from three stacked `Heatmap` traces over the same
row/column grid, so only the per-cell `z`/`text` arrays change from poll
to poll. That lets Plotly's client-side `react` diff update just the
data instead of rebuilding the whole scene — which it would have to do
with per-module layout shapes/annotations. The three layers, bottom to
top, are:

  1. value heatmap (Viridis) — colours + colorbar for filled cells;
  2. empty overlay (flat grey) — fills mapped-but-unfilled cells so
     they stay visible instead of vanishing into the background;
  3. a transparent hit layer on top — the only interactive trace. It
     has a real (non-gap) value on every mapped cell so a click always
     lands on a point (a click on a heatmap *gap* does not fire in
     Plotly), carrying the channel index as `customdata` for the
     cross-tab navigation. The two visible layers are `hoverinfo=skip`
     so all hover/click goes through this one.

The two visible layers draw a 2 px inter-cell gap for the grid look;
the hit layer drops it (`xgap/ygap=0`) so its click target covers the
full cell pitch and there are no dead zones between modules.
"""

from __future__ import annotations

from typing import Optional

import plotly.graph_objects as go
from dash import dcc, html

from .. import theme
from ..mapping import get_pmt_channel_info
from .base import Panel

# Physical module dimensions (mm). Used only for the cell aspect ratio,
# so each module keeps its real 128 x 28.3 proportions on screen.
MODULE_WIDTH_MM = 128.0
MODULE_HEIGHT_MM = 28.3

# PMT types, in the order their figures appear (S first, then C).
PMT_TYPES = ("S", "C")

COLORSCALE = "Viridis"


class DetectorPanel(Panel):
    def __init__(self, panel_id, params):
        super().__init__(panel_id, params)
        # The spatial layout (grid extent, per-cell module labels and
        # channel indices) derives only from the calo mapping, which is
        # fixed for the process lifetime. Build it once on first render
        # and reuse it every poll — only the `z` colours change.
        self._geometry: Optional[dict] = None

    def _geom(self) -> Optional[dict]:
        if self._geometry is None:
            self._geometry = _build_geometry(get_pmt_channel_info())
        return self._geometry

    def _hist_name(self) -> str:
        return self.params.get("histogram", "ADC_mean")

    def _value_label(self) -> str:
        return self.params.get("label", "mean ADC")

    def _title_suffix(self) -> str:
        """Trigger-type qualifier for the title, derived from the histogram
        name (e.g. ``ADC_mean_physics`` -> `` (physics)``). Empty for the
        inclusive histograms so their title is unchanged."""
        name = self._hist_name()
        if name.endswith("_physics"):
            return " (physics)"
        if name.endswith("_pedestal"):
            return " (pedestal)"
        return ""

    def _title(self, pmt_type: str) -> str:
        # Optional `title_tag` disambiguates two maps of the same histogram
        # family in one tab (e.g. mean vs noise, both on pedestal data).
        tag = self.params.get("title_tag")
        tag = f" — {tag}" if tag else ""
        return f"Detector — {pmt_type} channels{tag}{self._title_suffix()}"

    def histogram_names(self) -> list[str]:
        return [self._hist_name()]

    def figure_names(self) -> list[str]:
        # The detector reads the raw TProfile buffers itself and lays the
        # values out spatially, so it never uses the pre-built bar figure.
        return []

    def layout(self) -> html.Div:
        height = self.params.get("height", "320px")
        # When modules are clickable, tag the graphs so the stylesheet can
        # show a pointer cursor over the plot area (see assets/base.css).
        graph_class = "detector-clickable" if self.link_tab() else ""
        slots = [
            dcc.Graph(
                id={"type": "panel-graph", "panel": self.panel_id, "index": i},
                figure=theme.placeholder_figure(self._title(ptype)),
                style={"flex": "1", "minWidth": "0", "height": height},
                className=graph_class,
                config={"displayModeBar": False},
            )
            for i, ptype in enumerate(PMT_TYPES)
        ]
        return html.Div(
            style={"display": "flex", "gap": "12px", "marginBottom": "12px"},
            children=slots,
        )

    def link_tab(self) -> Optional[str]:
        """Tab id to open when a module is clicked (None = not clickable)."""
        return self.params.get("link_tab")

    def render(self, figs, payloads, client_state):
        payload = payloads.get(self._hist_name())
        values = _channel_means(payload)
        geom = self._geom()
        label = self._value_label()
        return [_detector_figure(ptype, values, geom, label, self._title(ptype)) for ptype in PMT_TYPES]


def _channel_means(payload: Optional[dict]) -> Optional[dict[int, float]]:
    """channel index -> per-channel value, read straight from the buffers.

    For a `TProfile` this is the bin mean (`fArray/fBinEntries`); for a
    plain per-channel `TH1` (e.g. `ADC_noise_pedestal`) it is the bin
    content. Returns None when the payload is missing/unusable (so the
    figure can show a "missing" placeholder).

    A `TProfile` channel with no entries is absent from the dict (its mean
    is undefined). A `TH1` keeps every in-range bin, including a genuine
    `0.0` (so e.g. a zero-noise channel still renders).
    """
    if not payload or "_typename" not in payload:
        return None

    nbins = payload.get("fXaxis", {}).get("fNbins", 0)
    sumw = payload.get("fArray") or []
    if nbins < 1 or not sumw:
        return None

    # fArray / fBinEntries layout: [underflow, bin_1, ..., bin_N, overflow].
    # Channel c was filled at x = c, which lands in bin c + 1.
    entries = payload.get("fBinEntries") or []

    means: dict[int, float] = {}
    for channel in range(nbins):
        idx = channel + 1
        if idx >= len(sumw):
            break
        if entries:
            # TProfile: mean = sum(weight*y) / sum(weight).
            if idx < len(entries) and entries[idx] > 0:
                means[channel] = sumw[idx] / entries[idx]
        else:
            # Plain TH1 fallback: the bin content is the value as-is. Keep it
            # even when 0.0 (a genuine zero, e.g. zero noise) — a truthiness
            # check here would drop legitimate zero-valued channels.
            means[channel] = float(sumw[idx])

    return means


def _build_geometry(info: dict[int, dict]) -> Optional[dict]:
    """Precompute the static spatial layout shared by every poll.

    The grid extent and the per-cell module labels / channel indices are
    fixed by the calo mapping, so we build them once. Returns None when
    there is no mapping (the figure then shows a "no module mapping"
    placeholder). The returned dict holds, per PMT type, the `text` and
    `customdata` grids and a flat list of `(row_idx, col_idx, channel)`
    placements that `render` walks to fill in just the `z` values.

    The grid spans the full integer row/column range of *all* PMT modules
    (not just one type's) so the S and C maps line up and any missing
    position stays an empty cell rather than being collapsed away.
    """
    if not info:
        return None

    all_cols = [d["column"] for d in info.values()]
    all_rows = [d["row"] for d in info.values()]
    columns = list(range(min(all_cols), max(all_cols) + 1))
    rows = list(range(min(all_rows), max(all_rows) + 1))
    col_idx = {c: i for i, c in enumerate(columns)}
    row_idx = {r: i for i, r in enumerate(rows)}

    per_type: dict[str, dict] = {}
    for pmt_type in PMT_TYPES:
        text: list[list[str]] = [[""] * len(columns) for _ in rows]
        customdata: list[list[Optional[int]]] = [[None] * len(columns) for _ in rows]
        cells: list[tuple[int, int, int]] = []
        for d in info.values():
            if d["type"] != pmt_type:
                continue
            ri, ci = row_idx[d["row"]], col_idx[d["column"]]
            text[ri][ci] = d["module"]
            customdata[ri][ci] = d["channel"]
            cells.append((ri, ci, d["channel"]))
        per_type[pmt_type] = {"text": text, "customdata": customdata, "cells": cells}

    return {"columns": columns, "rows": rows, "per_type": per_type}


def _detector_figure(
    pmt_type: str,
    values: Optional[dict[int, float]],
    geom: Optional[dict],
    value_label: str = "mean ADC",
    title: str = "",
) -> go.Figure:
    """One figure for a PMT type: a single Heatmap over the (row, column)
    grid, each cell coloured by that module's per-channel value (mean ADC
    or noise).

    `geom` is the precomputed static layout (see `_build_geometry`); only
    the `z` colours are rebuilt here, per poll. `title` is the full figure
    title (the caller tags it with the trigger type / a `title_tag`)."""
    title = title or f"Detector — {pmt_type} channels"
    # Build layout + trace as plain data and construct the go.Figure once
    # at the end — building two heatmaps per poll this way is much cheaper
    # than go.Figure()+update_layout() (Plotly validation dominates).
    layout = theme.base_figure_layout(title)

    if geom is None:
        layout["annotations"] = [dict(text="no module mapping", showarrow=False, font=dict(color=theme.WARN, size=14))]
        return go.Figure(layout=layout)

    if values is None:
        layout["title"] = f"{title} (missing)"
        layout["annotations"] = [dict(text="missing on server", showarrow=False, font=dict(color=theme.WARN, size=14))]
        return go.Figure(layout=layout)

    columns = geom["columns"]
    rows = geom["rows"]
    cell_info = geom["per_type"][pmt_type]

    # `z` and the per-cell click/hover payload vary poll-to-poll; the static
    # text grid and cell placements come straight from the cached geometry.
    z: list[list[Optional[float]]] = [[None] * len(columns) for _ in rows]
    # Mapped cells with no value this poll: flagged so they render as a flat
    # "empty" tile instead of vanishing into the (unmapped) background.
    z_empty: list[list[Optional[float]]] = [[None] * len(columns) for _ in rows]
    # Transparent hit layer: a real (non-gap) value on *every* mapped cell so
    # the top trace always has a clickable point there — filled or empty. A
    # gap in the value heatmap does NOT emit a click in Plotly (hoverongaps
    # only enables the hover *label*, not click events), so an empty cell can
    # only be made clickable by a non-gap point. `None` stays for unmapped
    # cells, which must remain inert.
    z_hit: list[list[Optional[float]]] = [[None] * len(columns) for _ in rows]
    # Per-cell hover string for the hit layer (rebuilt each poll: the value
    # changes). The hit layer draws no on-cell text, so its `text` is free to
    # hold the full hover label here. The channel index for the click comes
    # from the cached scalar `customdata` grid instead — Plotly does not
    # serialise a per-cell *array* customdata into a heatmap's clickData, so it
    # must stay a scalar.
    hovertext: list[list[Optional[str]]] = [[None] * len(columns) for _ in rows]
    present_values: list[float] = []
    for ri, ci, channel in cell_info["cells"]:
        value = values.get(channel)
        module = cell_info["text"][ri][ci]
        z[ri][ci] = value
        z_hit[ri][ci] = 0.0
        if value is not None:
            present_values.append(value)
            hovertext[ri][ci] = f"{module}  ·  ch {channel}<br>{value_label} {value:.3f}"
        else:
            z_empty[ri][ci] = 0.0
            hovertext[ri][ci] = f"{module}  ·  ch {channel}<br>no data"

    cmin, cmax = (min(present_values), max(present_values)) if present_values else (0.0, 1.0)
    if cmin == cmax:
        cmax = cmin + 1.0

    # The figure is three stacked heatmaps over the same row/column grid,
    # listed here bottom-to-top to match the `data=[...]` order at the return:
    #   1. empty overlay (flat grey) — fills the unfilled-but-mapped cells so
    #      they're visible instead of vanishing into the background;
    #   2. value heatmap (Viridis) — colours + colorbar for filled cells;
    #   3. hit layer (transparent) — the only interactive trace; a real point
    #      on every mapped cell so clicks/hover work uniformly.
    # The first two are visual only (`hoverinfo="skip"`) so they never take
    # part in hit-testing; Plotly's hit test picks the top trace, so all
    # hover/click must come from the transparent layer that sits on top.
    # (The traces are built below in a convenient construction order; only the
    # `data=[...]` order at the return determines stacking.)

    # Values. Empty cells are gaps here -> the grey overlay shows through.
    value_heatmap = go.Heatmap(
        x=columns, y=rows, z=z,
        text=cell_info["text"], texttemplate="%{text}",
        textfont=dict(size=11),
        colorscale=COLORSCALE,
        zmin=cmin, zmax=cmax,
        xgap=2, ygap=2,
        hoverinfo="skip",
        colorbar=dict(title=value_label, thickness=12),
    )

    # Flat grey tile + label for mapped cells with no value this poll.
    empty_overlay = go.Heatmap(
        x=columns, y=rows, z=z_empty,
        text=cell_info["text"], texttemplate="%{text}",
        textfont=dict(size=11, color=theme.FG),
        colorscale=[[0.0, theme.EMPTY], [1.0, theme.EMPTY]],
        # All cells share the same sentinel value (0.0); pin the range so the
        # flat colorscale is applied deterministically instead of autoscaling
        # a zmin == zmax buffer.
        zauto=False, zmin=0.0, zmax=1.0,
        showscale=False,
        xgap=2, ygap=2,
        hoverinfo="skip",
    )

    # Transparent click/hover catcher on top. `z_hit` is a real value on
    # every mapped cell (no gaps), so each module is a clickable point whether
    # or not it has data; unmapped cells stay `None` (inert). Fully transparent
    # colourscale so it doesn't alter the visuals below; `text`/`customdata`
    # drive the hover. No `texttemplate` -> it draws no labels of its own (the
    # labels come from the two visible layers).
    hit_layer = go.Heatmap(
        x=columns, y=rows, z=z_hit,
        # `text` holds the hover label (the layer draws no on-cell text);
        # `customdata` is the scalar channel index the navigation click reads.
        text=hovertext,
        customdata=cell_info["customdata"],
        colorscale=[[0.0, "rgba(0,0,0,0)"], [1.0, "rgba(0,0,0,0)"]],
        zauto=False, zmin=0.0, zmax=1.0,
        showscale=False,
        # No inter-cell gap here (the visible layers use 2): the catcher should
        # cover the full cell pitch, including the 2 px spacing between the
        # visible tiles, so there are no dead zones between modules where a
        # click would miss.
        xgap=0, ygap=0,
        hoverongaps=False,
        hovertemplate="%{text}<extra></extra>",
    )

    # Merge over the base axis styling rather than replacing it.
    layout["xaxis"] = {
        **layout["xaxis"],
        "title": "column",
        "tickmode": "array", "tickvals": columns,
        "showgrid": False, "zeroline": False,
    }
    # autorange reversed -> row 1 at the top (front view). scaleanchor with
    # scaleratio = height/width makes each 1x1 cell display with the real
    # 128 x 28.3 module proportions.
    layout["yaxis"] = {
        **layout["yaxis"],
        "title": "row",
        "tickmode": "array", "tickvals": rows,
        "autorange": "reversed",
        "showgrid": False, "zeroline": False,
        "scaleanchor": "x", "scaleratio": MODULE_HEIGHT_MM / MODULE_WIDTH_MM,
    }
    # Order = bottom-to-top: grey overlay, value colours, transparent hit
    # layer. The hit layer must be last so it sits on top and receives clicks.
    return go.Figure(data=[empty_overlay, value_heatmap, hit_layer], layout=layout)
