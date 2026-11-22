"""SiPMDetectorPanel — a 2D map of the SiPM (FERS) detector.

Like the calo `DetectorPanel`, this reads a single per-channel histogram
(`FERS_HG_mean` by default) and lays its values out *spatially*: each
FERS channel sits at its (row, column) fiber position in the detector,
coloured by that channel's mean. Unlike the calo map there is a single
grid: the "S" and "C" fibers of a module are interleaved by row within
the same column, so one heatmap shows the whole detector face.

The channel -> (column, row, type, module) map comes from
`hidra_frontend.mapping.get_sipm_channel_info()`. Its key is the global
FERS channel index ``boardID * 64 + ch`` — exactly the index the FERS
histograms are filled with — so a value looked up by that index drops
straight onto the right cell.

Config (in `config.yaml`):

    - type: sipm_detector
      histogram: FERS_HG_mean       # base name when mode_toggle is on
      label: "mean HG"
      mode_toggle: true             # physics/pedestal radio
      link_tab: fers_channels       # optional: click opens that tab

With `mode_toggle: true` the panel shows a physics/pedestal radio and
appends the active suffix to `histogram` (`FERS_HG_mean` -> `_physics` /
`_pedestal`), so only the *shown* variant is fetched. Without it,
`histogram` is used verbatim.

The figure is three stacked `Heatmap` traces over the same row/column
grid (same technique as the calo `DetectorPanel`), bottom to top:

  1. empty overlay (flat grey) — fills mapped-but-unfilled cells so the
     detector outline stays visible before data arrives;
  2. value heatmap (Viridis) — colours + colorbar for filled cells;
  3. a transparent hit layer on top — the only interactive trace, with a
     real (non-gap) value on every mapped cell so a click always lands on
     a point (a click on a heatmap *gap* does not fire in Plotly). It
     carries the channel index as a scalar `customdata` for the cross-tab
     navigation; the two visible layers are `hoverinfo=skip`.
"""

from __future__ import annotations

from typing import Optional

import plotly.graph_objects as go
from dash import Dash, Input, Output, dcc, html

from .. import theme
from ..mapping import get_sipm_channel_info
from .base import Panel

COLORSCALE = "Viridis"
_MODES = ("physics", "pedestal")

# Physical fiber pitch (mm): 16 mm between columns, fiber_diameter*sqrt(3)/2
# between rows. Used only for the cell aspect ratio so the map keeps the real
# (wide, short) detector proportions on screen.
COL_PITCH_MM = 16.0
ROW_PITCH_MM = 1.763


