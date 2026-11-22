"""Callbacks for the overlay file dropdown."""

from __future__ import annotations

from pathlib import Path

from dash import Dash, Input, Output, State

from ..backend_client import BackendClient
from ..config import Config
from ..overlay import OverlayStore


def register(app: Dash, store: OverlayStore, config: Config, client: BackendClient) -> None:
    # If overlay is disabled in config.yaml there are no widgets to
    # wire up — skip registration entirely.
    if not config.overlay.enabled:
        return

    @app.callback(
        Output("overlay-file-dropdown", "options"),
        Input("overlay-refresh-btn", "n_clicks"),
    )
    def refresh_files(_n_clicks):
        # Dash invokes this once at startup with n_clicks=None and again whenever
        # the user clicks "Refresh files". Resolve the reference directory: the
        # one the running monitor reports over HTTP wins (zero config, always the
        # real snapshot dir), else the config.yaml `search_dir` fallback.
        backend_dir = client.histo_output_dir()
        store.set_search_dir(Path(backend_dir) if backend_dir else config.overlay.search_dir)
        files = store.available_files()
        # An explicit "(none)" entry to turn the overlay off (clearer than the
        # dropdown's small clear "x"); pick_file maps its empty value to None.
        return [{"label": "(none)", "value": ""}] + [{"label": name, "value": name} for name in files]

    @app.callback(
        Output("client-state", "data"),
        Input("overlay-file-dropdown", "value"),
        State("client-state", "data"),
    )
    def pick_file(file_name, state):
        # The selected overlay file lives in `client-state` (a
        # browser-side dcc.Store) so the poll callback can read it via
        # State without triggering itself when the user opens/closes
        # the dropdown.
        state = dict(state or {})
        # The "(none)" entry (empty value) and the clear "x" both mean no overlay.
        state["overlay_file"] = file_name or None
        store.clear_cache()
        return state
