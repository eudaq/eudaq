"""MAXICCDetectorPanel — geometric ADC maps of the MAXICC crystal calorimeter.

MAXICC is read out by the three new FERS boards (local board 0/1/2) and has
three readout planes — front 15 µm, rear 15 µm, rear 50 µm. This panel shows
one gain (HG *or* LG) as **three heatmaps side by side**, one per plane: each
crystal sits at its ``(col, row)`` position, coloured by that channel's mean
ADC. The two gains live in two separate tabs (``maxicc_hg`` / ``maxicc_lg``).

It is the SiPM/detector pattern (`sipm_detector.py`) with two MAXICC twists:

  * the geometry comes from `get_maxicc_channel_info()` — ``{board, channel,
    layer, col, row}`` per channel (local board), grouped here by ``layer``;
  * MAXICC is its own monitor sub-event with dedicated ``MAXICC_*`` histograms
    (3 boards × 64 channels), indexed by the channel ``board * 64 + ch`` with
    the local board 0/1/2 — so the histogram index is
    ``(board_offset + board) * 64 + ch`` with ``board_offset = 0``. (The offset
    is kept configurable only for unusual wirings; it is normally 0.)

Config (in ``config.yaml``)::

    - type: maxicc_detector
      histogram: MAXICC_HG_mean   # base; mode_toggle appends _physics/_pedestal
      label: "mean HG"
      mode_toggle: true           # physics/pedestal radio
      board_offset: 0             # optional, default 0
      height: 360px

Each plane is the same three stacked heatmaps as the SiPM map (grey empty
overlay + Viridis values + transparent hover layer); row 0 is drawn at the top.
"""

from __future__ import annotations

from typing import Optional

import plotly.graph_objects as go
from dash import Dash, Input, Output, dcc, html

from .. import theme
from ..mapping import get_maxicc_channel_info
from .base import Panel
from .sipm_detector import _channel_means

COLORSCALE = "Viridis"
_MODES = ("physics", "pedestal")

# Readout planes in display order: (layer index in the mapping, title).
LAYERS = [(0, "front 15µm"), (1, "rear 15µm"), (2, "rear 50µm")]

DEFAULT_BOARD_OFFSET = 0


