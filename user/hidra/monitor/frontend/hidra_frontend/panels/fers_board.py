"""FERSBoardPanel — one 2D heatmap per FERS board, tiled on a grid.

Reads a single per-channel histogram (`FERS_HG_mean_physics` by default)
and lays its values out spatially, **one heatmap per board**: board `b`
shows its `channels_per_board` channels (default 64) in a
`board_cell_rows` x `board_cell_cols` block (default 16 x 4). The board
figures are tiled on a `columns`-wide grid (default 2 per row). All boards
share a common colour scale (global min/max over the channels with data)
so they are directly comparable.

Config (in `config.yaml`):

    - type: fers_board
      histogram: FERS_HG_mean           # base name when mode_toggle is on
      label: "mean HG"
      n_boards: 20                       # number of board heatmaps
      columns: 2                         # board plots per row
      mode_toggle: true                  # physics/pedestal radio (HG/LG means)
      link_tab: fers_channels            # optional: click opens that tab

With `mode_toggle: true` the panel shows a physics/pedestal radio and
appends the active suffix to `histogram` (`FERS_HG_mean` -> `_physics` /
`_pedestal`), so only the *shown* variant is fetched from the backend.
Without it, `histogram` is used verbatim (e.g. `FERS_HG_saturation_physics`).

The number of boards is fixed at layout time (one graph slot per board),
so it comes from `n_boards` (default 20, matching the backend
`FERS_NBOARDS`).

The source histogram is a `TProfile` filled with `Fill(channel, value)`
(per-channel mean) or a plain per-channel `TH1` (bin content, e.g. a
saturation fraction). We read the buffers directly here.
"""

from __future__ import annotations

from typing import Optional

import plotly.graph_objects as go
from dash import Dash, Input, Output, dcc, html

from .. import theme
from .base import Panel

COLORSCALE = "Viridis"
_MODES = ("physics", "pedestal")


