"""MaskStripPanel — a single-row heatmap of the recent trigger-mask values.

Unlike the generic `histograms` panel (which draws the `trigger_mask_recent`
strip as a bar chart, where each value's *height* encodes the class and gate=0
is an invisible zero-height bar), this panel lays the same strip out as one row
of coloured cells: bin content -> categorical colour. The repeating
`physics … physics pedestal` pattern reads as a band of one colour with a
periodic stripe of another, and any anomaly (`both`, `gate`, a missing/extra
pedestal) is an out-of-place colour that jumps out.

The source histogram is the `trigger_mask_recent` TH1 written by the backend's
`MetaFiller`: bin 1 is the oldest value, the last bin the newest. The bin holds
the trigger *class* encoded as mask+1, so 0 means "no data" (an unfilled slot or
an absent mask), 1 = gate, 2 = physics, 3 = pedestal, 4 = both. We read the bin
contents straight from the TBufferJSON buffers (same trick as `DetectorPanel`)
since all we need is the one value per cell.

Config (in `config.yaml`):

    - type: mask_strip
      histogram: trigger_mask_recent   # optional, this is the default
      title: Recent trigger masks      # optional
      height: 100px                    # optional
"""

from __future__ import annotations

from typing import Optional

import plotly.graph_objects as go
from dash import dcc, html

from .. import theme
from .base import Panel

# Trigger classes, keyed by the strip's stored value (trigger mask + 1, so 0 is
# "no data"). Each gets a discrete colour: physics (the calm bulk) blue, pedestal
# (the periodic marker) yellow, both (an anomaly) red, gate a muted grey, empty
# the background grey.
CATEGORIES = (
    (0, "empty", theme.EMPTY),
    (1, "gate", theme.BORDER),
    (2, "physics", theme.PRIMARY),
    (3, "pedestal", theme.WARN),
    (4, "both", theme.ERR),
)
_VMIN = CATEGORIES[0][0]
_VMAX = CATEGORIES[-1][0]
_NAMES = {value: name for value, name, _ in CATEGORIES}


class MaskStripPanel(Panel):
    def _hist_name(self) -> str:
        return self.params.get("histogram", "trigger_mask_recent")

    def _title(self) -> str:
        return self.params.get("title", "Recent trigger masks")

    def histogram_names(self) -> list[str]:
        return [self._hist_name()]

    def figure_names(self) -> list[str]:
        # We read the raw buffers and build the heatmap ourselves; the pre-built
        # bar figure is never used.
        return []

    def layout(self) -> html.Div:
        height = self.params.get("height", "100px")
        return html.Div(
            className="plot-cell",
            style={"flex": "1", "minWidth": "0", "marginBottom": "12px"},
            children=[
                dcc.Graph(
                    id={"type": "panel-graph", "panel": self.panel_id, "index": 0},
                    figure=theme.placeholder_figure(self._title()),
                    style={"minWidth": "0", "height": height},
                    config={"displayModeBar": False},
                ),
            ],
        )

    def render(self, figs, payloads, client_state):
        payload = payloads.get(self._hist_name())
        return [_strip_figure(_bin_values(payload), self._title())]


def _bin_values(payload: Optional[dict]) -> Optional[list[float]]:
    """The in-range bin contents (bin 1..N), oldest first. None when the payload
    is missing/unusable so the figure can show a placeholder.

    fArray layout is [underflow, bin_1, ..., bin_N, overflow]; the strip stores
    one mask value per bin via SetBinContent, so we just take indices 1..N.
    """
    if not payload or "_typename" not in payload:
        return None
    nbins = payload.get("fXaxis", {}).get("fNbins", 0)
    arr = payload.get("fArray") or []
    if nbins < 1 or len(arr) < nbins + 1:
        return None
    return [float(arr[i]) for i in range(1, nbins + 1)]


def _discrete_colorscale() -> list[list]:
    """Plotly colorscale with one flat band per category, so each integer mask
    value maps to a single solid colour (no interpolation)."""
    n = len(CATEGORIES)
    scale: list[list] = []
    for i, (_value, _name, colour) in enumerate(CATEGORIES):
        scale.append([i / n, colour])
        scale.append([(i + 1) / n, colour])
    return scale


def _strip_figure(values: Optional[list[float]], title: str) -> go.Figure:
    """One-row heatmap of the strip values, each cell coloured by its mask class.

    The colour key is a horizontal legend of square swatches (not a colorbar):
    on a strip this short a vertical colorbar's category labels overlap, while a
    horizontal legend lays them out side by side. The x-axis carries the bin
    index as a visible counter, and the index is repeated in the hover."""
    layout = theme.base_figure_layout(title)
    # A thin single-row strip: x carries the bin index, y is hidden. Compact
    # margins keep the row short; the top margin still fits the title plus the
    # legend stacked under it, the bottom one the x-axis counter.
    layout["margin"] = dict(l=20, r=20, t=48, b=28)
    layout["xaxis"] = {
        **layout["xaxis"],
        "title": "bin index (newest at right)",
        "showgrid": False,
        "zeroline": False,
        "showticklabels": True,
    }
    layout["yaxis"] = {
        **layout["yaxis"],
        "showgrid": False,
        "zeroline": False,
        "showticklabels": False,
    }
    layout["legend"] = dict(
        orientation="h",
        yanchor="bottom",
        y=1.0,
        xanchor="left",
        x=0.0,
        font=dict(size=10),
    )

    if not values:
        layout["annotations"] = [dict(text="missing on server", showarrow=False, font=dict(color=theme.WARN, size=14))]
        return go.Figure(layout=layout)

    x = list(range(len(values)))
    hovertext = [_NAMES.get(int(round(v)), f"{v:g}") for v in values]
    # Centre each integer value in its colour band: span half a unit past the
    # extreme categories so bands are one unit wide and centred on the integers.
    heatmap = go.Heatmap(
        x=x,
        y=[0],
        z=[values],
        text=[hovertext],
        hovertemplate="bin %{x}<br>%{text}<extra></extra>",
        colorscale=_discrete_colorscale(),
        zauto=False,
        zmin=_VMIN - 0.5,
        zmax=_VMAX + 0.5,
        xgap=0,
        ygap=0,
        showscale=False,
    )
    # Dummy invisible traces, one per class, purely to populate a horizontal
    # legend with a coloured square swatch + label (a categorical colour key
    # without a colorbar). They plot nothing (x/y are None) and never hit-test.
    swatches = [
        go.Scatter(
            x=[None],
            y=[None],
            mode="markers",
            marker=dict(symbol="square", size=12, color=colour),
            name=name,
            showlegend=True,
            hoverinfo="skip",
        )
        for _value, name, colour in CATEGORIES
    ]
    return go.Figure(data=[heatmap, *swatches], layout=layout)