class MAXICCDetectorPanel(Panel):
    def __init__(self, panel_id, params):
        super().__init__(panel_id, params)
        self._mode_toggle = bool(params.get("mode_toggle", False))
        self._mode = "physics"
        # Global index of the first MAXICC board (wiring-dependent, not geometry).
        self._board_offset = int(params.get("board_offset", DEFAULT_BOARD_OFFSET))
        # Static per-layer spatial layout (grid + per-cell global index/hover),
        # built once from the mapping and reused every poll — only z changes.
        self._geoms: Optional[dict[int, dict]] = None

    def _geometries(self) -> dict[int, dict]:
        if self._geoms is None:
            self._geoms = _build_layer_geometries(get_maxicc_channel_info(), self._board_offset)
        return self._geoms

    # ---- config helpers --------------------------------------------------

    def _base_hist(self) -> str:
        default = "MAXICC_HG_mean" if self._mode_toggle else "MAXICC_HG_mean_physics"
        name = self.params.get("histogram", default)
        # With the toggle on we append the active suffix ourselves; tolerate a
        # base that already carries one so we never request `..._physics_physics`.
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
        """Tab id to open when a cell is clicked (None = not clickable). The
        clicked cell carries the global FERS channel index as customdata, which
        the navigation callback feeds to that tab's channel selector."""
        return self.params.get("link_tab")

    def _mode_suffix(self) -> str:
        if self._mode_toggle:
            return f" ({self._mode})"
        name = self._hist_name()
        if name.endswith("_physics"):
            return " (physics)"
        if name.endswith("_pedestal"):
            return " (pedestal)"
        return ""

    # ---- Panel API -------------------------------------------------------

    def histogram_names(self) -> list[str]:
        # One per-channel histogram; the three planes read it at different indices.
        return [self._hist_name()]

    def figure_names(self) -> list[str]:
        # Reads the raw TProfile buffer itself and lays values out spatially.
        return []

    def layout(self) -> html.Div:
        height = self.params.get("height", "360px")
        graph_class = "detector-clickable" if self.link_tab() else ""
        slots = [
            html.Div(
                className="plot-cell",
                style={"flex": "1", "minWidth": "0"},
                children=[
                    dcc.Graph(
                        id={"type": "panel-graph", "panel": self.panel_id, "index": i},
                        figure=theme.placeholder_figure(name),
                        style={"height": height},
                        className=graph_class,
                        config={"displayModeBar": False},
                    )
                ],
            )
            for i, (_layer, name) in enumerate(LAYERS)
        ]
        row = html.Div(
            style={"display": "flex", "gap": "12px", "marginBottom": "12px"},
            children=slots,
        )
        children: list = []
        if self._mode_toggle:
            children.append(self._mode_controls())
        children.append(row)
        return html.Div(children=children)

    def _mode_controls(self) -> html.Div:
        return html.Div(
            style={"display": "flex", "alignItems": "center", "gap": "8px", "marginBottom": "8px"},
            children=[
                html.Span("Mode:", style={"color": theme.FG, "fontSize": "13px"}),
                dcc.RadioItems(
                    id={"type": "maxicc-detector-mode", "panel": self.panel_id},
                    options=[{"label": m, "value": m} for m in _MODES],
                    value=self._mode,
                    inline=True,
                    labelStyle={"marginRight": "12px", "color": theme.FG},
                ),
                # Sink for the toggle callback (the real state lives in self._mode).
                dcc.Store(id={"type": "maxicc-detector-mode-sink", "panel": self.panel_id}),
            ],
        )

    def render(self, figs, payloads, client_state):
        values = _channel_means(payloads.get(self._hist_name()))
        geoms = self._geometries()
        label = self._value_label()
        suffix = self._mode_suffix()
        return [
            _maxicc_layer_figure(values, geoms.get(layer), f"{name}{suffix}", label)
            for layer, name in LAYERS
        ]

    def register_callbacks(self, app: Dash) -> None:
        if not self._mode_toggle:
            return

        @app.callback(
            Output({"type": "maxicc-detector-mode-sink", "panel": self.panel_id}, "data"),
            Input({"type": "maxicc-detector-mode", "panel": self.panel_id}, "value"),
            prevent_initial_call=True,
        )
        def _on_mode(value):
            # Persist the choice; the next poll fetches only the active variant.
            if value in _MODES:
                self._mode = value
            return value


def _build_layer_geometries(info: list[dict[str, int]], board_offset: int) -> dict[int, dict]:
    """Per-layer static layout: grid extent, hover labels, and the global FERS
    index of each cell, so render only fills the z colours.

    Each cell is ``(row_index, col_index, global_channel_index)`` where the
    global index is ``(board_offset + board) * 64 + channel``.
    """
    by_layer: dict[int, list[dict[str, int]]] = {}
    for rec in info:
        by_layer.setdefault(rec["layer"], []).append(rec)

    geoms: dict[int, dict] = {}
    for layer, recs in by_layer.items():
        columns = list(range(min(r["col"] for r in recs), max(r["col"] for r in recs) + 1))
        rows = list(range(min(r["row"] for r in recs), max(r["row"] for r in recs) + 1))
        col_idx = {c: i for i, c in enumerate(columns)}
        row_idx = {r: i for i, r in enumerate(rows)}

        hover: list[list[str]] = [["" for _ in columns] for _ in rows]
        # Per-cell global FERS index for the click navigation (None where unmapped).
        customdata: list[list[Optional[int]]] = [[None] * len(columns) for _ in rows]
        cells: list[tuple[int, int, int]] = []
        for rec in recs:
            ri, ci = row_idx[rec["row"]], col_idx[rec["col"]]
            global_idx = (board_offset + rec["board"]) * 64 + rec["channel"]
            # Show both the local (board, channel) and the global FERS index the
            # cell maps to (= the navigation target), so the offset is visible.
            hover[ri][ci] = (
                f"B{rec['board']}·CH{rec['channel']} → FERS ch {global_idx} "
                f"(col {rec['col']}, row {rec['row']})"
            )
            customdata[ri][ci] = global_idx
            cells.append((ri, ci, global_idx))

        geoms[layer] = {
            "columns": columns, "rows": rows,
            "hover": hover, "customdata": customdata, "cells": cells,
        }
    return geoms


