"""TrackerPanel — 2D hit maps of the tracker stations + X/Y projections.

Each tracker station is published by the backend as a `TH2` (`Tracker_station<i>`,
x = X coordinate, y = Y coordinate). This panel draws:

  * one Plotly `Heatmap` per station (arranged in a grid of `cols` columns), and
  * optionally (``projections: true``, the default) two overlay plots below the
    heatmaps — the X distribution and the Y distribution — each with **one line
    per station** superimposed. The distributions are the projections of the
    `TH2` onto each axis (sum over the other axis), so they need no extra backend
    histograms. Overlaying assumes all stations share the same axis range/binning.

Config (in `config.yaml`):

    - type: tracker
      histograms: [Tracker_station0, Tracker_station1, Tracker_station2]
      cols: 3              # optional, default 2 (heatmaps per row)
      height: 420px        # optional, heatmap height
      equal_aspect: true   # optional, lock a 1:1 x/y pixel ratio on the maps
      projections: true    # optional, default true (X/Y overlay row)
      projection_height: 320px  # optional, projection-plot height

Like `DetectorPanel`, this panel reads the raw ROOT buffers of the `TH2`
directly (the 1D figure builder / pure decoder don't handle 2D histograms), so
it sets `figure_names() -> []` and builds the figures itself from `payloads`.
"""

from __future__ import annotations

from typing import Optional

import numpy as np
import plotly.graph_objects as go
from dash import dcc, html

from .. import theme
from .base import Panel

COLORSCALE = "Viridis"

# Titles of the two projection-overlay plots, in the order their graph slots are
# created (after the per-station heatmaps).
_PROJECTION_TITLES = ("X distribution — all stations", "Y distribution — all stations")


class TrackerPanel(Panel):
    def histogram_names(self) -> list[str]:
        return list(self.params.get("histograms", []))

    def figure_names(self) -> list[str]:
        # We read the raw TH2 buffers and build heatmaps/projections ourselves;
        # the standard 1D bar figure is never used for these.
        return []

    def _show_projections(self) -> bool:
        return bool(self.params.get("projections", True))

    def layout(self) -> html.Div:
        names = self.histogram_names()
        cols = int(self.params.get("cols", 2))
        height = self.params.get("height", "420px")

        heatmaps = [
            dcc.Graph(
                id={"type": "panel-graph", "panel": self.panel_id, "index": i},
                figure=theme.placeholder_figure("Loading " + name),
                style={"flex": "1", "minWidth": "0", "height": height},
                config={"displayModeBar": False},
            )
            for i, name in enumerate(names)
        ]

        rows = []
        for start in range(0, len(heatmaps), cols):
            rows.append(
                html.Div(
                    style={"display": "flex", "gap": "12px", "marginBottom": "12px"},
                    children=heatmaps[start:start + cols],
                )
            )

        if self._show_projections() and names:
            proj_height = self.params.get("projection_height", "320px")
            proj_slots = [
                dcc.Graph(
                    id={"type": "panel-graph", "panel": self.panel_id, "index": len(names) + k},
                    figure=theme.placeholder_figure(title),
                    style={"flex": "1", "minWidth": "0", "height": proj_height},
                    config={"displayModeBar": False},
                )
                for k, title in enumerate(_PROJECTION_TITLES)
            ]
            rows.append(
                html.Div(
                    style={"display": "flex", "gap": "12px", "marginBottom": "12px"},
                    children=proj_slots,
                )
            )

        return html.Div(rows)

    def render(self, figs, payloads, client_state):
        names = self.histogram_names()
        equal_aspect = bool(self.params.get("equal_aspect", False))

        decoded = [(name, _decode_th2(payloads.get(name))) for name in names]

        out = [_heatmap_figure(d, name, equal_aspect) for name, d in decoded]
        if self._show_projections() and names:
            out.append(_projection_overlay(decoded, "x"))
            out.append(_projection_overlay(decoded, "y"))
        return out


