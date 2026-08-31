"""Per-plot controls callbacks.

Only "reset zoom" needs server state: each click bumps a per-graph
counter kept in the ``graph-reset`` store. The poll callback folds that
counter into the figure's ``uirevision`` (``"<panel>|<index>|<counter>"``)
so the preserved zoom is dropped on the next tick after a click.

The "log y" toggle needs no callback here: the poll callback reads the
checklist values directly (as `State`) and applies the axis type.
"""

from __future__ import annotations

from dash import ALL, Dash, Input, Output, State, ctx, no_update


def reset_key(panel_id: str, index: int) -> str:
    return f"{panel_id}|{index}"


def register(app: Dash) -> None:
    @app.callback(
        Output("graph-reset", "data"),
        Input({"type": "graph-ctl-reset", "panel": ALL, "index": ALL}, "n_clicks"),
        State("graph-reset", "data"),
        prevent_initial_call=True,
    )
    def on_reset(_n_clicks, data):
        trigger = ctx.triggered_id
        if not isinstance(trigger, dict):
            return no_update
        key = reset_key(trigger["panel"], trigger["index"])
        data = dict(data or {})
        data[key] = data.get(key, 0) + 1
        return data
