"""Build a Plotly figure from a decoded histogram.

This module is the bridge between two worlds:

  * the **decoders** (in `decoders/`) — they take the raw JSON payload
    from the backend and produce a `DecodedHist` (counts, edges, errors).
  * **Plotly** — to render those numbers in the browser we wrap them in
    a `plotly.graph_objects.Figure`.

`to_figure()` is what the poll callback calls once per histogram.
"""

from __future__ import annotations

import logging
import re
from typing import Optional

import numpy as np
import plotly.graph_objects as go

from . import theme
from .config import HistogramDisplayCfg
from .decoders import Decoder, DecodedHist, DecoderError, rebin_decoded
from .perf import Phase

logger = logging.getLogger(__name__)

# Minimal ROOT TLatex -> Unicode map for axis titles. ROOT stores axis
# titles like "#Delta t [#mus]"; Plotly doesn't speak TLatex, so we
# translate the handful of tokens the HiDRA histograms actually use.
# Unknown "#tokens" are left as-is rather than guessed at.
# An explicit ordered list (not a dict) makes the substitution order part
# of the contract; here no token is a substring of another, so order does
# not actually affect the result.
_ROOT_TLATEX = [
    ("#Delta", "Δ"),
    ("#delta", "δ"),
    ("#sigma", "σ"),
    ("#Sigma", "Σ"),
    ("#sum", "∑"),  # n-ary summation, distinct from #Sigma
    ("#pi", "π"),
    ("#times", "×"),
    ("#sqrt", "√"),
    ("#mu", "µ"),  # also turns "#mus" into "µs"
]


def _root_latex_to_unicode(s: str) -> str:
    for token, glyph in _ROOT_TLATEX:
        s = s.replace(token, glyph)
    return s


def _axis_unit(axis_title: str) -> Optional[str]:
    """Pull the unit out of an axis title like "Δt [µs]" -> "µs"."""
    m = re.search(r"\[([^\]]+)\]", axis_title)
    return m.group(1).strip() if m else None


