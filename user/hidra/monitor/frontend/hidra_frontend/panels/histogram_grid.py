"""HistogramGridPanel — the default panel: a fixed list of histograms in an N-column grid.

This is the panel type you reference with `type: histograms` in
`config.yaml`. It expects:

    - type: histograms
      cols: 2                                # optional, default 2
      histograms: [hist_name_1, hist_name_2, ...]
      bin_labels: [label0, label1, ...]      # optional categorical x tick labels

It's a thin object: declare which histograms it wants, declare its
Dash layout (one `dcc.Graph` per histogram, arranged in rows of
`cols`), and on each poll receive a `figs` dict and return the figures
in the same order as the graph slots.

`bin_labels` (optional) writes categorical tick labels under the bars
(one per bin, in order) — handy for counter histograms whose bins are
categories rather than a numeric range. It's applied here in the
frontend using the bars' own bin centres as tick positions, so the
backend histogram doesn't need ROOT bin labels.

For anything fancier (channel selector, event display, multi-row
custom layout) write a new Panel subclass — see `panels/base.py` and
the README section "Custom panels".
"""

from __future__ import annotations

import plotly.graph_objects as go
from dash import dcc, html

from .. import theme
from .base import Panel
from .graph_controls import controls_overlay


class HistogramGridPanel(Panel):
    def histogram_names(self) -> list[str]:
        return list(self.params["histograms"])

    def control_indices(self) -> list[int]:
        # Every slot is a 1D bar histogram, so all of them get controls.
        return list(range(len(self.histogram_names())))

    def layout(self) -> html.Div:
        names = self.histogram_names()
        cols = int(self.params.get("cols", 2))

        # Use the static title (params['titles']) when available, else the name.
        titles = self.params.get("titles", {})
        # Each slot is the graph with its hover-reveal controls overlay in
        # the top-right corner (both inside a position:relative .plot-cell).
        graph_slots = [
            html.Div(
                className="plot-cell",
                style={"flex": "1", "minWidth": "0"},
                children=[
                    dcc.Graph(
                        id={"type": "panel-graph", "panel": self.panel_id, "index": i},
                        figure=theme.placeholder_figure(titles.get(name, "Loading " + name)),
                        style={"minWidth": "0"},
                        config={"displayModeBar": False},
                    ),
                    controls_overlay(self.panel_id, i),
                ],
            )
            for i, name in enumerate(names)
        ]

        # Arrange the slots in rows of `cols` items.
        rows = []
        for start in range(0, len(graph_slots), cols):
            row = html.Div(
                style={"display": "flex", "gap": "12px", "marginBottom": "12px"},
                children=graph_slots[start:start + cols],
            )
            rows.append(row)
        return html.Div(rows)

    def render(self, figs, payloads, client_state):
        # We ignore `payloads`: the standard bar-chart figure built by
        # the framework is exactly what we want to show. If a histogram
        # is missing from `figs` we fall back to the styled placeholder
        # so the slot doesn't go blank-white.
        bin_labels = self.params.get("bin_labels")
        out = []
        for n in self.histogram_names():
            fig = figs.get(n)
            if fig is None:
                out.append(theme.placeholder_figure(n))
                continue
            if bin_labels:
                fig = _with_bin_labels(fig, bin_labels)
            out.append(fig)
        return out


def _with_bin_labels(fig, labels):
    """Return `fig` with categorical x tick labels, using the bars' own bin
    centres as the tick positions. Frontend-only: no backend bin labels needed.

    The poll callback builds one Figure per histogram and shares it across every
    panel that references that histogram, so we copy before relabelling to avoid
    leaking the ticks into other panels/tabs that show the same histogram
    without `bin_labels`."""
    if not hasattr(fig, "data"):
        return fig
    centres = None
    for trace in fig.data:
        x = getattr(trace, "x", None)
        if x is not None and len(x):
            centres = list(x)
            break
    if not centres:
        return fig
    k = min(len(centres), len(labels))
    fig = go.Figure(fig)  # copy; leaves the shared original untouched
    fig.update_xaxes(tickmode="array", tickvals=centres[:k], ticktext=list(labels)[:k])
    return fig
