"""Top-level Dash layout.

This module builds the initial layout shown when the page loads:
header (title, pause button, poll-rate dropdown, overlay controls),
status bar, tab strip, and the empty container `tab-content` where the
per-tab panels are injected by the `update_tab_content` callback.

It also instantiates one `Panel` object per panel entry in
`config.yaml`. The Panel objects live in the `panels_by_tab` dict for
the entire app lifetime — they hold no per-poll state, only the
layout/parameters they were configured with.
"""

from __future__ import annotations

from dash import dcc, html

from . import theme
from .config import Config
from .panels import build_panel
from .panels.base import Panel


def build_panels(
    config: Config,
    available_histograms: list[str] | None = None,
    client=None,
) -> dict[str, list[Panel]]:
    """Instantiate one Panel object per panel entry. Indexed by tab id.

    `available_histograms` is the list of names the backend currently
    exposes (from `BackendClient.list_histograms()`). Panels that
    auto-discover their histograms — e.g. `channel_selector` in name mode —
    receive it via the `available_histograms` param. `client` (the
    `BackendClient`) lets projection-mode channel selectors learn their
    channel count once from the per-channel TH2's x axis (GetNbinsX).
    """
    available = list(available_histograms or [])
    by_tab: dict[str, list[Panel]] = {}
    for tab in config.tabs:
        panels: list[Panel] = []
        # `panel_id` is what the panel uses to scope its component IDs.
        # The `<tab>__p<i>` pattern makes IDs unique across tabs.
        for i, panel_cfg in enumerate(tab.panels):
            panel_id = f"{tab.id}__p{i}"
            params = panel_cfg.params
            if panel_cfg.type == "channel_selector" and not params.get("names"):
                params = {**params, "available_histograms": available}
            # FERS board size is a detector property (the `fers:` config
            # section), not a per-panel value: inject it where panels need it
            # instead of repeating the number in each panel's config.
            needs_board_size = panel_cfg.type == "fers_board" or (
                panel_cfg.type == "channel_selector" and params.get("board_labels")
            )
            if needs_board_size and "channels_per_board" not in params:
                params = {**params, "channels_per_board": config.fers.channels_per_board}
            # Projection-mode channel selector (FERS, ADC): no per-channel names
            # to discover. Give it the client so it can learn the channel count
            # from the per-channel TH2's x axis (GetNbinsX) lazily — resolved on
            # first use and retried while unknown, so it survives the backend not
            # being up yet at frontend startup.
            if panel_cfg.type == "channel_selector" and params.get("projection_templates"):
                params = {**params, "_client": client}
            panels.append(build_panel(panel_id, panel_cfg.type, params))
        by_tab[tab.id] = panels
    return by_tab