def to_figure(
    decoder: Decoder,
    obj_dict: Optional[dict],
    name: str,
    overlay_hist: Optional[DecodedHist] = None,
    options: Optional[HistogramDisplayCfg] = None,
    normalize: str = "none",
) -> go.Figure:
    """Render one histogram as a Plotly figure.

    * `obj_dict` is the raw payload the backend returned (or `None` if
      the name is missing on the server).
    * `overlay_hist` is the optional reference histogram from a local
      `.root` file (loaded by `OverlayStore`).
    * `normalize` controls live-vs-reference scaling when an overlay is
      shown: "area" divides the live and the reference each by its own
      sum of counts (both integrate to 1, shapes compare regardless of
      statistics); "none" keeps raw counts. No effect without an overlay.
    * `options` are the per-histogram display options from config.yaml
      (`logx`, `density`); `None` means defaults (linear, raw counts).
    * Errors during decoding/rendering produce a figure with an
      explanatory annotation instead of crashing the callback.
    """

    logx = bool(options.logx) if options else False
    density = bool(options.density) if options else False
    show_flow = bool(options.show_flow) if options else False

    # Decode early so we can use the histogram's own title, if available.
    # A failed decode must NOT silently fall through to an empty plot
    # (which looks like valid-but-blank data): capture the error here and
    # render it as an annotation below. DecoderError = a payload shape we
    # don't support yet (expected, warn); anything else is unexpected and
    # gets a full stack trace in the log.
    decoded = None
    decode_error: Optional[tuple[str, str]] = None  # (message, colour)
    if obj_dict is not None and "_typename" in obj_dict:
        try:
            with Phase("decode.live"):
                decoded = decoder.decode(obj_dict)
        except DecoderError as exc:
            logger.warning("decoding %s unsupported: %s", name, exc)
            decode_error = (f"unsupported: {exc}", theme.WARN)
        except Exception as exc:
            logger.exception("decoding %s failed", name)
            decode_error = (f"decode error: {exc}", theme.ERR)

    # Pick the title: the decoded histogram's own title when valid, else the name.
    plot_title = decoded.title if decoded and hasattr(decoded, "title") and decoded.title else name

    # For 1D histograms (TH1*, not TProfile or 2D) append the total entry count
    # — ROOT's fEntries, which counts every Fill including over/underflow — to
    # the title. TProfile/TH2 are excluded: their "entries" mean something else
    # (per-bin samples / 2D), so we keep their title unchanged.
    if decoded is not None and decoded.typename.startswith("TH1"):
        entries = (obj_dict or {}).get("fEntries")
        if entries is not None:
            plot_title = f"{plot_title}  (entries: {int(entries):,})"

    # Accumulate traces + layout tweaks as plain data, then build the
    # go.Figure in a single shot at the end. Constructing once with
    # data=/layout= is ~4x cheaper than go.Figure()+update_layout()+
    # add_trace()+update_layout(), which dominates the poll (validation).
    layout = theme.base_figure_layout(plot_title)
    traces: list = []
    annotations: list = []

    # No payload for this name: show a "missing on server" placeholder
    # instead of an empty plot, so the user notices that something is off.
    if obj_dict is None or "_typename" not in obj_dict:
        layout["title"] = f"{plot_title} (missing)"
        annotations.append(dict(text="missing on server", showarrow=False, font=dict(color=theme.WARN, size=14)))
        layout["annotations"] = annotations
        return go.Figure(layout=layout)

    # Decoding failed (above): show the captured message instead of an
    # empty plot, so a broken payload is visibly broken rather than blank.
    if decode_error is not None:
        text, colour = decode_error
        annotations.append(dict(text=text, showarrow=False, font=dict(color=colour, size=12)))
        layout["annotations"] = annotations
        return go.Figure(layout=layout)

    # A log x-axis only makes sense when every bin edge is positive; if
    # not (e.g. a histogram that starts at 0), fall back to linear so the
    # plot doesn't silently drop bins.
    apply_logx = logx and decoded is not None and decoded.edges.size > 0 and bool(np.all(decoded.edges > 0))

    # Axis titles (with units) come straight from the ROOT histogram, which
    # stores them as "Title;x-title;y-title" — already split into
    # fXaxis.fTitle / fYaxis.fTitle by ROOT at construction. Histograms with
    # no axis title (most ADC/TDC ones) leave these empty and are unchanged.
    # obj_dict is non-None here (guarded by the early returns above); normalize
    # defensively so the .get() chain is safe regardless.
    obj = obj_dict or {}
    x_title = _root_latex_to_unicode((obj.get("fXaxis") or {}).get("fTitle", "") or "")
    y_title = _root_latex_to_unicode((obj.get("fYaxis") or {}).get("fTitle", "") or "")

    # A per-channel histogram has one bin per channel, so the hover should
    # read "ch N" rather than the raw bin centre. This is driven explicitly
    # by the x-axis title being "channel" (set by the backend on the
    # channel-indexed TProfiles and on ADC_noise_pedestal) — we do NOT assume
    # every TProfile is channel-indexed, so a profile vs. time / other
    # quantity keeps its real x in the hover.
    per_channel = decoded is not None and x_title.strip().lower() == "channel"
    # For a per-channel histogram, optionally also surface the board number and
    # the channel within the board in the hover (e.g. FERS: 64 channels/board).
    channels_per_board = int(options.channels_per_board) if options else 0

    # Reference-overlay normalization. When a reference trace is shown and area
    # normalization is on, scale the live and the reference each by 1/Σcounts so
    # both integrate to 1 and their shapes compare regardless of statistics. With
    # no overlay the live histogram keeps its natural counts axis (scale = 1).
    # Area-normalize genuine distributions only. A TProfile's y is already a
    # mean and a per-channel plot's y is a value per channel — dividing either by
    # a sum of counts is meaningless, so those overlay in their raw units.
    normalized = (
        overlay_hist is not None
        and normalize == "area"
        and decoded is not None
        and decoded.typename.startswith("TH1")
        and not per_channel
    )
    live_scale = ref_scale = 1.0
    if normalized:
        if decoded is not None:
            live_sum = float(np.sum(decoded.counts))
            live_scale = 1.0 / live_sum if live_sum > 0 else 1.0
        ref_sum = float(np.sum(overlay_hist.counts))
        ref_scale = 1.0 / ref_sum if ref_sum > 0 else 1.0

    # Build the live trace from the (successfully) decoded histogram.
    if decoded is not None:
        try:
            with Phase(f"trace_build.{decoded.typename[:3]}"):
                trace, extra_layout = _build_trace(
                    decoded, color=theme.PRIMARY, density=density, logx=apply_logx,
                    per_channel=per_channel, channels_per_board=channels_per_board,
                    scale=live_scale,
                )
            if trace is not None:
                traces.append(trace)
                layout.update(extra_layout)
                # Optional underflow/overflow bars at the edges (e.g. the
                # "no trigger mask" events). Skipped on a log x-axis, where a
                # bar at/below the lower edge can't be placed.
                if show_flow and not apply_logx:
                    flow = _flow_trace(decoded, scale=live_scale)
                    if flow is not None:
                        traces.append(flow)
                        # Distinct x positions, but keep them from being
                        # grouped/narrowed against the main bars.
                        layout["barmode"] = "overlay"
            else:
                annotations.append(dict(text=f"Unknown type: {decoded.typename}", showarrow=False, font=dict(size=14)))
        except Exception as exc:
            # The payload decoded but we couldn't turn it into a trace
            # (e.g. malformed edges): render it rather than going blank.
            logger.exception("building trace for %s failed", name)
            annotations.append(dict(text=f"render error: {exc}", showarrow=False, font=dict(color=theme.ERR, size=12)))
            layout["annotations"] = annotations
            return go.Figure(layout=layout)

    # Optional reference overlay (already decoded — comes from
    # OverlayStore which uses uproot). Match the live trace's density/logx
    # so the two are directly comparable.
    if overlay_hist is not None:
        with Phase("trace_build.overlay"):
            otrace, _ = _build_trace(
                overlay_hist, color=theme.REFERENCE, dashed=True, label_suffix=" (ref)",
                density=density, logx=apply_logx, scale=ref_scale,
            )
        if otrace is not None:
            traces.append(otrace)

    # A log x-axis is a layout property; the trace already places its bars
    # at geometric centres with log-width to match.
    if apply_logx:
        layout["xaxis"]["type"] = "log"

    # Apply the axis titles computed above (kept here so density can rewrite
    # the y label using the x unit).
    if x_title:
        layout["xaxis"]["title"] = x_title

    if density:
        # Density rescales the y content (per unit x). Label it with the real
        # x unit when the x title carries one (e.g. "events / µs"), else fall
        # back to a generic label so the bars aren't read as raw counts.
        base_y = y_title or "entries"
        unit = _axis_unit(x_title)
        layout["yaxis"]["title"] = f"{base_y} / {unit}" if unit else f"{base_y} / bin width"
    elif y_title:
        layout["yaxis"]["title"] = y_title

    # Area normalization changes what the y values mean (a fraction of the
    # entries, not raw counts), so relabel the axis to say so. With density the
    # integral (sum of fraction*width) is still 1, but the height is per unit x.
    if normalized:
        if density:
            unit = _axis_unit(x_title)
            layout["yaxis"]["title"] = f"fraction / {unit}" if unit else "fraction / bin width"
        else:
            layout["yaxis"]["title"] = "fraction of entries"

    if annotations:
        layout["annotations"] = annotations
    return go.Figure(data=traces, layout=layout)


