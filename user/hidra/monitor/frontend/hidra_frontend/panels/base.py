"""Panel ABC.

A Panel is the unit of layout inside a Tab. Each panel:

  * declares which histograms it wants the poll callback to fetch this
    tick (`histogram_names()`),
  * builds its Dash component tree (`layout()`),
  * optionally registers its own callbacks at app startup
    (`register_callbacks()`),
  * receives a dict of fetched data and produces Plotly figures
    (`render()`).

Custom layouts and custom behaviour for a tab are implemented as
custom Panel subclasses. Register them in `panels/__init__.py`
(`PANEL_TYPES`) and reference them by `type` in `config.yaml`.
"""

from __future__ import annotations

from abc import ABC, abstractmethod
from typing import Any, Optional

import plotly.graph_objects as go
from dash import Dash, html


class Panel(ABC):
    def __init__(self, panel_id: str, params: dict[str, Any]) -> None:
        self.panel_id = panel_id
        self.params = params

    @abstractmethod
    def histogram_names(self) -> list[str]:
        """Backend histograms this panel needs on every poll."""

    @abstractmethod
    def layout(self) -> html.Div:
        """Static Dash layout for the panel (graph slots and controls)."""

    @abstractmethod
    def render(
        self,
        figs: dict[str, go.Figure],
        payloads: dict[str, Optional[dict]],
        client_state: dict,
    ) -> list[go.Figure]:
        """Return the figures in the order matching the `panel-graph` indices in `layout()`.

        * `figs` maps histogram name -> already-rendered Plotly figure
          (bar chart, with overlay applied if active). Most panels just
          forward these as-is.
        * `payloads` maps histogram name -> raw TBufferJSON dict (or
          None if missing on the server). Panels that need to read
          individual bin contents — e.g. a metric card showing a
          single number — use this directly.
        * `client_state` is the global `dcc.Store` payload (overlay
          file, paused flag, ...).
        """

    def figure_names(self) -> list[str]:
        """Subset of `histogram_names()` this panel renders from the
        pre-built `figs` dict (vs. reading the raw `payloads`).

        The poll callback builds a Plotly figure only for these names, so
        panels that consume the raw payload directly (metric, detector)
        override this to `[]` and avoid the figure-construction cost.
        Default: everything the panel fetches.
        """
        return self.histogram_names()

    def control_indices(self) -> list[int]:
        """Graph-slot indices that carry per-plot controls (log-y, reset zoom).

        Default: no controls. Panels whose slots are 1D bar histograms
        (e.g. grid, channel_selector) override this to opt in. The poll
        callback uses the returned indices to know which figures should
        get a persistent `uirevision` and have the controls applied.
        """
        return []

    def register_callbacks(self, app: Dash) -> None:
        """Optional: panels with their own widgets register callbacks here."""
        return None