def _maxicc_layer_figure(
    values: Optional[dict[int, float]],
    geom: Optional[dict],
    title: str,
    value_label: str,
) -> go.Figure:
    """One readout plane: three stacked heatmaps over the (col, row) crystal grid,
    each cell coloured by its channel's mean ADC (grey when no data this poll)."""
    layout = theme.base_figure_layout(title)

    if geom is None:
        layout["annotations"] = [
            dict(text="no channel mapping", showarrow=False, font=dict(color=theme.WARN, size=14))
        ]
        return go.Figure(layout=layout)

    columns = geom["columns"]
    rows = geom["rows"]
    z: list[list[Optional[float]]] = [[None] * len(columns) for _ in rows]
    z_empty: list[list[Optional[float]]] = [[None] * len(columns) for _ in rows]
    z_hit: list[list[Optional[float]]] = [[None] * len(columns) for _ in rows]
    hovertext: list[list[Optional[str]]] = [[None] * len(columns) for _ in rows]
    present: list[float] = []
    for ri, ci, global_idx in geom["cells"]:
        value = values.get(global_idx) if values else None
        label = geom["hover"][ri][ci]
        z_hit[ri][ci] = 0.0
        if value is not None:
            z[ri][ci] = value
            present.append(value)
            hovertext[ri][ci] = f"{label}<br>{value_label} {value:.3f}"
        else:
            z_empty[ri][ci] = 0.0
            hovertext[ri][ci] = f"{label}<br>no data"

    cmin, cmax = (min(present), max(present)) if present else (0.0, 1.0)
    if cmin == cmax:
        cmax = cmin + 1.0

    value_heatmap = go.Heatmap(
        x=columns, y=rows, z=z,
        colorscale=COLORSCALE, zmin=cmin, zmax=cmax,
        hoverinfo="skip",
        colorbar=dict(title=value_label, thickness=12),
    )
    empty_overlay = go.Heatmap(
        x=columns, y=rows, z=z_empty,
        colorscale=[[0.0, theme.EMPTY], [1.0, theme.EMPTY]],
        zauto=False, zmin=0.0, zmax=1.0, showscale=False, hoverinfo="skip",
    )
    hit_layer = go.Heatmap(
        x=columns, y=rows, z=z_hit,
        text=hovertext,
        customdata=geom["customdata"],
        colorscale=[[0.0, "rgba(0,0,0,0)"], [1.0, "rgba(0,0,0,0)"]],
        zauto=False, zmin=0.0, zmax=1.0, showscale=False,
        hoverongaps=False, hovertemplate="%{text}<extra></extra>",
    )

    layout["xaxis"] = {**layout["xaxis"], "title": "col", "showgrid": False, "zeroline": False}
    # row 0 at the top (as drawn) -> reversed y; square crystals -> 1:1 aspect.
    layout["yaxis"] = {
        **layout["yaxis"], "title": "row", "showgrid": False, "zeroline": False,
        "autorange": "reversed", "scaleanchor": "x", "scaleratio": 1,
    }
    return go.Figure(data=[empty_overlay, value_heatmap, hit_layer], layout=layout)