def _decode_th2(payload: Optional[dict]) -> Optional[dict]:
    """Read a ROOT TH2 JSON payload into x/y bin centres and a 2D `z` grid.

    ROOT stores the bin contents in `fArray` as a flat, row-major
    `(ny+2) x (nx+2)` grid including under/overflow on both axes; the content
    of bin `(ix, iy)` (1-based) is `fArray[iy*(nx+2) + ix]`. Returns None when
    the payload is missing or not a usable TH2.
    """
    if not payload or not str(payload.get("_typename", "")).startswith("TH2"):
        return None

    xaxis = payload.get("fXaxis", {})
    yaxis = payload.get("fYaxis", {})
    nx = int(xaxis.get("fNbins", 0))
    ny = int(yaxis.get("fNbins", 0))
    arr = payload.get("fArray") or []
    if nx < 1 or ny < 1 or len(arr) < (nx + 2) * (ny + 2):
        return None

    grid = np.asarray(arr, dtype=np.float64)[: (nx + 2) * (ny + 2)].reshape(ny + 2, nx + 2)
    z = grid[1:ny + 1, 1:nx + 1]  # drop under/overflow on both axes -> (ny, nx)

    return {
        "x": _centres(xaxis, nx),
        "y": _centres(yaxis, ny),
        "z": z,
        "title": payload.get("fTitle", ""),
        "xtitle": xaxis.get("fTitle", ""),
        "ytitle": yaxis.get("fTitle", ""),
    }


def _centres(axis: dict, n: int) -> np.ndarray:
    """Bin centres for an axis (handles fixed and variable binning)."""
    xbins = axis.get("fXbins") or []
    if xbins:
        edges = np.asarray(xbins, dtype=np.float64)
    else:
        edges = np.linspace(axis["fXmin"], axis["fXmax"], n + 1)
    return 0.5 * (edges[:-1] + edges[1:])


def _station_label(name: str) -> str:
    """Short legend label for a station histogram (`Tracker_station0` -> `station0`)."""
    return name[len("Tracker_"):] if name.startswith("Tracker_") else name


def _heatmap_figure(decoded: Optional[dict], name: str, equal_aspect: bool) -> go.Figure:
    title = (decoded or {}).get("title") or name
    layout = theme.base_figure_layout(title)

    if decoded is None:
        layout["title"] = f"{title} (missing)"
        layout["annotations"] = [dict(text="missing on server", showarrow=False, font=dict(color=theme.WARN, size=14))]
        return go.Figure(layout=layout)

    heatmap = go.Heatmap(
        x=decoded["x"], y=decoded["y"], z=decoded["z"],
        colorscale=COLORSCALE,
        colorbar=dict(title="entries", thickness=12),
        hovertemplate=(
            f"{decoded['xtitle'] or 'X'}=%{{x}}<br>"
            f"{decoded['ytitle'] or 'Y'}=%{{y}}<br>entries=%{{z}}<extra></extra>"
        ),
    )

    layout["xaxis"] = {**layout["xaxis"], "title": decoded["xtitle"] or "X", "zeroline": False}
    layout["yaxis"] = {**layout["yaxis"], "title": decoded["ytitle"] or "Y", "zeroline": False}
    if equal_aspect:
        layout["yaxis"]["scaleanchor"] = "x"
        layout["yaxis"]["scaleratio"] = 1

    return go.Figure(data=[heatmap], layout=layout)


def _projection_overlay(decoded: list[tuple[str, Optional[dict]]], axis: str) -> go.Figure:
    """One overlay plot of the per-station `TH2` projections onto `axis`.

    `axis` is "x" (sum the z grid over y -> counts per X bin) or "y" (sum over x
    -> counts per Y bin). One step-line trace per station; the legend toggles
    them. Assumes a shared axis range/binning across stations.
    """
    nice = "X" if axis == "x" else "Y"
    layout = theme.base_figure_layout(f"{nice} distribution — all stations")

    traces = []
    axis_title = nice
    # Colour by station *position* (not by filtered trace order) so each station
    # keeps the same colour across the X and Y overlays even if some are missing.
    for i, (name, d) in enumerate(decoded):
        if d is None:
            continue
        if axis == "x":
            proj = d["z"].sum(axis=0)  # over y -> length nx
            centres = d["x"]
            axis_title = d["xtitle"] or "X"
        else:
            proj = d["z"].sum(axis=1)  # over x -> length ny
            centres = d["y"]
            axis_title = d["ytitle"] or "Y"
        traces.append(
            go.Scatter(
                x=list(np.asarray(centres)), y=list(np.asarray(proj)),
                mode="lines", line_shape="hvh",
                line=dict(color=theme.PALETTE[i % len(theme.PALETTE)]),
                name=_station_label(name),
            )
        )

    if not traces:
        layout["annotations"] = [dict(text="missing on server", showarrow=False, font=dict(color=theme.WARN, size=14))]
        return go.Figure(layout=layout)

    layout["xaxis"] = {**layout["xaxis"], "title": axis_title}
    layout["yaxis"] = {**layout["yaxis"], "title": "entries"}
    layout["showlegend"] = True
    return go.Figure(data=traces, layout=layout)
