"""Fallback decoder that goes through ROOT's TBufferJSON.

Kept as a reference / safety net: if the pure decoder gets a payload
shape it doesn't recognise, you can switch `decoder: pyroot` in
config.yaml and still read it.

This decoder serialises the dict back to a JSON string, hands it to
ROOT, then reads the rebuilt TH1 in C++. Slower than the pure path
(2-200x depending on type) and adds a global lock because PyROOT is
not thread-safe.
"""

from __future__ import annotations

import json
import threading
from typing import Optional

import numpy as np

from .base import Decoder, DecodedHist, DecoderError

# Storage dtype for each TH1 storage variant (TH1::GetArray returns a
# typed pointer; numpy needs the matching dtype or it sees garbage).
_TH1_DTYPE = {
    "TH1C": np.int8,
    "TH1S": np.int16,
    "TH1I": np.int32,
    "TH1L": np.int64,
    "TH1F": np.float32,
    "TH1D": np.float64,
}


class PyRootDecoder(Decoder):
    def __init__(self) -> None:
        import ROOT  # noqa: PLC0415  (lazy import — keeps PureDecoder runnable without PyROOT)

        ROOT.gROOT.SetBatch(True)
        self._ROOT = ROOT
        self._lock = threading.Lock()

    def decode(self, obj_dict: dict) -> DecodedHist:
        typename = obj_dict.get("_typename", "")
        with self._lock:
            h = self._ROOT.TBufferJSON.ConvertFromJSON(json.dumps(obj_dict))
            if not h:
                raise DecoderError("ConvertFromJSON returned null")

            if typename == "TProfile":
                return self._decode_tprofile(h, obj_dict)
            if typename.startswith("TH1"):
                return self._decode_th1(h, obj_dict, typename)

        raise DecoderError(f"{typename} not implemented in pyroot decoder yet")

    def _decode_th1(self, h, d: dict, typename: str) -> DecodedHist:
        n = h.GetNbinsX()
        dtype = _TH1_DTYPE.get(typename, np.float64)
        counts = np.frombuffer(h.GetArray(), dtype=dtype, count=h.GetSize())[1:n + 1].astype(np.float64, copy=True)

        errors: Optional[np.ndarray] = None
        sumw2 = d.get("fSumw2") or []
        if sumw2:
            sumw2_arr = np.asarray(sumw2, dtype=np.float64)[1:n + 1]
            errors = np.sqrt(np.maximum(sumw2_arr, 0.0))

        return DecodedHist(
            name=h.GetName(),
            title=h.GetTitle(),
            typename=typename,
            edges=_edges(h.GetXaxis(), n),
            counts=counts,
            errors=errors,
            # Out-of-range bins, so `show_flow` works with this decoder too.
            underflow=float(h.GetBinContent(0)),
            overflow=float(h.GetBinContent(n + 1)),
        )

    def _decode_tprofile(self, h, d: dict) -> DecodedHist:
        n = h.GetNbinsX()
        counts = np.fromiter((h.GetBinContent(i) for i in range(1, n + 1)), dtype=np.float64, count=n)
        errors = np.fromiter((h.GetBinError(i) for i in range(1, n + 1)), dtype=np.float64, count=n)
        return DecodedHist(
            name=h.GetName(),
            title=h.GetTitle(),
            typename="TProfile",
            edges=_edges(h.GetXaxis(), n),
            counts=counts,
            errors=errors,
        )


def _edges(xaxis, n: int) -> np.ndarray:
    xbins = xaxis.GetXbins()
    if xbins.GetSize() > 0:
        return np.frombuffer(xbins.GetArray(), dtype=np.float64, count=xbins.GetSize()).copy()
    return np.linspace(xaxis.GetXmin(), xaxis.GetXmax(), n + 1)