def overlay_figure(
    decoder: Decoder,
    specs: list[tuple[Optional[dict], str, str]],
    title: str,
    per_channel: bool = False,
    rebin: int = 1,
    show_flow: bool = False,
) -> go.Figure:
    """Render several histograms superimposed on a single figure.

    `specs` is a list of `(payload, color, label)`: each payload is the
    raw TBufferJSON dict the backend returned for that histogram (or
    `None` if it was missing on the server). Every successfully decoded
    histogram becomes one `go.Scatter` trace; the legend is shown so the
    user can click an entry to hide/show it (all visible by default).

    `rebin` (> 1) merges groups of that many bins before building the
    traces (frontend rebin control). Applied to the decoded 1D histograms
    only — skipped for the per-channel comparison whose x is a channel
    index, and a no-op on TProfile (see `rebin_decoded`). The traces are
    rebuilt from the coarser edges/counts, so widths and the y autorange
    adapt while the x range stays put.

    * Default (distributions): an **edge-aligned step line** — the step
      boundaries sit on the ROOT bin edges, so several overlaid spectra
      stay readable where overlaid bars would not. Used by the channel
      selector's total/physics/pedestal overlay.
    * `per_channel=True` (per-channel comparison, x = channel): a
      `lines+markers` trace, one marker per channel, with a "ch N" hover
      that names the series — e.g. the IQR vs std pedestal-noise overlay.

    `show_flow` (distributions only) appends ROOT's overflow and prepends
    the underflow as one extra bin just outside the range, in each series'
    own colour, so out-of-range entries stay visible.

    Decode/render failures for one spec are logged and skipped rather
    than failing the whole figure.
    """
    # Note the active rebin factor in the title so it's clear the bins were merged.
    if rebin > 1:
        title = f"{title} · rebin ×{rebin}"
    layout = theme.base_figure_layout(title)
    layout["showlegend"] = True
    layout["legend"] = dict(bgcolor="rgba(0,0,0,0)", font=dict(size=11))

    traces: list = []
    x_title = ""
    y_title = ""
    for rank, (payload, color, label) in enumerate(specs):
        if not payload or "_typename" not in payload:
            continue
        try:
            with Phase("decode.overlay"):
                decoded = decoder.decode(payload)
        except DecoderError as exc:
            logger.warning("overlay decode of %s unsupported: %s", label, exc)
            continue
        except Exception:
            logger.exception("overlay decode of %s failed", label)
            continue

        # Coarsen the binning if requested (no-op for factor 1 / per-channel / TProfile).
        if rebin > 1 and not per_channel:
            decoded = rebin_decoded(decoded, rebin)

        edges = decoded.edges
        if edges.size < 2:
            continue
        y = decoded.counts.astype(float)
        # For distribution overlays (e.g. the channel selector's total/physics/
        # pedestal) show each series' total entry count — ROOT's fEntries, incl.
        # over/underflow — in the legend. Skipped for the per-channel comparison,
        # where "entries" would just be the number of channels.
        legend_label = label
        if not per_channel:
            entries = payload.get("fEntries")
            if entries is not None:
                legend_label = f"{label}  ({int(entries):,})"
        trace_kwargs: dict = dict(
            line=dict(color=color),
            name=legend_label,
            # Keep the legend in spec order regardless of draw order below.
            legendrank=rank,
        )
        if per_channel:
            # x is a channel index: one marker per channel, "ch N" hover that
            # names the series (each trace carries its own label literally).
            centers = 0.5 * (edges[:-1] + edges[1:])
            trace_kwargs.update(
                x=centers, y=y, mode="lines+markers",
                customdata=np.arange(len(centers)),
                hovertemplate=f"{label}<br>ch %{{customdata}}<br>%{{y:.4g}}<extra></extra>",
            )
            trace_kwargs["line"]["shape"] = "linear"
        else:
            # Edge-aligned step: hold each bin's height across its full width
            # [edge_i, edge_{i+1}]. With x = edges and y repeating its last
            # value, line_shape "hv" makes the step transitions fall on the
            # bin edges (using centers would shift them by half a bin).
            step_edges = edges
            step_counts = y
            if show_flow:
                # Append ROOT's overflow / prepend underflow as one extra bin
                # (same width as the adjacent one) just outside the range, so the
                # out-of-range entries stay visible in each series' own colour.
                if decoded.overflow:
                    wn = edges[-1] - edges[-2]
                    step_edges = np.append(step_edges, edges[-1] + wn)
                    step_counts = np.append(step_counts, decoded.overflow)
                if decoded.underflow:
                    w0 = edges[1] - edges[0]
                    step_edges = np.insert(step_edges, 0, edges[0] - w0)
                    step_counts = np.insert(step_counts, 0, decoded.underflow)
            trace_kwargs.update(
                x=step_edges, y=np.append(step_counts, step_counts[-1]), mode="lines",
            )
            trace_kwargs["line"]["shape"] = "hv"
        traces.append(go.Scatter(**trace_kwargs))

        # Keep the first non-empty axis titles we come across (all series
        # share the same binning/units).
        if not x_title:
            x_title = _root_latex_to_unicode((payload.get("fXaxis") or {}).get("fTitle", "") or "")
        if not y_title:
            y_title = _root_latex_to_unicode((payload.get("fYaxis") or {}).get("fTitle", "") or "")

    if not traces:
        layout["annotations"] = [dict(text="missing on server", showarrow=False, font=dict(color=theme.WARN, size=14))]
        return go.Figure(layout=layout)

    if x_title:
        layout["xaxis"]["title"] = x_title
    if y_title:
        layout["yaxis"]["title"] = y_title
    # Draw order is data order (later = on top). Reverse so the first spec
    # (the primary series, e.g. IQR) is drawn last and stays visible on top
    # of the others; legendrank keeps the legend in the original spec order.
    return go.Figure(data=list(reversed(traces)), layout=layout)