class SiPMDetectorPanel(Panel):
    def __init__(self, panel_id, params):
        super().__init__(panel_id, params)
        # Optional physics/pedestal toggle (HG/LG mean maps). When on, the
        # active suffix is appended to the configured base histogram name and
        # only that variant is fetched.
        self._mode_toggle = bool(params.get("mode_toggle", False))
        self._mode = "physics"
        # The spatial layout (grid extent, per-cell channel/hover) derives only
        # from the SiPM mapping, fixed for the process lifetime. Build it once
        # on first render and reuse it every poll — only the `z` colours change.
        self._geometry: Optional[dict] = None

    def _geom(self) -> Optional[dict]:
        if self._geometry is None:
            self._geometry = _build_geometry(get_sipm_channel_info())
        return self._geometry

    # ---- config helpers --------------------------------------------------

    def _base_hist(self) -> str:
        default = "FERS_HG_mean" if self._mode_toggle else "FERS_HG_mean_physics"
        name = self.params.get("histogram", default)
        # With the toggle on we append the active `_physics`/`_pedestal` suffix
        # ourselves (see `_hist_name`); tolerate a base that already carries one
        # (e.g. a copied `FERS_HG_mean_physics`) so we never request a
        # double-suffixed, non-existent `..._physics_physics`.
        if self._mode_toggle:
            for suffix in (f"_{m}" for m in _MODES):
                if name.endswith(suffix):
                    return name[: -len(suffix)]
        return name

    def _hist_name(self) -> str:
        if self._mode_toggle:
            return f"{self._base_hist()}_{self._mode}"
        return self._base_hist()

    def _value_label(self) -> str:
        return self.params.get("label", "mean")

    def gain_tag(self) -> Optional[str]:
        """`"HG"`/`"LG"` parsed from the histogram name, used to pair this map
        with the matching channel selector in `link_tab` (None if neither)."""
        name = self._base_hist()
        if "_HG" in name:
            return "HG"
        if "_LG" in name:
            return "LG"
        return None

    def link_tab(self) -> Optional[str]:
        """Tab id to open when a cell is clicked (None = not clickable)."""
        return self.params.get("link_tab")

    def _title_suffix(self) -> str:
        if self._mode_toggle:
            return f" ({self._mode})"
        name = self._hist_name()
        if name.endswith("_physics"):
            return " (physics)"
        if name.endswith("_pedestal"):
            return " (pedestal)"
        return ""

    def _title(self) -> str:
        gain = self.gain_tag()
        gain = f" {gain}" if gain else ""
        tag = self.params.get("title_tag")
        tag = f" — {tag}" if tag else ""
        return f"SiPM detector{gain}{tag}{self._title_suffix()}"

    # ---- Panel API -------------------------------------------------------

    def histogram_names(self) -> list[str]:
        return [self._hist_name()]

    def figure_names(self) -> list[str]:
        # Reads the raw TProfile buffers itself and lays the values out
        # spatially, so it never uses the pre-built bar figure.
        return []

    def layout(self) -> html.Div:
        height = self.params.get("height", "420px")
        graph_class = "detector-clickable" if self.link_tab() else ""
        graph = dcc.Graph(
            id={"type": "panel-graph", "panel": self.panel_id, "index": 0},
            figure=theme.placeholder_figure(self._title()),
            style={"height": height},
            className=graph_class,
            config={"displayModeBar": False},
        )
        children: list = []
        if self._mode_toggle:
            children.append(self._mode_controls())
        children.append(graph)
        return html.Div(style={"marginBottom": "12px"}, children=children)

    def _mode_controls(self) -> html.Div:
        return html.Div(
            style={"display": "flex", "alignItems": "center", "gap": "8px", "marginBottom": "8px"},
            children=[
                html.Span("Mode:", style={"color": theme.FG, "fontSize": "13px"}),
                dcc.RadioItems(
                    id={"type": "sipm-detector-mode", "panel": self.panel_id},
                    options=[{"label": m, "value": m} for m in _MODES],
                    value=self._mode,
                    inline=True,
                    labelStyle={"marginRight": "12px", "color": theme.FG},
                ),
                # Sink for the toggle callback (the real state lives in self._mode).
                dcc.Store(id={"type": "sipm-detector-mode-sink", "panel": self.panel_id}),
            ],
        )

    def render(self, figs, payloads, client_state):
        payload = payloads.get(self._hist_name())
        values = _channel_means(payload)
        return [_sipm_figure(values, self._geom(), self._value_label(), self._title())]

    def register_callbacks(self, app: Dash) -> None:
        if not self._mode_toggle:
            return

        @app.callback(
            Output({"type": "sipm-detector-mode-sink", "panel": self.panel_id}, "data"),
            Input({"type": "sipm-detector-mode", "panel": self.panel_id}, "value"),
            prevent_initial_call=True,
        )
        def _on_mode(value):
            # Persist the choice; the next poll fetches only the active variant
            # via histogram_names(). Returning value satisfies Dash's Output rule.
            if value in _MODES:
                self._mode = value
            return value


