"""Pure-Python decoder for the TBufferJSON payload.

The payload returned by ROOT's THttpServer is plain JSON: the field
names match the TH1/TProfile members (`fArray`, `fXaxis`, `fSumw2`,
`fBinEntries`, ...). We can read them directly with numpy and skip the
round-trip through `TBufferJSON.ConvertFromJSON` + GetBinContent calls.

Supports TH1 (any storage type) and TProfile (kERRORMEAN). TH2 /
TProfile2D will raise DecoderError until they're needed.
"""

from __future__ import annotations

import numpy as np

from .base import Decoder, DecodedHist, DecoderError


class PureDecoder(Decoder):
    def decode(self, obj_dict: dict) -> DecodedHist:
        typename = obj_dict.get("_typename", "")
        if typename == "TProfile":
            return self._decode_tprofile(obj_dict)
        if typename.startswith("TH1"):
            return self._decode_th1(obj_dict)
        if typename.startswith("TH2") or typename == "TProfile2D":
            raise DecoderError(f"{typename} not implemented in pure decoder yet")
        raise DecoderError(f"unsupported type: {typename!r}")

    def _decode_th1(self, d: dict) -> DecodedHist:
        n = d["fXaxis"]["fNbins"]
        arr = np.asarray(d["fArray"], dtype=np.float64)
        counts = arr[1:n + 1]

        errors = None
        sumw2 = d.get("fSumw2") or []
        if sumw2:
            sumw2_arr = np.asarray(sumw2, dtype=np.float64)[1:n + 1]
            errors = np.sqrt(np.maximum(sumw2_arr, 0.0))

        return DecodedHist(
            name=d.get("fName", ""),
            title=d.get("fTitle", ""),
            typename=d["_typename"],
            edges=_edges_from_xaxis(d["fXaxis"], n),
            counts=counts,
            errors=errors,
            # fArray layout: [underflow, bin_1, ..., bin_n, overflow].
            underflow=float(arr[0]) if arr.size else 0.0,
            overflow=float(arr[n + 1]) if arr.size > n + 1 else 0.0,
        )

    def _decode_tprofile(self, d: dict) -> DecodedHist:
        # TProfile stores three parallel buffers of size fNcells:
        #   fArray       = sum of weights * y
        #   fSumw2       = sum of (weights * y)^2  (only if Sumw2() called)
        #   fBinEntries  = sum of weights
        # Bin mean is fArray/fBinEntries; error mode kERRORMEAN gives
        # err = sqrt((fSumw2/n - mean^2) / n).
        n = d["fXaxis"]["fNbins"]

        sumw = np.asarray(d["fArray"], dtype=np.float64)[1:n + 1]

        bin_entries = d.get("fBinEntries") or []
        if not bin_entries:
            raise DecoderError("TProfile missing fBinEntries")
        nent = np.asarray(bin_entries, dtype=np.float64)[1:n + 1]

        sumw2 = d.get("fSumw2") or []
        sumw2_arr = np.asarray(sumw2, dtype=np.float64)[1:n + 1] if sumw2 else np.zeros(n)

        with np.errstate(divide="ignore", invalid="ignore"):
            mean = np.where(nent > 0, sumw / nent, 0.0)
            var = np.where(nent > 0, sumw2_arr / nent - mean * mean, 0.0)
            var = np.maximum(var, 0.0)
            err = np.where(nent > 0, np.sqrt(var / nent), 0.0)

        return DecodedHist(
            name=d.get("fName", ""),
            title=d.get("fTitle", ""),
            typename="TProfile",
            edges=_edges_from_xaxis(d["fXaxis"], n),
            counts=mean,
            errors=err,
        )


def _edges_from_xaxis(xaxis: dict, n: int) -> np.ndarray:
    xbins = xaxis.get("fXbins") or []
    if xbins:
        return np.asarray(xbins, dtype=np.float64)
    return np.linspace(xaxis["fXmin"], xaxis["fXmax"], n + 1)
