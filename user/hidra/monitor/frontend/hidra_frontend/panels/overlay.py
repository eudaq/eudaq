"""OverlayPanel — several histograms superimposed in one plot.

Unlike `histograms` (one graph slot per histogram), this panel draws a
*group* of histograms as overlaid line traces in a single graph, with a
legend whose entries toggle each series on/off (Plotly native; all
visible by default).

It supports one group or several groups laid out in a row (like the
`histograms` panel's `cols`), so related overlays sit side by side — e.g.
the scintillator (S) and Cherenkov (C) sums on one row.

Config (in `config.yaml`)::

    # single group
    - type: overlay
      histograms: [ADC_noise_pedestal, ADC_noise_std_pedestal]
      labels: ["IQR/1.349", "std"]   # optional, default = histogram names
      title: "Pedestal noise per channel"   # optional
      per_channel: true              # optional; x is a channel index ->
                                     #   "ch N" hover and markers per point
      show_flow: true                # optional; append the overflow / prepend
                                     #   the underflow bin just outside the range

    # several groups on one row
    - type: overlay
      cols: 2                        # optional, default = number of groups
      groups:
        - title: "Sum over scintillator PMTs"
          histograms: [sum_PMT_S, sum_PMT_S_physics, sum_PMT_S_pedestal]
          labels: ["total", "physics", "pedestal"]
        - title: "Sum over Cherenkov PMTs"
          histograms: [sum_PMT_C, sum_PMT_C_physics, sum_PMT_C_pedestal]
          labels: ["total", "physics", "pedestal"]

It builds its own figures from the raw payloads (like `detector`), so it
needs its own decoder.
"""

from __future__ import annotations

import logging

from dash import dcc, html

from .. import theme
from ..decoders import get_decoder
from ..figure_builder import overlay_figure
from .base import Panel
from .graph_controls import controls_overlay

logger = logging.getLogger(__name__)

# Colour cycle for the overlaid series. The first (primary/robust) series
# gets the accent blue; the rest are visually distinct.
_COLORS = [theme.PRIMARY, theme.REFERENCE, theme.SECONDARY, theme.ACCENT, theme.ERR]


class OverlayPanel(Panel):
    def __init__(self, panel_id, params):
        super().__init__(panel_id, params)
        self._decoder = get_decoder(params.get("decoder", "pure"))
        self._per_channel = bool(params.get("per_channel", False))
        self._show_flow = bool(params.get("show_flow", False))
        self._height = params.get("height", "320px")
        # One group from the top-level keys, or several from `groups`.
        raw_groups = params.get("groups")
        sources = raw_groups if raw_groups else [params]
        self._groups = [self._make_group(cfg) for cfg in sources]
        self._cols = int(params.get("cols", len(self._groups)))

    def _make_group(self, cfg: dict) -> dict:
        histos = list(cfg.get("histograms", []))
        # Pad/truncate labels to match the histograms instead of letting a later
        # zip() silently drop series on a misconfigured `labels`: warn on a length
        # mismatch and fall back to the histogram name.
        labels = cfg.get("labels") or []
        if labels and len(labels) != len(histos):
            logger.warning(
                "overlay panel %s: %d labels for %d histograms; padding with names",
                self.panel_id, len(labels), len(histos),
            )
        labels = [labels[i] if i < len(labels) else name for i, name in enumerate(histos)]
        title = cfg.get("title", histos[0] if histos else "overlay")
        return {"histos": histos, "labels": labels, "title": title}

    def histogram_names(self) -> list[str]:
        # Flatten across groups, preserving order and dropping duplicates so the
        # poll fetches each backend histogram once.
        seen: dict[str, None] = {}
        for g in self._groups:
            for name in g["histos"]:
                seen.setdefault(name, None)
        return list(seen)

    def figure_names(self) -> list[str]:
        # Builds its own overlaid figures from the raw payloads.
        return []

    def control_indices(self) -> list[int]:
        return list(range(len(self._groups)))

    def layout(self) -> html.Div:
        slots = [
            html.Div(
                className="plot-cell",
                style={"flex": "1", "minWidth": "0"},
                children=[
                    dcc.Graph(
                        id={"type": "panel-graph", "panel": self.panel_id, "index": i},
                        figure=theme.placeholder_figure(g["title"]),
                        style={"minWidth": "0", "height": self._height},
                        config={"displayModeBar": False},
                    ),
                    controls_overlay(self.panel_id, i),
                ],
            )
            for i, g in enumerate(self._groups)
        ]
        # Arrange the slots in rows of `cols` items (one row when cols >= groups).
        rows = [
            html.Div(
                style={"display": "flex", "gap": "12px", "marginBottom": "12px"},
                children=slots[start:start + self._cols],
            )
            for start in range(0, len(slots), self._cols)
        ]
        return html.Div(rows)

    def render(self, figs, payloads, client_state):
        out = []
        for g in self._groups:
            specs = [
                (payloads.get(name), _COLORS[i % len(_COLORS)], label)
                for i, (name, label) in enumerate(zip(g["histos"], g["labels"]))
            ]
            out.append(
                overlay_figure(
                    self._decoder, specs, g["title"], per_channel=self._per_channel, show_flow=self._show_flow
                )
            )
        return out
