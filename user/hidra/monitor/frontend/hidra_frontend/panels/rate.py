"""RatePanel — the live event rate, as a big number plus a sparkline.

Reads the backend counter ``events_received`` (the **true**, pre-prescale
cumulative number of events the monitor has seen) and derives the rate
from its change between polls: ``rate = Δcount / Δt`` in Hz. Because the
counter is pre-prescale, this is the real DAQ rate regardless of
``EVENT_PRESCALE`` (a sampled counter like ``event_count`` would read low
by a factor of the prescale).

The big-number card always shows this **raw** instantaneous rate;
``smoothing`` only smooths the sparkline next to it.

Config (in ``config.yaml``)::

    - type: rate
      histogram: events_received   # optional, this is the default
      count_histogram: event_count # optional; adds a total-count card
      count_label: "Events"        # optional title for that card
      smoothing: 0.4               # optional EMA factor for the SPARKLINE
                                   # in [0.02, 1]; 1 = none (number is raw)
      history: 120                 # optional sparkline length (samples)

Graph slots, left to right on one row: the rate number card (slot 0), a
sparkline of the recent rate history (slot 1) that stretches to fill the
width, and — when ``count_histogram`` is set — a total event-count card
(slot 2).

State (previous count/time, EMA, history) lives in per-process instance
attributes — the same single-worker assumption the channel selector
relies on (see README). `figure_names()` returns ``[]`` so the poll
callback never builds a bar figure for the counter; we read the raw
payload here, like the metric panel.
"""

from __future__ import annotations

import logging
import time
from typing import Optional

import plotly.graph_objects as go
from dash import dcc, html

from .. import theme
from .base import Panel
from .metric import _indicator_figure

logger = logging.getLogger(__name__)

DEFAULT_HIST = "events_received"

# Smallest practical EMA factor. The averaging window is ~1/alpha samples,
# so values below this (e.g. 0.001 ≈ 1000 samples ≈ minutes of poll) make
# the sparkline take far too long to track reality and look "frozen". The
# big-number card is always raw, so this floor only affects the sparkline.
MIN_SMOOTHING = 0.02


class RatePanel(Panel):
    def __init__(self, panel_id, params):
        super().__init__(panel_id, params)
        self._hist = params.get("histogram", DEFAULT_HIST)
        # Optional total-count card shown on the same row (e.g. event_count).
        self._count_hist = params.get("count_histogram")
        # Friendly label for that card (the histogram name is ugly as a title).
        self._count_label = params.get("count_label", "Events")
        # EMA factor for the *sparkline*: 1.0 = raw, no smoothing; smaller
        # = smoother but laggier. The big-number card ignores this and
        # always shows the raw instantaneous rate. Valid range is
        # [MIN_SMOOTHING, 1.0]: alpha <= 0 freezes the EMA on the first
        # sample, alpha > 1 overshoots, and very small alpha makes the
        # sparkline unusably slow. Clamp out-of-range values (keeping the
        # user's intended direction) and warn rather than failing.
        alpha = float(params.get("smoothing", 0.4))
        if not MIN_SMOOTHING <= alpha <= 1.0:
            clamped = min(1.0, max(MIN_SMOOTHING, alpha))
            logger.warning(
                "rate panel %s: smoothing=%s outside [%s, 1]; clamping to %s",
                panel_id, alpha, MIN_SMOOTHING, clamped,
            )
            alpha = clamped
        self._alpha = alpha
        self._maxlen = int(params.get("history", 120))
        self._prev_count: Optional[float] = None
        self._prev_time: Optional[float] = None
        self._ema: Optional[float] = None
        self._history: list[float] = []
        # Last raw (un-smoothed) instantaneous rate — what the number card
        # shows; kept so a transient missing counter doesn't blank it.
        self._last_rate: Optional[float] = None

    # ---- Panel API -------------------------------------------------------

    def histogram_names(self) -> list[str]:
        names = [self._hist]
        if self._count_hist:
            names.append(self._count_hist)
        return names

    def figure_names(self) -> list[str]:
        # We read the raw counters, never a pre-built bar figure.
        return []

    def layout(self) -> html.Div:
        height = self.params.get("height", "150px")
        card_style = {"flex": "0 0 200px", "height": height}
        children = [
            dcc.Graph(
                id={"type": "panel-graph", "panel": self.panel_id, "index": 0},
                figure=_rate_number_figure(None),
                style=card_style,
                className="metric-card glow-rate",
                config={"displayModeBar": False},
            ),
            dcc.Graph(
                id={"type": "panel-graph", "panel": self.panel_id, "index": 1},
                figure=_sparkline_figure([]),
                style={"flex": "1", "minWidth": "0", "height": height},
                config={"displayModeBar": False},
            ),
        ]
        if self._count_hist:
            children.append(
                dcc.Graph(
                    id={"type": "panel-graph", "panel": self.panel_id, "index": 2},
                    figure=_indicator_figure(self._count_label, None),
                    style=card_style,
                    className="metric-card glow-count",
                    config={"displayModeBar": False},
                )
            )
        return html.Div(
            style={"display": "flex", "gap": "12px", "marginBottom": "12px", "alignItems": "stretch"},
            children=children,
        )

    def render(self, figs, payloads, client_state):
        count = _extract_count(payloads.get(self._hist))
        rate = self._update(count)
        out = [_rate_number_figure(rate), _sparkline_figure(self._history)]
        if self._count_hist:
            out.append(_indicator_figure(self._count_label, _extract_count(payloads.get(self._count_hist))))
        return out

    # ---- rate computation ------------------------------------------------

    def _update(self, count: Optional[float]) -> Optional[float]:
        """Fold one new counter reading into the rate state.

        Returns the raw (un-smoothed) instantaneous rate in Hz for the
        big-number card, or None if not computable yet. The smoothed EMA
        is accumulated into ``self._history`` and only drives the
        sparkline."""
        if count is None:
            # Counter missing (e.g. backend not yet rebuilt): keep showing
            # the last known value, don't advance the deltas.
            return self._last_rate

        now = time.monotonic()
        rate: Optional[float] = None
        if self._prev_count is not None and self._prev_time is not None:
            dn = count - self._prev_count
            dt = now - self._prev_time
            if dn < 0:
                # Counter went backwards -> run reset. Start fresh.
                self._history = []
                self._ema = 0.0
                rate = 0.0
            elif dt > 0:
                rate = dn / dt

        self._prev_count = count
        self._prev_time = now

        if rate is not None:
            # Raw rate drives the number card; the EMA drives the sparkline.
            self._last_rate = rate
            self._ema = rate if self._ema is None else self._alpha * rate + (1 - self._alpha) * self._ema
            self._history.append(self._ema)
            if len(self._history) > self._maxlen:
                self._history = self._history[-self._maxlen:]
        return self._last_rate


