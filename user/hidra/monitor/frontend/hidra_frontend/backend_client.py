"""Thin client for the ROOT THttpServer monitor backend.

The backend serves histograms under `/Histograms/<name>/root.json` and
exposes a batch endpoint `POST /multi.json?number=N` whose body is a
newline-delimited list of subrequests. The trailing newline on the last
entry is mandatory (without it THttpServer silently drops the last item).
"""

from __future__ import annotations

import logging
from typing import Optional

import requests

logger = logging.getLogger(__name__)


# TODO(monitor_info): when the backend exposes GET /monitor_info.json,
# read pump_interval_ms from it on startup and clamp polling.floor_ms
# accordingly. Until then the static hint in config.yaml is the only
# source of truth.

# TODO(reset): when the backend exposes POST /reset (e.g. via
# THttpServer::RegisterCommand), add BackendClient.reset() and wire it
# to a "Reset histograms" button. Today DoReset() is only reachable
# from the EUDAQ RunControl GUI.


class BackendClient:
    def __init__(self, url: str, timeout_s: float = 2.0) -> None:
        self.url = url.rstrip("/")
        self.timeout_s = timeout_s

    def list_histograms(self) -> list[str]:
        """Return all histogram names registered under /Histograms.

        Used at startup to validate that names referenced by config.yaml
        actually exist on the server.
        """
        try:
            r = requests.get(f"{self.url}/h.json", timeout=self.timeout_s)
            r.raise_for_status()
            tree = r.json()
        except Exception as exc:
            logger.warning("list_histograms failed: %s", exc)
            return []

        for child in tree.get("_childs", []):
            if child.get("_name") == "Histograms":
                return [c["_name"] for c in child.get("_childs", [])]
        return []

    def histo_output_dir(self) -> Optional[str]:
        """The directory where the monitor saves its histogram snapshots.

        The monitor publishes it as a dedicated TNamed (`histo_output_dir`) whose
        title is the absolute directory; we read it via `root.json`. Returns None
        when the backend is unreachable or doesn't expose it (older monitor /
        snapshot saving disabled), so the caller can fall back to the config.
        """
        try:
            r = requests.get(f"{self.url}/histo_output_dir/root.json", timeout=self.timeout_s)
            r.raise_for_status()
            obj = r.json()
            # Guard the access inside the try: a non-dict body (a JSON array,
            # `null`, …) would otherwise raise AttributeError out of this method.
            title = obj.get("fTitle") if isinstance(obj, dict) else None
        except Exception as exc:
            logger.debug("histo_output_dir not available: %s", exc)
            return None
        return title or None

    def nbins_x(self, name: str) -> Optional[int]:
        """Number of x-axis bins of a registered histogram, via the exe.json
        GetNbinsX method (allowed on TH2s). Used once at startup to learn the
        channel count of a per-channel TH2 (its x axis) for the projection-mode
        channel selector. None on failure.
        """
        try:
            r = requests.get(
                f"{self.url}/Histograms/{name}/exe.json",
                params={"method": "GetNbinsX"},
                timeout=self.timeout_s,
            )
            r.raise_for_status()
            value = r.json()
        except Exception as exc:
            logger.warning("nbins_x(%s) failed: %s", name, exc)
            return None
        return int(value) if value is not None else None

    # Token marking a server-side TH2 ProjectionY request:
    # "<th2 name>#projy=<x bin>" (a single channel's 1D slice). It becomes an
    # exe.json sub-request in the same /multi.json batch as the plain names, so
    # the whole 2D histogram is never transferred (see issue #138).
    _PROJ_SEP = "#projy="

    @classmethod
    def _subrequest(cls, name: str) -> str:
        """Map a requested name to its /multi.json sub-request path."""
        th2, sep, xbin = name.partition(cls._PROJ_SEP)
        if sep:
            # Server-side ProjectionY of one x bin (= one channel) -> a 1D TH1.
            # `_destroy_result_` frees the temporary projection once it has been
            # serialised (ROOT maintainers' guidance on root-project/root#22501);
            # the per-TH2 target name keeps the sub-requests of one /multi.json
            # batch from sharing a target. It does NOT address the per-call
            # exe.json interpreter growth -- see #153 for that.
            return (
                f"Histograms/{th2}/exe.json?method=ProjectionY"
                f"&name=_proj_{th2}&firstxbin={xbin}&lastxbin={xbin}&_destroy_result_"
            )
        return f"Histograms/{name}/root.json"

    def fetch_multi(self, names: list[str]) -> dict[str, Optional[dict]]:
        """Batch-fetch the given names in one POST /multi.json.

        Returns a dict {name: obj_dict | None}. None means the server returned
        null for that sub-request (typically: histogram not registered). A
        `#projy=` token is fetched as a server-side TH2 projection (a 1D slice)
        within the same batch.
        """
        if not names:
            return {}

        body = "".join(self._subrequest(n) + "\n" for n in names)
        try:
            r = requests.post(
                f"{self.url}/multi.json",
                params={"number": len(names)},
                data=body.encode(),
                timeout=self.timeout_s,
            )
            r.raise_for_status()
            entries = r.json()
        except Exception as exc:
            logger.warning("fetch_multi failed: %s", exc)
            return {n: None for n in names}

        # Always return exactly one item per requested name. If the
        # backend sends back fewer entries than requested (partial
        # response, proxy truncation, server bug), zip() would silently
        # drop the trailing names — and the caller's status accounting
        # (n_total = len(data)) would be wrong too. Index by position
        # with a bounds check instead, filling any shortfall with None.
        if not isinstance(entries, list):
            logger.warning("fetch_multi: expected a JSON list, got %s", type(entries).__name__)
            entries = []
        if len(entries) != len(names):
            logger.warning(
                "fetch_multi: backend returned %d entries for %d requested names; "
                "missing ones treated as absent",
                len(entries), len(names),
            )
        return {
            name: (entries[i] if i < len(entries) and entries[i] else None)
            for i, name in enumerate(names)
        }