def _channel_means(payload: Optional[dict]) -> Optional[dict[int, float]]:
    """channel index -> per-channel value, read straight from the buffers.

    For a `TProfile` this is the bin mean (`fArray/fBinEntries`); for a plain
    per-channel `TH1` it is the bin content. Returns None when the payload is
    missing/unusable. A `TProfile` channel with no entries is absent from the
    dict (its mean is undefined); a `TH1` keeps every in-range bin (incl. 0.0).
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
            if idx < len(entries) and entries[idx] > 0:
                means[channel] = sumw[idx] / entries[idx]
        else:
            means[channel] = float(sumw[idx])

    return means


def _build_geometry(info: dict[int, dict]) -> Optional[dict]:
    """Precompute the static spatial layout shared by every poll.

    The grid extent and the per-cell hover text / channel indices are fixed by
    the SiPM mapping, so we build them once. Returns None when there is no
    mapping (the figure then shows a "no channel mapping" placeholder). The
    returned dict holds the `text` (hover) and `customdata` grids and a flat
    list of `(row_idx, col_idx, channel)` placements that `render` walks to
    fill in just the `z` values.
    """
    if not info:
        return None

    all_cols = [d["column"] for d in info.values()]
    all_rows = [d["row"] for d in info.values()]
    columns = list(range(min(all_cols), max(all_cols) + 1))
    rows = list(range(min(all_rows), max(all_rows) + 1))
    col_idx = {c: i for i, c in enumerate(columns)}
    row_idx = {r: i for i, r in enumerate(rows)}

    # Static per-cell hover label (channel/module/type) and the scalar channel
    # index for the click; unmapped cells stay None / "".
    hover: list[list[str]] = [["" for _ in columns] for _ in rows]
    customdata: list[list[Optional[int]]] = [[None] * len(columns) for _ in rows]
    cells: list[tuple[int, int, int]] = []
    for ch, d in info.items():
        ri, ci = row_idx[d["row"]], col_idx[d["column"]]
        hover[ri][ci] = f"module {d['module']} · {d['type']} · ch {ch}"
        customdata[ri][ci] = ch
        cells.append((ri, ci, ch))

    return {"columns": columns, "rows": rows, "hover": hover, "customdata": customdata, "cells": cells}


def _sipm_figure(
    values: Optional[dict[int, float]],
    geom: Optional[dict],
    value_label: str,
    title: str,
) -> go.Figure:
    """The SiPM detector map: three stacked heatmaps over the (row, column)
    fiber grid, each cell coloured by its channel's per-channel value.

    `geom` is the precomputed static layout (see `_build_geometry`); only the
    `z` colours and hover values are rebuilt here, per poll."""
    layout = theme.base_figure_layout(title)

    if geom is None:
        layout["annotations"] = [
            dict(text="no channel mapping", showarrow=False, font=dict(color=theme.WARN, size=14))
        ]
        return go.Figure(layout=layout)

    if values is None:
        layout["title"] = f"{title} (missing)"
        layout["annotations"] = [
            dict(text="missing on server", showarrow=False, font=dict(color=theme.WARN, size=14))
        ]
        return go.Figure(layout=layout)

    columns = geom["columns"]
    rows = geom["rows"]

    # `z` and the per-cell hover value vary poll-to-poll; the static hover label
    # and cell placements come straight from the cached geometry.
    z: list[list[Optional[float]]] = [[None] * len(columns) for _ in rows]
    z_empty: list[list[Optional[float]]] = [[None] * len(columns) for _ in rows]
    # Transparent hit layer: a real (non-gap) value on every mapped cell so the
    # top trace always has a clickable point there — filled or empty.
    z_hit: list[list[Optional[float]]] = [[None] * len(columns) for _ in rows]
    hovertext: list[list[Optional[str]]] = [[None] * len(columns) for _ in rows]
    present_values: list[float] = []
    for ri, ci, channel in geom["cells"]:
        value = values.get(channel)
        label = geom["hover"][ri][ci]
        z[ri][ci] = value
        z_hit[ri][ci] = 0.0
        if value is not None:
            present_values.append(value)
            hovertext[ri][ci] = f"{label}<br>{value_label} {value:.3f}"
        else:
            z_empty[ri][ci] = 0.0
            hovertext[ri][ci] = f"{label}<br>no data"

    cmin, cmax = (min(present_values), max(present_values)) if present_values else (0.0, 1.0)
    if cmin == cmax:
        cmax = cmin + 1.0

    # Values. Empty cells are gaps here -> the grey overlay shows through.
    value_heatmap = go.Heatmap(
        x=columns, y=rows, z=z,
        colorscale=COLORSCALE,
        zmin=cmin, zmax=cmax,
        hoverinfo="skip",
        colorbar=dict(title=value_label, thickness=12),
    )

    # Flat grey tile for mapped cells with no value this poll, so the detector
    # outline stays visible before data arrives.
    empty_overlay = go.Heatmap(
        x=columns, y=rows, z=z_empty,
        colorscale=[[0.0, theme.EMPTY], [1.0, theme.EMPTY]],
        zauto=False, zmin=0.0, zmax=1.0,
        showscale=False,
        hoverinfo="skip",
    )

    # Transparent click/hover catcher on top. `z_hit` is a real value on every
    # mapped cell (no gaps), so each channel is a clickable point whether or not
    # it has data; unmapped cells stay None (inert). The scalar `customdata`
    # holds the channel index the navigation click reads.
    hit_layer = go.Heatmap(
        x=columns, y=rows, z=z_hit,
        text=hovertext,
        customdata=geom["customdata"],
        colorscale=[[0.0, "rgba(0,0,0,0)"], [1.0, "rgba(0,0,0,0)"]],
        zauto=False, zmin=0.0, zmax=1.0,
        showscale=False,
        hoverongaps=False,
        hovertemplate="%{text}<extra></extra>",
    )

    layout["xaxis"] = {
        **layout["xaxis"],
        "title": "fiber column",
        "showgrid": False, "zeroline": False,
    }
    # row increases upward (row 0 at the bottom = physical bottom of the
    # detector), so no autorange reversal. scaleanchor + scaleratio make each
    # 1x1 cell display with the real (wide, short) fiber proportions.
    layout["yaxis"] = {
        **layout["yaxis"],
        "title": "fiber row",
        "showgrid": False, "zeroline": False,
        "scaleanchor": "x", "scaleratio": ROW_PITCH_MM / COL_PITCH_MM,
    }
    # Order = bottom-to-top: grey overlay, value colours, transparent hit layer.
    # The hit layer must be last so it sits on top and receives clicks.
    return go.Figure(data=[empty_overlay, value_heatmap, hit_layer], layout=layout)