def _build_trace(
    decoded: DecodedHist,
    color: str,
    dashed: bool = False,
    label_suffix: str = "",
    density: bool = False,
    logx: bool = False,
    per_channel: bool = False,
    channels_per_board: int = 0,
    scale: float = 1.0,
) -> tuple[Optional[go.BaseTraceType], dict]:
    """Build one trace for a decoded histogram.

    Returns ``(trace, extra_layout)``. ``trace`` is None for unknown
    types (TH2 / TProfile2D / …), which the caller turns into an
    annotation. ``extra_layout`` carries any layout tweak the trace
    needs (e.g. ``bargap=0`` for the bar chart).

    `density` divides each bin content by its (linear) bin width.
    `logx` lays the bars out for a logarithmic x-axis: bars are centred
    at the bin's geometric mean and their `width` is given in log10 (the
    axis' own units when `xaxis.type == "log"`), so a non-uniform binning
    renders correctly instead of being squashed against the left edge.
    """
    label = decoded.name + label_suffix

    # Bin centres and widths come from the bin edges. Same shape for
    # TH1 and TProfile — the difference between the two is only in
    # what `decoded.counts` means (raw count vs. mean), which we don't
    # need to know here.
    if decoded.typename == "TProfile" or decoded.typename.startswith("TH1"):
        edges = decoded.edges
        lin_widths = edges[1:] - edges[:-1]

        # `scale` (a constant) applies the area normalization; it commutes with
        # the per-width density division below.
        y = decoded.counts.astype(float) * scale
        if density:
            # Per unit x. Guard against zero-width bins (shouldn't happen
            # for valid edges, but never divide by zero).
            with np.errstate(divide="ignore", invalid="ignore"):
                y = np.where(lin_widths > 0, y / lin_widths, 0.0)

        if logx:
            # On a log axis Plotly interprets x positions and bar widths
            # in log10 units: centre on the geometric mean, width = decade
            # span of the bin.
            centers = np.sqrt(edges[:-1] * edges[1:])
            bar_widths = np.log10(edges[1:]) - np.log10(edges[:-1])
        else:
            centers = 0.5 * (edges[:-1] + edges[1:])
            bar_widths = lin_widths

        if dashed:
            # A "dashed bar chart" is unreadable, so the overlay trace
            # always uses a dashed line on top of the live bars.
            return (
                go.Scatter(
                    x=centers, y=y,
                    mode="lines",
                    line=dict(color=color, dash="dash"),
                    name=label,
                ),
                {},
            )
        # Per-channel histograms (each bin is a channel) surface the channel
        # number in the hover instead of the raw bin centre. Plain value
        # histograms (e.g. ADC_inclusive, whose x is an ADC count) keep
        # Plotly's default x/y hover.
        hover: dict = {}
        if per_channel:
            channels = np.arange(len(centers))
            if channels_per_board > 0:
                # Also show board number and channel-within-board. customdata
                # columns: [global channel, board, local channel].
                board, local = np.divmod(channels, channels_per_board)
                hover = dict(
                    customdata=np.stack([channels, board, local], axis=-1),
                    hovertemplate=(
                        "ch %{customdata[0]}<br>"
                        "board %{customdata[1]} · ch %{customdata[2]}<br>"
                        "%{y:.4g}<extra></extra>"
                    ),
                )
            else:
                hover = dict(
                    customdata=channels,
                    hovertemplate="ch %{customdata}<br>%{y:.4g}<extra></extra>",
                )
        return (
            go.Bar(
                x=centers, y=y, width=bar_widths,
                marker=dict(color=color, line=dict(width=0)),
                name=label,
                **hover,
            ),
            {"bargap": 0},
        )

    # TH2 / TProfile2D and anything else: not implemented yet.
    return None, {}


def _flow_trace(decoded: DecodedHist, scale: float = 1.0) -> Optional[go.Bar]:
    """A small bar trace for ROOT's underflow/overflow bins, drawn just outside
    the histogram range (one extra bar on each side). Only the non-empty side(s)
    are shown; returns None when both are empty. `scale` matches the main trace's
    area normalization so the flow bars stay on the same y scale."""
    edges = decoded.edges
    if edges.size < 2:
        return None
    w0 = edges[1] - edges[0]
    wn = edges[-1] - edges[-2]
    points = []  # (center, value, width, label)
    if decoded.underflow:
        points.append((edges[0] - 0.5 * w0, float(decoded.underflow) * scale, w0, "underflow"))
    if decoded.overflow:
        points.append((edges[-1] + 0.5 * wn, float(decoded.overflow) * scale, wn, "overflow"))
    if not points:
        return None
    xs, ys, widths, labels = zip(*points)
    return go.Bar(
        x=list(xs), y=list(ys), width=list(widths),
        marker=dict(color=theme.WARN, line=dict(width=0)),
        name="under/overflow",
        text=list(labels),
        hovertemplate="%{text}<br>%{y:.4g}<extra></extra>",
        showlegend=False,
    )
