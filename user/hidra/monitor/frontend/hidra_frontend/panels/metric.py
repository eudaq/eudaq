"""MetricPanel — show a histogram's content as a single big number.

Useful for histograms that are really just a counter (e.g.
`event_count`, which is a TH1I with one bin). Instead of drawing a
trivial bar chart, we display the bin content as a "scorecard" using
Plotly's `Indicator` trace.

Config (in `config.yaml`):

    - type: metric
      histograms: [event_count, some_counter]

Behaviour:
  * For a 1-bin TH1: the number shown is that single bin's value.
  * For a multi-bin TH1: the sum of all in-range bins (under/overflow
    excluded). This is a sensible default for counters; if you need
    something else, copy this panel and adapt `_extract_value`.
"""

from __future__ import annotations

from typing import Optional

import plotly.graph_objects as go
from dash import dcc, html

from .. import theme
from .base import Panel


class MetricPanel(Panel):
    def histogram_names(self) -> list[str]:
        return list(self.params.get("histograms", []))

    def figure_names(self) -> list[str]:
        # We read the raw payload (bin content) directly, never the
        # pre-built bar figure — so don't build any.
        return []

    def layout(self) -> html.Div:
        names = self.histogram_names()
        cards = [
            dcc.Graph(
                id={"type": "panel-graph", "panel": self.panel_id, "index": i},
                figure=_indicator_figure(name, None),
                # Cards are short and live side by side in a row.
                style={"flex": "1", "minWidth": "0", "height": "160px"},
                config={"displayModeBar": False},
            )
            for i, name in enumerate(names)
        ]
        return html.Div(
            style={"display": "flex", "gap": "12px", "marginBottom": "12px"},
            children=cards,
        )

    def render(self, figs, payloads, client_state):
        return [
            _indicator_figure(name, _extract_value(payloads.get(name)))
            for name in self.histogram_names()
        ]


def _extract_value(payload: Optional[dict]) -> Optional[float]:
    """Pull a scalar value out of a TBufferJSON histogram payload.

    We read `fArray` directly — same format the pure decoder uses,
    but we only need one number so we skip the full DecodedHist path.
    Returns None when the payload is missing or malformed.
    """
    if not payload or "_typename" not in payload:
        return None
    arr = payload.get("fArray") or []
    nbins = payload.get("fXaxis", {}).get("fNbins", 0)
    if not arr or nbins < 1:
        return None
    # fArray layout: [underflow, bin_1, ..., bin_N, overflow]. We sum
    # the in-range bins.
    return float(sum(arr[1:nbins + 1]))


def _indicator_figure(name: str, value: Optional[float]) -> go.Figure:
    """Build the Plotly figure for one metric card."""
    layout = dict(
        margin=dict(l=10, r=10, t=10, b=10),
        paper_bgcolor=theme.BG,
        plot_bgcolor=theme.BG,
        font=dict(color=theme.FG, family=theme.FONT_FAMILY),
        # Keep the per-card uirevision so Plotly doesn't reset any UI
        # state we might add in the future (mode switches etc.).
        uirevision=name,
    )
    if value is None:
        layout["annotations"] = [
            dict(
                text="(missing)",
                showarrow=False,
                font=dict(color=theme.WARN, size=12),
                xref="paper", yref="paper", x=0.5, y=0.05,
            )
        ]
    indicator = go.Indicator(
        mode="number",
        value=(value if value is not None else 0),
        # Font size 40 (not 56): a 6-digit grouped value like "100,000" fits the
        # ~200px-wide card without overflowing sideways, while staying fixed so a
        # small value (e.g. "98") doesn't auto-scale up and shove the title off
        # the top of the card.
        number={"font": {"size": 40, "color": theme.PRIMARY}, "valueformat": ",.0f"},
        title={"text": name, "font": {"size": 16, "color": theme.FG}},
    )
    # Single-shot construction (data=/layout=) avoids the validation cost
    # of go.Figure()+update_layout().
    return go.Figure(data=[indicator], layout=layout)