def _extract_count(payload: Optional[dict]) -> Optional[float]:
    """Read the single-bin counter value from the TBufferJSON payload.

    Same direct-buffer trick the metric panel uses: sum the in-range bins
    (here just one). Returns None when the payload is missing/malformed.
    """
    if not payload or "_typename" not in payload:
        return None
    arr = payload.get("fArray") or []
    nbins = payload.get("fXaxis", {}).get("fNbins", 0)
    if not arr or nbins < 1:
        return None
    return float(sum(arr[1:nbins + 1]))


def _rate_number_figure(rate: Optional[float]) -> go.Figure:
    layout = dict(
        margin=dict(l=10, r=10, t=28, b=6),
        paper_bgcolor=theme.BG,
        plot_bgcolor=theme.BG,
        font=dict(color=theme.FG, family=theme.FONT_FAMILY),
        uirevision="rate-number",
    )
    if rate is None:
        layout["annotations"] = [
            dict(text="(no data)", showarrow=False, font=dict(color=theme.WARN, size=12),
                 xref="paper", yref="paper", x=0.5, y=0.1)
        ]
    indicator = go.Indicator(
        mode="number",
        value=(rate if rate is not None else 0),
        number={"font": {"size": 48, "color": theme.OK}, "valueformat": ",.0f", "suffix": " Hz"},
        title={"text": "Event rate", "font": {"size": 15, "color": theme.FG}},
    )
    return go.Figure(data=[indicator], layout=layout)


def _sparkline_figure(history: list[float]) -> go.Figure:
    layout = dict(
        margin=dict(l=6, r=6, t=8, b=6),
        paper_bgcolor=theme.BG,
        plot_bgcolor=theme.BG,
        font=dict(color=theme.FG, family=theme.FONT_FAMILY),
        uirevision="rate-spark",
        showlegend=False,
        xaxis=dict(visible=False),
        yaxis=dict(visible=False, rangemode="tozero"),
    )
    traces: list = []
    if history:
        # Glow effect: a wide, low-opacity spline underneath the crisp line.
        traces.append(
            go.Scatter(
                y=history,
                mode="lines",
                line=dict(color=theme.OK, width=8, shape="spline"),
                opacity=0.18,
                hoverinfo="skip",
                showlegend=False,
            )
        )
        traces.append(
            go.Scatter(
                y=history,
                mode="lines",
                line=dict(color=theme.OK, width=2.5, shape="spline"),
                fill="tozeroy",
                fillcolor="rgba(166, 227, 161, 0.22)",
                name="rate",
            )
        )
    return go.Figure(data=traces, layout=layout)