class FERSBoardPanel(Panel):
    def __init__(self, panel_id, params):
        super().__init__(panel_id, params)
        # Injected from the `fers:` config section by build_panels; the default
        # is the standard FERS board size (= board_cell_rows × board_cell_cols).
        self._channels_per_board = int(params.get("channels_per_board", 64))
        self._cell_rows = int(params.get("board_cell_rows", 16))
        self._cell_cols = int(params.get("board_cell_cols", 4))
        if self._cell_rows * self._cell_cols != self._channels_per_board:
            raise ValueError(
                f"fers_board: board_cell_rows*board_cell_cols "
                f"({self._cell_rows}*{self._cell_cols}) must equal "
                f"channels_per_board ({self._channels_per_board})"
            )
        self._n_boards = int(params.get("n_boards", 20))
        self._columns = max(1, int(params.get("columns", 2)))
        # Optional physics/pedestal toggle (HG/LG mean maps). When on, the
        # active suffix is appended to the configured base histogram name and
        # only that variant is fetched.
        self._mode_toggle = bool(params.get("mode_toggle", False))
        self._mode = "physics"
        # Per-board cell placement (lr, lc, local_channel), identical for every
        # board — only the channel offset b*channels_per_board differs.
        self._cells = self._build_cells()

    def _build_cells(self) -> list[tuple[int, int, int]]:
        return [(*divmod(local, self._cell_cols), local) for local in range(self._channels_per_board)]

    # ---- config helpers --------------------------------------------------

    def _base_hist(self) -> str:
        default = "FERS_HG_mean" if self._mode_toggle else "FERS_HG_mean_physics"
        return self.params.get("histogram", default)

    def _hist_name(self) -> str:
        if self._mode_toggle:
            return f"{self._base_hist()}_{self._mode}"
        return self._base_hist()

    def _value_label(self) -> str:
        return self.params.get("label", "mean")

    def gain_tag(self) -> Optional[str]:
        """`"HG"`/`"LG"` parsed from the histogram name, used to pair this map
        with the matching channel selector in `link_tab` (None if neither)."""
        name = self._base_hist()
        if "_HG" in name:
            return "HG"
        if "_LG" in name:
            return "LG"
        return None

    def link_tab(self) -> Optional[str]:
        """Tab id to open when a cell is clicked (None = not clickable)."""
        return self.params.get("link_tab")

    def _title_suffix(self) -> str:
        if self._mode_toggle:
            return f" ({self._mode})"
        name = self._hist_name()
        if name.endswith("_physics"):
            return " (physics)"
        if name.endswith("_pedestal"):
            return " (pedestal)"
        return ""

    def _board_title(self, board: int) -> str:
        gain = self.gain_tag()
        gain = f" {gain}" if gain else ""
        tag = self.params.get("title_tag")
        tag = f" — {tag}" if tag else ""
        return f"FERS{gain} · board {board}{tag}{self._title_suffix()}"

    # ---- Panel API -------------------------------------------------------

    def histogram_names(self) -> list[str]:
        return [self._hist_name()]

    def figure_names(self) -> list[str]:
        # Reads the raw buffers and lays the values out spatially, so it never
        # uses the pre-built bar figure.
        return []

    def layout(self) -> html.Div:
        height = self.params.get("height", "180px")
        graph_class = "detector-clickable" if self.link_tab() else ""
        # One graph slot per board, tiled on a `columns`-wide grid.
        grid = html.Div(
            style={
                "display": "grid",
                "gridTemplateColumns": f"repeat({self._columns}, 1fr)",
                "gap": "8px",
            },
            children=[
                dcc.Graph(
                    id={"type": "panel-graph", "panel": self.panel_id, "index": b},
                    figure=theme.placeholder_figure(self._board_title(b)),
                    style={"height": height},
                    className=graph_class,
                    config={"displayModeBar": False},
                )
                for b in range(self._n_boards)
            ],
        )
        children: list = []
        if self._mode_toggle:
            children.append(self._mode_controls())
        children.append(grid)
        return html.Div(style={"marginBottom": "12px"}, children=children)

    def _mode_controls(self) -> html.Div:
        return html.Div(
            style={"display": "flex", "alignItems": "center", "gap": "8px", "marginBottom": "8px"},
            children=[
                html.Span("Mode:", style={"color": theme.FG, "fontSize": "13px"}),
                dcc.RadioItems(
                    id={"type": "fers-board-mode", "panel": self.panel_id},
                    options=[{"label": m, "value": m} for m in _MODES],
                    value=self._mode,
                    inline=True,
                    labelStyle={"marginRight": "12px", "color": theme.FG},
                ),
                # Sink for the toggle callback (the real state lives in self._mode).
                dcc.Store(id={"type": "fers-board-mode-sink", "panel": self.panel_id}),
            ],
        )

    def render(self, figs, payloads, client_state):
        payload = payloads.get(self._hist_name())
        values, _ = _channel_values(payload)
        # Common colour scale across all boards so they are comparable.
        present = list(values.values()) if values else []
        cmin, cmax = (min(present), max(present)) if present else (0.0, 1.0)
        if cmin == cmax:
            cmax = cmin + 1.0
        return [self._board_figure(b, values, cmin, cmax) for b in range(self._n_boards)]

    def register_callbacks(self, app: Dash) -> None:
        if not self._mode_toggle:
            return

        @app.callback(
            Output({"type": "fers-board-mode-sink", "panel": self.panel_id}, "data"),
            Input({"type": "fers-board-mode", "panel": self.panel_id}, "value"),
            prevent_initial_call=True,
        )
        def _on_mode(value):
            # Persist the choice; the next poll fetches only the active variant
            # via histogram_names(). Returning value satisfies Dash's Output rule.
            if value in _MODES:
                self._mode = value
            return value

    # ---- figure ----------------------------------------------------------

    def _board_figure(
        self, board: int, values: Optional[dict[int, float]], cmin: float, cmax: float
    ) -> go.Figure:
        title = self._board_title(board)
        layout = theme.base_figure_layout(title)

        if values is None:
            layout["title"] = f"{title} (missing)"
            layout["annotations"] = [
                dict(text="missing on server", showarrow=False, font=dict(color=theme.WARN, size=14))
            ]
            return go.Figure(layout=layout)

        offset = board * self._channels_per_board
        z: list[list[Optional[float]]] = [[None] * self._cell_cols for _ in range(self._cell_rows)]
        text: list[list[str]] = [[""] * self._cell_cols for _ in range(self._cell_rows)]
        customdata: list[list[Optional[int]]] = [[None] * self._cell_cols for _ in range(self._cell_rows)]
        for lr, lc, local in self._cells:
            channel = offset + local
            z[lr][lc] = values.get(channel)
            text[lr][lc] = f"board {board} · ch {local}"
            customdata[lr][lc] = channel

        heatmap = go.Heatmap(
            z=z,
            text=text,
            customdata=customdata,
            colorscale=COLORSCALE,
            zmin=cmin, zmax=cmax,
            xgap=1, ygap=1,
            hoverongaps=False,
            hovertemplate="%{text}<br>" + self._value_label() + " %{z:.3f}<extra></extra>",
            colorbar=dict(title=self._value_label(), thickness=12),
        )
        layout["xaxis"] = {
            **layout["xaxis"],
            "showticklabels": False, "showgrid": False, "zeroline": False,
        }
        layout["yaxis"] = {
            **layout["yaxis"],
            "showticklabels": False, "showgrid": False, "zeroline": False,
            "autorange": "reversed",
        }
        return go.Figure(data=[heatmap], layout=layout)


def _channel_values(payload: Optional[dict]) -> tuple[Optional[dict[int, float]], Optional[int]]:
    """channel index -> per-channel value, read straight from the buffers.

    For a `TProfile` this is the bin mean (`fArray/fBinEntries`); for a plain
    per-channel `TH1` it is the bin content. Returns ``(values, nbins)``;
    ``values`` is None when the payload is missing/unusable. A `TProfile`
    channel with no entries is absent from the dict (its mean is undefined);
    a `TH1` keeps every in-range bin, including a genuine ``0.0``.
    """
    if not payload or "_typename" not in payload:
        return None, None

    nbins = payload.get("fXaxis", {}).get("fNbins", 0)
    sumw = payload.get("fArray") or []
    if nbins < 1 or not sumw:
        return None, nbins or None

    # fArray / fBinEntries layout: [underflow, bin_1, ..., bin_N, overflow].
    # Channel c was filled at x = c, which lands in bin c + 1.
    entries = payload.get("fBinEntries") or []

    values: dict[int, float] = {}
    for channel in range(nbins):
        idx = channel + 1
        if idx >= len(sumw):
            break
        if entries:
            # TProfile: mean = sum(weight*y) / sum(weight).
            if idx < len(entries) and entries[idx] > 0:
                values[channel] = sumw[idx] / entries[idx]
        else:
            # Plain TH1: bin content as-is. Keep 0.0 (a genuine zero).
            values[channel] = float(sumw[idx])

    return values, nbins
