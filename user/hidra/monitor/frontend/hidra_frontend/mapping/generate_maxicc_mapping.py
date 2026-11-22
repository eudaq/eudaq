#!/usr/bin/env python3
"""Generate the MAXICC channel-mapping JSON from the source Excel.

MAXICC is the crystal EM calorimeter read out by the three new FERS boards
(locally B0/B1/B2). It has two longitudinal segments: the **rear** is read
twice (SiPM 15 µm and 50 µm) and the **front** once (15 µm), so there are three
channel tables. Each is a 9x9 "diamond" grid (rows 0-8, columns 0-8) with one
cell per channel, written as ``B<board> - CH<ch>``.

This script reads ``MAXICC_Mapping_for_HIDRA.xlsx`` (single sheet ``Sheet1``,
the three tables laid out at fixed anchors) and writes a single
``maxicc_channels.json``, nested by **board then channel** (local board id
0/1/2), with the geometric position ``[layer, col, row]`` as the value::

    { "0": { "28": [1, 3, 0], ... }, "1": { ... }, "2": { ... } }

- ``layer`` distinguishes the three readout planes: **0 = front 15 µm,
  1 = rear 15 µm, 2 = rear 50 µm** (front is the upstream segment; the two
  rear layers are the two readouts of the same rear segment).
- ``col`` / ``row`` are the 0-8 grid indices, row 0 at the top, exactly as
  drawn — the front is intentionally *not* mirrored.

The board/channel are kept separate (no global ``board*64 + ch`` index here);
the frontend computes that index when it needs to match a FERS histogram. Each
physical FERS channel belongs to exactly one plane, so ``(board, ch)`` is a
unique key across all three tables.

Regenerate when the wiring changes::

    cd user/hidra/monitor/frontend
    .venv/bin/python hidra_frontend/mapping/generate_maxicc_mapping.py \
        /path/to/MAXICC_Mapping_for_HIDRA.xlsx
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

import openpyxl

# Default location of the source spreadsheet (outside the repo).
DEFAULT_XLSX = "/home/turra/Scaricati/MAXICC_Mapping_for_HIDRA.xlsx"

# Where the generated JSON goes (this script's own directory).
OUT_DIR = Path(__file__).resolve().parent
OUT_FILE = "maxicc_channels.json"

# A grid cell, e.g. "B0 - CH28", tolerant of the spacing seen in the sheet
# ("B2 -CH4", "B2-CH0", "B2 - CH40").
CELL_RE = re.compile(r"B\s*(\d+)\s*-\s*CH\s*(\d+)", re.IGNORECASE)

# Each table is a 9x9 block. Anchors are the (row, col) of the top-left grid
# cell (1-based, as openpyxl uses), determined from the sheet layout:
#   - FRONT 15 µm: grid cols B..J (2..10), data rows 15..23  -> (15, 2)
#   - REAR 15 µm : grid cols B..J (2..10), data rows 3..11   -> (3, 2)
#   - REAR 50 µm : grid cols M..U (13..21), data rows 3..11  -> (3, 13)
# Each entry: (layer, label, top_row, left_col, expected_boards).
GRID = 9
TABLES = [
    (0, "FRONT 15um", 15, 2, {2}),
    (1, "REAR 15um", 3, 2, {0, 1}),
    (2, "REAR 50um", 3, 13, {0, 1}),
]
EXPECTED_CELLS = 61


def parse_table(ws, layer: int, top_row: int, left_col: int, label: str,
                expected_boards: set[int]):
    """Parse one 9x9 table into ``{board: {ch: [layer, col, row]}}``, validated."""
    by_board: dict[int, dict[int, list[int]]] = {}
    seen_pos: dict[tuple[int, int], tuple[int, int]] = {}  # (col,row) -> (board,ch)
    n = 0
    for row_off in range(GRID):
        for col_off in range(GRID):
            value = ws.cell(top_row + row_off, left_col + col_off).value
            if value is None:
                continue
            m = CELL_RE.search(str(value).replace(" ", ""))
            if not m:
                continue
            board, ch = int(m.group(1)), int(m.group(2))
            pos = (col_off, row_off)

            chans = by_board.setdefault(board, {})
            if ch in chans:
                sys.exit(f"[{label}] duplicate channel B{board}-CH{ch} "
                         f"at {pos} and {chans[ch][1:]}")
            if pos in seen_pos:
                sys.exit(f"[{label}] position {pos} used by B{board}-CH{ch} "
                         f"and B{seen_pos[pos][0]}-CH{seen_pos[pos][1]}")
            seen_pos[pos] = (board, ch)
            chans[ch] = [layer, col_off, row_off]
            n += 1

    if n != EXPECTED_CELLS:
        sys.exit(f"[{label}] expected {EXPECTED_CELLS} channels, found {n}")
    if set(by_board) != expected_boards:
        sys.exit(f"[{label}] expected boards {sorted(expected_boards)}, "
                 f"found {sorted(by_board)}")

    counts = ", ".join(f"B{b}:{len(by_board[b])}" for b in sorted(by_board))
    print(f"[{label}] OK: {n} channels ({counts}), no position collisions")
    return by_board


def merge(tables: list[dict[int, dict[int, list[int]]]]) -> dict[int, dict[int, list[int]]]:
    """Merge per-table maps into one ``{board: {ch: [layer, col, row]}}``.

    Each physical FERS channel lives in exactly one plane, so a ``(board, ch)``
    appearing in two tables is a wiring/transcription error and aborts.
    """
    merged: dict[int, dict[int, list[int]]] = {}
    for table in tables:
        for board, chans in table.items():
            dst = merged.setdefault(board, {})
            for ch, pos in chans.items():
                if ch in dst:
                    sys.exit(f"channel B{board}-CH{ch} appears in two planes: "
                             f"layer {dst[ch][0]} and layer {pos[0]}")
                dst[ch] = pos
    return merged


def dumps_nested(by_board: dict[int, dict[int, list[int]]]) -> str:
    """Serialize ``{board: {ch: [layer, col, row]}}`` with one channel per line.

    Numerically sorted boards/channels and inline arrays keep the file compact
    and diff-friendly (plain ``json.dump(indent=2)`` would spread every triple
    over five lines). The output is still standard JSON.
    """
    boards = []
    for b in sorted(by_board):
        chans = by_board[b]
        lines = [f'    "{ch}": [{chans[ch][0]}, {chans[ch][1]}, {chans[ch][2]}]'
                 for ch in sorted(chans)]
        boards.append(f'  "{b}": {{\n' + ",\n".join(lines) + "\n  }")
    return "{\n" + ",\n".join(boards) + "\n}\n"


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("xlsx", nargs="?", default=DEFAULT_XLSX,
                        help=f"path to the source Excel (default: {DEFAULT_XLSX})")
    args = parser.parse_args()

    xlsx = Path(args.xlsx)
    if not xlsx.is_file():
        sys.exit(f"Excel file not found: {xlsx}")

    wb = openpyxl.load_workbook(xlsx, data_only=True)
    ws = wb["Sheet1"]

    tables = [parse_table(ws, layer, top_row, left_col, label, boards)
              for layer, label, top_row, left_col, boards in TABLES]
    merged = merge(tables)

    total = sum(len(c) for c in merged.values())
    counts = ", ".join(f"B{b}:{len(merged[b])}" for b in sorted(merged))
    print(f"[MERGED] {total} channels ({counts})")

    out_path = OUT_DIR / OUT_FILE
    out_path.write_text(dumps_nested(merged), encoding="utf-8")
    print(f"  wrote {out_path}")


if __name__ == "__main__":
    main()
