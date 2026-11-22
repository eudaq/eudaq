"""Decoder registry.

The pure decoder is always available. The pyroot decoder is registered
only if `import ROOT` succeeds, so the frontend can run without PyROOT
installed.
"""

from __future__ import annotations

from .base import Decoder, DecodedHist, DecoderError, rebin_decoded
from .pure import PureDecoder

DECODERS: dict[str, type[Decoder]] = {"pure": PureDecoder}

try:  # pragma: no cover — optional dependency
    from .pyroot import PyRootDecoder

    DECODERS["pyroot"] = PyRootDecoder
except ImportError:
    pass


def get_decoder(name: str) -> Decoder:
    try:
        cls = DECODERS[name]
    except KeyError as exc:
        raise ValueError(
            f"unknown decoder '{name}'; available: {sorted(DECODERS)}"
        ) from exc
    return cls()


__all__ = ["Decoder", "DecodedHist", "DecoderError", "rebin_decoded", "DECODERS", "get_decoder"]
