"""Cross-tab navigation: click a detector module -> open its ADC channel.

The detector map (`DetectorPanel`) draws one cell per PMT module, each
carrying its ADC channel index as Plotly ``customdata``. A detector panel
configured with ``link_tab: <tab id>`` becomes clickable: clicking a cell

  1. tells the `ChannelSelectorPanel` in the linked tab to select that
     channel (`select_channel`), and
  2. switches the active tab to the linked tab.

Switching the tab makes `update_tab_content` rebuild that tab's DOM; the
selector's dropdown is laid out with the channel we just set, and the
poll callback then fetches and draws that channel's histogram.

All detector graphs share the generic ``panel-graph`` component id (the
same one the poll callback writes figures into), so we listen on every
``panel-graph``'s ``clickData`` and use the registry built here to keep
only the clicks coming from a *clickable detector* panel.
"""

from __future__ import annotations

import logging

from dash import ALL, Dash, Input, Output, ctx, no_update

from ..panels.base import Panel
from ..panels.channel_selector import ChannelSelectorPanel
from ..panels.detector import DetectorPanel
from ..panels.fers_board import FERSBoardPanel
from ..panels.maxicc_detector import MAXICCDetectorPanel
from ..panels.sipm_detector import SiPMDetectorPanel

logger = logging.getLogger(__name__)

# Panel types whose cells carry a channel index as Plotly customdata and can
# link to a channel selector. The calo `DetectorPanel`, the per-board
# `FERSBoardPanel`, the `SiPMDetectorPanel` and the `MAXICCDetectorPanel` share
# the same click contract.
_CLICKABLE_MAPS = (DetectorPanel, FERSBoardPanel, SiPMDetectorPanel, MAXICCDetectorPanel)


def register(app: Dash, panels_by_tab: dict[str, list[Panel]]) -> None:
    # panel_id of a clickable map -> (target tab id, its selector panel).
    nav: dict[str, tuple[str, ChannelSelectorPanel]] = {}
    for panels in panels_by_tab.values():
        for panel in panels:
            if not isinstance(panel, _CLICKABLE_MAPS):
                continue
            target = panel.link_tab()
            if not target:
                continue
            # Pair the map with the selector of the same gain when it exposes
            # one (FERS HG/LG), else the first selector in the target tab.
            gain = panel.gain_tag() if hasattr(panel, "gain_tag") else None
            selector = _find_channel_selector(panels_by_tab.get(target, []), gain)
            if selector is None:
                logger.warning(
                    "map panel %s has link_tab=%r but that tab has no matching "
                    "channel_selector panel; clicks will be ignored",
                    panel.panel_id, target,
                )
                continue
            nav[panel.panel_id] = (target, selector)

    if not nav:
        return

    @app.callback(
        Output("tabs", "value"),
        Input({"type": "panel-graph", "panel": ALL, "index": ALL}, "clickData"),
        prevent_initial_call=True,
    )
    def on_module_click(_all_click_data):
        trigger = ctx.triggered_id
        if not isinstance(trigger, dict):
            return no_update
        entry = nav.get(trigger.get("panel"))
        if entry is None:
            return no_update

        click_data = ctx.triggered[0]["value"]
        points = (click_data or {}).get("points") or []
        if not points:
            return no_update
        # Both clickable maps carry the channel index as a scalar `customdata`
        # (the detector map keeps its hover label separately, in `text`). A
        # heatmap only serialises a *scalar* customdata into clickData, so this
        # is always a bare number here.
        channel = points[0].get("customdata")
        if channel is None:  # clicked an unmapped cell (no module there)
            return no_update

        target_tab, selector = entry
        selector.select_channel(int(channel))
        return target_tab


def _find_channel_selector(panels: list[Panel], gain: str | None = None) -> ChannelSelectorPanel | None:
    selectors = [p for p in panels if isinstance(p, ChannelSelectorPanel)]
    if gain is not None:
        for sel in selectors:
            if sel.gain_tag() == gain:
                return sel
    return selectors[0] if selectors else None