def build(config: Config, panels_by_tab: dict[str, list[Panel]]) -> html.Div:
    first_tab = config.tabs[0].id
    pump_hint = config.polling.server_pump_ms_hint

    # Only offer poll-rate choices >= the configured floor.
    poll_choices = [
        {"label": f"{ms} ms", "value": ms}
        for ms in config.polling.choices_ms
        if ms >= config.polling.floor_ms
    ]

    # Overlay controls are only added when the feature is enabled in
    # config.yaml. If disabled, the corresponding callback in
    # callbacks/overlay.py is also skipped.
    overlay_controls: list = []
    if config.overlay.enabled:
        overlay_controls = [
            html.Span("Overlay:", style={"color": theme.FG, "fontSize": "13px"}),
            dcc.Dropdown(
                id="overlay-file-dropdown",
                options=[],
                value=config.overlay.default_file,
                placeholder="(none)",
                clearable=True,
                style={"width": "260px"},
            ),
            html.Button(
                "Refresh files",
                id="overlay-refresh-btn",
                n_clicks=0,
                className="btn",
                style=_button_style(),
            ),
        ]

    return html.Div(
        style={
            "backgroundColor": theme.BG,
            "minHeight": "100vh",
            "fontFamily": "monospace",
            "color": theme.FG,
            "padding": "16px",
        },
        children=[
            # ── Sticky header: stays pinned so the controls and tabs are
            #    always reachable while the panels below scroll. ─────────
            html.Div(
                className="app-header",
                style={
                    "position": "sticky",
                    "top": "0",
                    "zIndex": 50,
                    "backgroundColor": theme.BG,
                    "paddingTop": "4px",
                    "paddingBottom": "8px",
                    "marginBottom": "4px",
                    "borderBottom": f"1px solid {theme.BORDER}",
                },
                children=[
                    # Top bar: title, pause, poll rate, overlay
                    html.Div(
                        style={"display": "flex", "alignItems": "center", "gap": "16px", "marginBottom": "8px"},
                        children=[
                            html.H2(
                                "HiDRa Monitor",
                                id="app-title",
                                style={
                                    "color": theme.ACCENT,
                                    "margin": 0,
                                    "letterSpacing": "1px",
                                    # Subtle accent glow on the title (HTML text
                                    # takes text-shadow, unlike the SVG numbers).
                                    "textShadow": "0 0 18px rgba(203, 166, 247, 0.45)",
                                },
                            ),
                            html.Button("⏸ Pause", id="pause-btn", n_clicks=0, className="btn", style=_button_style()),
                            html.Span("Poll:", style={"color": theme.FG, "fontSize": "13px"}),
                            dcc.Dropdown(
                                id="poll-rate-dropdown",
                                options=poll_choices,
                                value=config.polling.default_ms,
                                clearable=False,
                                style={"width": "120px"},
                            ),
                            *overlay_controls,
                            # Total event count, pinned to the right of the top
                            # bar so it's visible from any tab (updated every poll
                            # by callbacks/poll.py, regardless of the active tab).
                            html.Div(
                                id="header-event-count",
                                className="header-count",
                                children="—",
                                style={
                                    "marginLeft": "auto",
                                    "color": theme.PRIMARY,
                                    "fontSize": "20px",
                                    "fontWeight": "bold",
                                    "whiteSpace": "nowrap",
                                },
                            ),
                        ],
                    ),

                    # Status bar (updated on every poll). Plain text with a
                    # coloured state dot prepended by the poll callback.
                    html.Div(
                        id="status-bar",
                        style={"color": theme.FG, "fontSize": "12px"},
                        children=f"server pump ~{pump_hint} ms",
                    ),

                    # Tabs
                    dcc.Tabs(
                        id="tabs",
                        value=first_tab,
                        colors={"border": theme.SURFACE, "primary": theme.ACCENT, "background": theme.BG_ALT},
                        style={"marginTop": "10px"},
                        children=[
                            dcc.Tab(
                                label=tab.label,
                                value=tab.id,
                                style={"color": theme.FG, "backgroundColor": theme.BG_ALT, "border": "none"},
                                selected_style={
                                    "color": theme.ACCENT,
                                    "backgroundColor": theme.BG,
                                    "fontWeight": "bold",
                                    # Accent indicator on the active tab.
                                    "borderTop": f"2px solid {theme.ACCENT}",
                                    "boxShadow": "0 0 14px rgba(203, 166, 247, 0.18)",
                                },
                            )
                            for tab in config.tabs
                        ],
                    ),
                ],
            ),

            # Frozen-data indicator: shown by the pause callback so a paused
            # dashboard can't be mistaken for live data.
            html.Div(id="paused-badge", children="⏸ PAUSED — data frozen", style={"display": "none"}),

            # The active tab's panels are injected here by
            # `update_tab_content` in callbacks/poll.py.
            html.Div(id="tab-content", style={"marginTop": "16px"}),

            # Ticks every `interval` ms (changed at runtime by the
            # poll-rate dropdown). `disabled=True` pauses the polling.
            dcc.Interval(id="interval", interval=config.polling.default_ms, n_intervals=0, disabled=False),

            # Browser-side state. `dcc.Store` lets callbacks read/write
            # small dicts without round-tripping through the server
            # except when a callback is triggered.
            #   * client-state holds the selected overlay file (and
            #     any future bits the panels want to share).
            #   * tab-mount-state is bumped by update_tab_content so
            #     poll() runs only AFTER the DOM has been rebuilt for
            #     the new tab — preventing a race where the previous
            #     tab's figures land in the new tab's slots.
            dcc.Store(id="client-state", data={"overlay_file": config.overlay.default_file}),
            dcc.Store(id="tab-mount-state", data={"tab": first_tab, "rev": 0}),
            # Per-graph "reset zoom" counters: {"<panel>|<index>": n}. The
            # poll callback folds these into each figure's uirevision so a
            # click clears the preserved zoom on the next tick.
            dcc.Store(id="graph-reset", data={}),

            # Static config read by assets/ui_effects.js (a plain asset, not
            # a Dash callback): the local hour for the daily "shower"
            # animation, or empty to disable the automatic trigger.
            html.Div(
                id="ui-effects-config",
                style={"display": "none"},
                **{"data-shower-hour": "" if config.ui.shower_hour is None else str(config.ui.shower_hour)},
            ),
        ],
    )


def _button_style() -> dict:
    return {
        "backgroundColor": theme.SURFACE,
        "color": theme.FG,
        "border": f"1px solid {theme.BORDER}",
        "padding": "6px 14px",
        "fontFamily": "monospace",
        "fontSize": "13px",
        "cursor": "pointer",
        "borderRadius": "4px",
    }
