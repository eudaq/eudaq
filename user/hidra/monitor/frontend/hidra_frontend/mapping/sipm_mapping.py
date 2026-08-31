from __future__ import annotations

import json

from pathlib import Path


class SiPMMapping:
    """FERS/SiPM channel mapping: global channel index -> detector position.

    The raw map lives in the bundled `sipm_channels.json`, a compact
    ``{idx: [col, row, type, module]}`` object where:

    - ``idx`` is the global FERS channel index ``boardID * 64 + ch`` — the
      same index the FERS histograms are filled with, so it can be used
      directly to look up a per-channel mean;
    - ``col`` / ``row`` are the global fiber column / row in the SiPM
      detector grid (the spatial position of the channel);
    - ``type`` is the fiber type, ``"S"`` (scintillation) or ``"C"``
      (Cherenkov) — both share the same grid, interleaved by row;
    - ``module`` is the calo module the channel belongs to.

    The JSON was generated from
    ``hidraTBAnalysis.mapping_sipm.get_sipm_channel_info(year=2026)``
    (columns ``fiber_col_global`` / ``fiber_row_global``). Regenerate it
    when the SiPM wiring changes.
    """

    def __init__(self, sipm_channels_file: str | Path) -> None:
        with open(sipm_channels_file, encoding="utf-8") as file:
            raw: dict[str, list] = json.load(file)
        self._channels: dict[int, dict[str, int | str]] = {
            int(idx): {
                "channel": int(idx),
                "column": int(col),
                "row": int(row),
                "type": str(typ),
                "module": str(module),
            }
            for idx, (col, row, typ, module) in raw.items()
        }

    def get_sipm_channels_info(self) -> dict[int, dict[str, int | str]]:
        """Channel index -> {channel, column, row, type, module}."""
        return self._channels
