"""Per-plot controls shown above a 1D histogram graph.

A small toolbar with a "log y" toggle and a "reset zoom" button, placed
above each bar-chart graph slot. It is deliberately generic: any panel
whose slots are 1D histograms can call `controls_overlay(panel_id, index)`
and opt in via `Panel.control_indices()`.

The control IDs are pattern-matching dicts so a single callback (and the
poll callback) can address every control at once:

  * ``{"type": "graph-ctl-logy",  "panel": <id>, "index": <i>}`` — a
    `dcc.Checklist`; its value (``["logy"]`` or ``[]``) is read by the
    poll callback, which applies ``yaxis.type="log"`` to that figure.
    `persistence` keeps the toggle across tab switches / reloads.
  * ``{"type": "graph-ctl-reset", "panel": <id>, "index": <i>}`` — a
    button; clicks are accumulated into the ``graph-reset`` store by
    `callbacks/graph_controls.py`. The poll callback folds that counter
    into the figure's ``uirevision`` so a click drops the preserved
    zoom on the next tick.

This module only builds the layout; the behaviour lives in the poll
callback and `callbacks/graph_controls.py`.
"""

from __future__ import annotations

from dash import dcc, html


def logy_id(panel_id: str, index: int) -> dict:
    return {"type": "graph-ctl-logy", "panel": panel_id, "index": index}


def reset_id(panel_id: str, index: int) -> dict:
    return {"type": "graph-ctl-reset", "panel": panel_id, "index": index}


def rebin_id(panel_id: str, index: int) -> dict:
    return {"type": "graph-ctl-rebin", "panel": panel_id, "index": index}


# Rebin factors offered by the segmented control (×1 = no rebin, the default).
REBIN_FACTORS = [1, 2, 4, 8]


def controls_overlay(panel_id: str, index: int, rebin: bool = False, rebin_value: int = 1) -> html.Div:
    """A modebar-style toolbar (log-y toggle + reset-zoom) for one graph slot.

    Rendered as an absolutely-positioned overlay in the plot's top-right
    corner, hidden until the plot is hovered (see `.plot-ctl-overlay` in
    assets/base.css). Place it inside a `.plot-cell` (position: relative)
    container next to the `dcc.Graph`.

    With `rebin=True` a segmented ×1/×2/×4/×8 rebin control is added (initial
    selection `rebin_value`). Only panels that build their own figures from the
    factor (the channel selector) opt in; the others omit it.
    """
    children: list = [
        dcc.Checklist(
            id=logy_id(panel_id, index),
            options=[{"label": " log y", "value": "logy"}],
            value=[],
            persistence=True,
            persistence_type="session",
            className="plot-ctl-logy",
            inputStyle={"marginRight": "3px", "cursor": "pointer"},
            labelStyle={"cursor": "pointer", "margin": "0"},
        ),
    ]
    if rebin:
        children.append(
            dcc.RadioItems(
                id=rebin_id(panel_id, index),
                options=[{"label": f"×{f}", "value": f} for f in REBIN_FACTORS],
                value=rebin_value,
                inline=True,
                className="plot-ctl-rebin",
                # Flush segments: no inter-label margin (CSS draws the dividers).
                labelStyle={"margin": "0"},
            )
        )
    children.append(
        html.Button(
            "⤢ reset",
            id=reset_id(panel_id, index),
            n_clicks=0,
            className="plot-ctl-btn",
        )
    )
    return html.Div(className="plot-ctl-overlay", children=children)
