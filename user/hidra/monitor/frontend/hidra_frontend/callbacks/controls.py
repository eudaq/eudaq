"""Callbacks for the top-bar controls: Pause / Resume button + poll-rate dropdown."""

from __future__ import annotations

from dash import Dash, Input, Output, State

from .. import theme

# Frozen-data indicator styles (toggled with the pause state).
_BADGE_HIDDEN = {"display": "none"}
_BADGE_VISIBLE = {
    "position": "fixed", "bottom": "18px", "right": "18px", "zIndex": 200,
    "backgroundColor": "rgba(249, 226, 175, 0.12)", "color": theme.WARN,
    "border": f"1px solid {theme.WARN}", "borderRadius": "8px",
    "padding": "5px 14px", "fontFamily": "monospace", "fontSize": "13px",
    "fontWeight": "bold", "boxShadow": "0 0 16px rgba(249, 226, 175, 0.25)",
    "pointerEvents": "none",
}
# Dim the panels while paused so frozen numbers can't be misread as live.
_CONTENT_LIVE = {"marginTop": "16px", "transition": "opacity 0.2s ease"}
_CONTENT_PAUSED = {"marginTop": "16px", "opacity": "0.55", "transition": "opacity 0.2s ease"}


def register(app: Dash) -> None:
    @app.callback(
        Output("interval", "disabled"),
        Output("pause-btn", "children"),
        Output("paused-badge", "style"),
        Output("tab-content", "style"),
        Input("pause-btn", "n_clicks"),
        State("interval", "disabled"),
        prevent_initial_call=True,
    )
    def toggle_pause(_n_clicks, currently_disabled):
        # Pausing simply disables the dcc.Interval — no extra state to
        # carry around. When the user resumes, the next tick fires
        # immediately and the figures catch up.
        new_disabled = not currently_disabled
        if new_disabled:
            return True, "▶ Resume", _BADGE_VISIBLE, _CONTENT_PAUSED
        return False, "⏸ Pause", _BADGE_HIDDEN, _CONTENT_LIVE

    @app.callback(
        Output("interval", "interval"),
        Input("poll-rate-dropdown", "value"),
        prevent_initial_call=True,
    )
    def set_poll_rate(value_ms):
        # Changing `interval.interval` takes effect on the next tick;
        # Dash handles the timer reset for us.
        return int(value_ms)
