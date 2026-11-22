from __future__ import annotations

import json
from pathlib import Path


class MAXICCMapping:
    """MAXICC crystal-calorimeter channel mapping: (board, channel) -> position.

    The raw map lives in the bundled ``maxicc_channels.json``, nested by board
    then channel, with the geometric position ``[layer, col, row]`` as the value
    (see ``generate_maxicc_mapping.py``)::

        { "0": { "28": [1, 3, 0], ... }, "1": { ... }, "2": { ... } }

    - ``board`` / ``channel`` are **local** to MAXICC (board 0/1/2). The global
      FERS channel index is ``(board_offset + board) * 64 + channel``; the
      offset (the global index of MAXICC's first board) is applied by the
      consumer, since it depends on the DAQ wiring, not on the geometry.
    - ``layer`` is the readout plane: 0 = front 15 µm, 1 = rear 15 µm,
      2 = rear 50 µm.
    - ``col`` / ``row`` are the 0-8 grid indices, row 0 at the top, as drawn.

    Regenerate ``maxicc_channels.json`` with ``generate_maxicc_mapping.py`` when
    the wiring changes.
    """

    def __init__(self, maxicc_channels_file: str | Path) -> None:
        with open(maxicc_channels_file, encoding="utf-8") as file:
            raw: dict[str, dict[str, list[int]]] = json.load(file)
        self._channels: list[dict[str, int]] = [
            {
                "board": int(board),
                "channel": int(ch),
                "layer": int(layer),
                "col": int(col),
                "row": int(row),
            }
            for board, chans in raw.items()
            for ch, (layer, col, row) in chans.items()
        ]

    def get_channels_info(self) -> list[dict[str, int]]:
        """List of ``{board, channel, layer, col, row}`` records, one per channel."""
        return self._channels
