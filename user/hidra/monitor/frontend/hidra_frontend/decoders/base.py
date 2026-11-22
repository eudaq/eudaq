"""Decoder interface and shared data class.

Both the pure-Python and the PyROOT backends produce the same
`DecodedHist` so the figure builder doesn't have to branch on the
backend.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Optional

import numpy as np


@dataclass
class DecodedHist:
    """Backend-agnostic decoded histogram."""

    name: str
    title: str
    typename: str  # e.g. "TH1D", "TProfile", "TH2F", "TProfile2D"
    # 1D fields
    edges: np.ndarray = field(default_factory=lambda: np.empty(0))
    counts: np.ndarray = field(default_factory=lambda: np.empty(0))
    errors: Optional[np.ndarray] = None
    # Out-of-range bin contents (ROOT's underflow/overflow), for display when
    # a panel opts into showing them. Default 0 when a decoder doesn't set them.
    underflow: float = 0.0
    overflow: float = 0.0
    # 2D fields (filled only for TH2 / TProfile2D)
    x_edges: Optional[np.ndarray] = None
    y_edges: Optional[np.ndarray] = None
    z: Optional[np.ndarray] = None


def rebin_decoded(hist: DecodedHist, factor: int) -> DecodedHist:
    """Merge groups of ``factor`` adjacent bins into a coarser 1D histogram.

    Used by the frontend rebin control (×1/×2/×4/×8). A no-op when ``factor``
    is <= 1, the histogram isn't a 1D TH1 (TProfile means / 2D have different
    semantics and must not be summed), or there are no bins. Counts are summed
    per group; if the bin count isn't a multiple of ``factor`` the trailing
    remainder is merged into the last bin (like ROOT's TH1::Rebin). Errors, when
    present, are combined in quadrature. under/overflow and metadata are kept.
    """
    if factor <= 1 or not hist.typename.startswith("TH1"):
        return hist
    edges = np.asarray(hist.edges, dtype=np.float64)
    counts = np.asarray(hist.counts, dtype=np.float64)
    n = counts.size
    if n < 2 or edges.size != n + 1:
        return hist

    # Group boundaries 0, factor, 2*factor, ...; reduceat sums [start:next).
    starts = np.arange(0, n, factor)
    new_counts = np.add.reduceat(counts, starts)
    # New edges: the left edge of each group plus the original last edge.
    new_edges = np.append(edges[starts], edges[-1])

    new_errors = None
    if hist.errors is not None:
        err = np.asarray(hist.errors, dtype=np.float64)
        if err.size == n:
            new_errors = np.sqrt(np.add.reduceat(err * err, starts))

    return DecodedHist(
        name=hist.name,
        title=hist.title,
        typename=hist.typename,
        edges=new_edges,
        counts=new_counts,
        errors=new_errors,
        underflow=hist.underflow,
        overflow=hist.overflow,
    )


class DecoderError(Exception):
    pass


class Decoder:
    """Decoder protocol — implement `decode(obj_dict) -> DecodedHist`."""

    def decode(self, obj_dict: dict) -> DecodedHist:
        raise NotImplementedError
