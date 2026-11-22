#!/usr/bin/env python3
"""
Live decoder for the HIDRA merged binary format written by EventSerializer.

The reader can follow a file while the DataCollector is writing it, prints one
diagnostic line per complete event, and stores decoded quantities in a ROOT
TTree.  Detector payload interpretation is intentionally split into small
functions so board-specific decoders can be extended without touching the file
tailing or ROOT output code.
"""

from __future__ import annotations

import argparse
import os
import signal
import struct
import sys
import time
from dataclasses import dataclass, field
from typing import BinaryIO, Callable, Dict, Iterable, List, Optional, Sequence


EVENT_MARKER = 0xB0BF
EVENT_HEADER_END_MARKER = 0xBBBB
EVENT_TRAILER = 0xD04E
DETECTOR_EVENT_MARKER = 0xDEDE
DETECTOR_EVENT_END_MARKER = 0xDDDD
EVENT_HEADER_SIZE = 65
DETECTOR_HEADER_SIZE = 31
MAX_DETECTORS = 8


class DecodeError(RuntimeError):
    pass


@dataclass
class Quantity:
    name: str
    value: float
    unit: str = ""


@dataclass
class DetectorEvent:
    det_id: int
    event_number: int
    spill_number: int
    event_time: int
    native_event_time: int
    payload: bytes
    quantities: List[Quantity] = field(default_factory=list)


@dataclass
class HidraEvent:
    file_offset: int
    version: int
    header_size: int
    trailer_size: int
    event_size: int
    run_number: int
    event_number: int
    spill_number: int
    event_time: int
    trigger_mask: int
    detector_mask: int
    detector_sizes: List[int]
    detectors: List[DetectorEvent]


def u16(data: bytes, offset: int) -> int:
    return struct.unpack_from("<H", data, offset)[0]


def u32(data: bytes, offset: int) -> int:
    return struct.unpack_from("<I", data, offset)[0]


def u64(data: bytes, offset: int) -> int:
    return struct.unpack_from("<Q", data, offset)[0]


def f64(data: bytes, offset: int) -> float:
    return struct.unpack_from("<d", data, offset)[0]


def active_detector_ids(mask: int) -> List[int]:
    return [det_id for det_id in range(MAX_DETECTORS) if mask & (1 << det_id)]


def find_detector_payload_end(record: bytes, payload_start: int, event_end: int) -> int:
    """Find the detector end marker.

    `event_end` is the offset of the top-level event trailer inside `record`.
    The serializer writes `0xDDDD` after every detector payload.  To reduce false
    positives inside payload bytes, accept only candidates followed by either the
    next detector marker or the top-level event trailer.
    """

    marker = struct.pack("<H", DETECTOR_EVENT_END_MARKER)
    pos = payload_start
    while True:
        idx = record.find(marker, pos, event_end)
        if idx < 0:
            raise DecodeError("detector end marker 0xDDDD not found")

        next_offset = idx + 2
        if next_offset == event_end:
            return idx
        if next_offset + 2 <= event_end and u16(record, next_offset) == DETECTOR_EVENT_MARKER:
            return idx
        pos = idx + 1


def parse_event_record(record: bytes, file_offset: int) -> HidraEvent:
    if len(record) < EVENT_HEADER_SIZE + 2:
        raise DecodeError("record is shorter than the fixed event header")
    if u16(record, 0) != EVENT_MARKER:
        raise DecodeError(f"bad event marker 0x{u16(record, 0):04x} at file offset {file_offset}")

    version = record[2]
    header_size = u32(record, 3)
    trailer_size = u32(record, 7)
    event_size = u32(record, 11)
    run_number = u16(record, 15)
    event_number = u32(record, 17)
    spill_number = u32(record, 21)
    event_time = u64(record, 25)
    trigger_mask = record[33]
    detector_mask = record[46]
    detector_sizes = [u16(record, 47 + 2 * det_id) for det_id in range(MAX_DETECTORS)]

    if event_size != len(record):
        raise DecodeError(f"record size mismatch: header says {event_size}, read {len(record)}")
    if header_size != EVENT_HEADER_SIZE:
        raise DecodeError(f"unexpected header size {header_size}")
    if trailer_size != 2:
        raise DecodeError(f"unexpected trailer size {trailer_size}")
    if u16(record, header_size - 2) != EVENT_HEADER_END_MARKER:
        raise DecodeError("bad event header end marker")

    trailer_offset = event_size - trailer_size
    if u16(record, trailer_offset) != EVENT_TRAILER:
        raise DecodeError("bad event trailer marker")

    detectors: List[DetectorEvent] = []
    pos = header_size
    while pos < trailer_offset:
        if pos + DETECTOR_HEADER_SIZE > trailer_offset:
            raise DecodeError("truncated detector header")
        if u16(record, pos) != DETECTOR_EVENT_MARKER:
            raise DecodeError(f"bad detector marker 0x{u16(record, pos):04x} at record offset {pos}")

        det_id = record[pos + 2]
        det_event_number = u32(record, pos + 3)
        det_spill_number = u32(record, pos + 7)
        det_event_time = u64(record, pos + 11)
        native_event_time = u64(record, pos + 19)
        payload_start = pos + DETECTOR_HEADER_SIZE
        payload_end = find_detector_payload_end(record, payload_start, trailer_offset)
        payload = record[payload_start:payload_end]

        detectors.append(
            DetectorEvent(
                det_id=det_id,
                event_number=det_event_number,
                spill_number=det_spill_number,
                event_time=det_event_time,
                native_event_time=native_event_time,
                payload=payload,
            )
        )
        pos = payload_end + 2

    mask_ids = set(active_detector_ids(detector_mask))
    parsed_ids = {det.det_id for det in detectors}
    if parsed_ids != mask_ids:
        raise DecodeError(f"detector mask {sorted(mask_ids)} does not match parsed detectors {sorted(parsed_ids)}")

    return HidraEvent(
        file_offset=file_offset,
        version=version,
        header_size=header_size,
        trailer_size=trailer_size,
        event_size=event_size,
        run_number=run_number,
        event_number=event_number,
        spill_number=spill_number,
        event_time=event_time,
        trigger_mask=trigger_mask,
        detector_mask=detector_mask,
        detector_sizes=detector_sizes,
        detectors=detectors,
    )


def read_exact_following(
    fh: BinaryIO, nbytes: int, follow: bool, poll_seconds: float, stop: Callable[[], bool]
) -> Optional[bytes]:
    chunks: List[bytes] = []
    remaining = nbytes
    while remaining > 0:
        chunk = fh.read(remaining)
        if chunk:
            chunks.append(chunk)
            remaining -= len(chunk)
            continue
        if not follow or stop():
            return None
        time.sleep(poll_seconds)
    return b"".join(chunks)


def iter_events(
    filename: str, follow: bool, poll_seconds: float, stop: Callable[[], bool]
) -> Iterable[HidraEvent]:
    with open(filename, "rb") as fh:
        while not stop():
            file_offset = fh.tell()
            prefix = read_exact_following(fh, 15, follow, poll_seconds, stop)
            if prefix is None:
                return

            if u16(prefix, 0) != EVENT_MARKER:
                raise DecodeError(f"bad event marker 0x{u16(prefix, 0):04x} at file offset {file_offset}")

            event_size = u32(prefix, 11)
            if event_size < EVENT_HEADER_SIZE + 2:
                raise DecodeError(f"invalid event size {event_size} at file offset {file_offset}")

            rest = read_exact_following(fh, event_size - len(prefix), follow, poll_seconds, stop)
            if rest is None:
                return

            yield parse_event_record(prefix + rest, file_offset)


def decode_generic(det: DetectorEvent) -> List[Quantity]:
    quantities = [
        Quantity("payload_bytes", float(len(det.payload)), "B"),
        Quantity("detector_event_time", float(det.event_time), "ns"),
    ]
    if det.native_event_time != 0xFFFFFFFFFFFFFFFF:
        quantities.extend(
            [
                Quantity("detector_native_event_time", float(det.native_event_time), "ns"),
                Quantity("detector_native_time_delta", float(det.native_event_time - det.event_time), "ns"),
            ]
        )
    return quantities


def decode_xdc_words(det: DetectorEvent) -> List[Quantity]:
    payload = det.payload
    quantities = decode_generic(det)
    if len(payload) % 4 != 0:
        quantities.append(Quantity("trailing_payload_bytes", float(len(payload) % 4), "B"))

    words = list(struct.unpack("<" + "I" * (len(payload) // 4), payload[: len(payload) - (len(payload) % 4)]))
    quantities.append(Quantity("xdc_data_words", float(len(words)), "words"))
    if words:
        quantities.extend(
            [
                Quantity("xdc_first_word", float(words[0])),
                Quantity("xdc_last_word", float(words[-1])),
                Quantity("xdc_word_min", float(min(words))),
                Quantity("xdc_word_max", float(max(words))),
            ]
        )
    return quantities


def decode_fers_dry_or_legacy(det: DetectorEvent) -> List[Quantity]:
    quantities = decode_generic(det)
    payload = det.payload
    block_starts = []
    trigger_id_bytes = struct.pack("<Q", det.event_number)
    for offset in range(0, max(0, len(payload) - 16)):
        if payload.startswith(trigger_id_bytes, offset + 9):
            block_starts.append(offset)

    if block_starts:
        timestamps = [f64(payload, offset + 1) * 1000.0 for offset in block_starts]
        board_ids = [payload[offset] for offset in block_starts]
        board_mask = 0
        for board_id in board_ids:
            if board_id < 64:
                board_mask |= 1 << board_id
        quantities.extend(
            [
                Quantity("fers_blocks", float(len(block_starts))),
                Quantity("fers_board_mask", float(board_mask)),
                Quantity("fers_first_board_id", float(board_ids[0])),
                Quantity("fers_first_trigger_timestamp_ns", timestamps[0], "ns"),
                Quantity("fers_min_trigger_timestamp_ns", min(timestamps), "ns"),
                Quantity("fers_max_trigger_timestamp_ns", max(timestamps), "ns"),
                Quantity("fers_first_trigger_id", float(det.event_number)),
            ]
        )
    elif len(payload) >= 17:
        quantities.extend(
            [
                Quantity("fers_first_board_id", float(payload[0])),
                Quantity("fers_first_trigger_timestamp_ns", f64(payload, 1) * 1000.0, "ns"),
                Quantity("fers_first_trigger_id", float(u64(payload, 9))),
            ]
        )
    return quantities


def decode_fers2_spect_like(det: DetectorEvent) -> List[Quantity]:
    quantities = decode_generic(det)
    payload = det.payload
    if len(payload) >= 56:
        quantities.extend(
            [
                Quantity("fers2_tstamp_us", f64(payload, 0), "us"),
                Quantity("fers2_rel_tstamp_us", f64(payload, 8), "us"),
                Quantity("fers2_trigger_id", float(u64(payload, 32))),
                Quantity("fers2_chmask", float(u64(payload, 40))),
                Quantity("fers2_qdmask", float(u64(payload, 48))),
            ]
        )
    if len(payload) >= 56 + 64 * 2:
        energy_hg = struct.unpack_from("<64H", payload, 56)
        quantities.extend(
            [
                Quantity("fers2_energy_hg_sum", float(sum(energy_hg))),
                Quantity("fers2_energy_hg_max", float(max(energy_hg))),
            ]
        )
    return quantities


def decode_detector(det: DetectorEvent) -> List[Quantity]:
    """Dispatch detector payloads to specific decoding functions.

    The current Hidra configs commonly use detID 6 for dry FERS and detID 7 for
    dry XDC.  Real runs often use detID 0 for FERS and detID 1 for XDC.  The
    functions are deliberately independent and return name/value quantities.
    """

    if det.det_id in (1, 7):
        return decode_xdc_words(det)
    if det.det_id == 6:
        return decode_fers_dry_or_legacy(det)
    if det.det_id == 0:
        return decode_fers2_spect_like(det)
    return decode_generic(det)


def format_event_diagnostic(event: HidraEvent) -> str:
    parts = [
        f"run={event.run_number}",
        f"evt={event.event_number}",
        f"mask=0x{event.detector_mask:02x}",
        f"size={event.event_size}B",
    ]
    for det in event.detectors:
        interesting = {
            q.name: q.value
            for q in det.quantities
            if q.name
            in {
                "payload_bytes",
                "xdc_data_words",
                "fers_first_trigger_id",
                "fers2_trigger_id",
                "fers2_energy_hg_sum",
            }
        }
        summary = ", ".join(f"{key}={value:.0f}" for key, value in interesting.items())
        parts.append(f"det{det.det_id}({summary})")
    return " | ".join(parts)


class RootWriter:
    def __init__(self, output: str):
        try:
            import ROOT  # type: ignore
        except ImportError as exc:
            raise RuntimeError("PyROOT is required to write the ROOT TTree") from exc

        from array import array

        self.ROOT = ROOT
        self.array = array
        self.file = ROOT.TFile(output, "RECREATE")
        self.tree = ROOT.TTree("hidra", "HIDRA live decoded quantities")

        self.run = array("i", [0])
        self.event = array("I", [0])
        self.event_time = array("Q", [0])
        self.detector_mask = array("B", [0])
        self.event_size = array("I", [0])
        self.n_detectors = array("i", [0])

        self.q_det = ROOT.std.vector("int")()
        self.q_name = ROOT.std.vector("string")()
        self.q_value = ROOT.std.vector("double")()
        self.q_unit = ROOT.std.vector("string")()

        self.tree.Branch("run", self.run, "run/I")
        self.tree.Branch("event", self.event, "event/i")
        self.tree.Branch("event_time", self.event_time, "event_time/l")
        self.tree.Branch("detector_mask", self.detector_mask, "detector_mask/b")
        self.tree.Branch("event_size", self.event_size, "event_size/i")
        self.tree.Branch("n_detectors", self.n_detectors, "n_detectors/I")
        self.tree.Branch("q_det", self.q_det)
        self.tree.Branch("q_name", self.q_name)
        self.tree.Branch("q_value", self.q_value)
        self.tree.Branch("q_unit", self.q_unit)

    def fill(self, event: HidraEvent) -> None:
        self.run[0] = event.run_number
        self.event[0] = event.event_number
        self.event_time[0] = event.event_time
        self.detector_mask[0] = event.detector_mask
        self.event_size[0] = event.event_size
        self.n_detectors[0] = len(event.detectors)

        self.q_det.clear()
        self.q_name.clear()
        self.q_value.clear()
        self.q_unit.clear()

        for det in event.detectors:
            for quantity in det.quantities:
                self.q_det.push_back(det.det_id)
                self.q_name.push_back(quantity.name)
                self.q_value.push_back(quantity.value)
                self.q_unit.push_back(quantity.unit)

        self.tree.Fill()

    def close(self) -> None:
        self.file.cd()
        self.tree.Write()
        self.file.Close()


def main(argv: Optional[Sequence[str]] = None) -> int:
    parser = argparse.ArgumentParser(description="Follow and decode HIDRA EventSerializer binary files")
    parser.add_argument("input", help="HIDRA binary file to read")
    parser.add_argument("-o", "--output", default="hidra_live_decode.root", help="output ROOT file")
    parser.add_argument("--no-follow", action="store_true", help="read current file contents and exit")
    parser.add_argument("--poll", type=float, default=0.2, help="poll period while following a growing file")
    parser.add_argument("-n", "--max-events", type=int, default=0, help="stop after N complete events")
    parser.add_argument("--no-root", action="store_true", help="print diagnostics without writing a ROOT file")
    args = parser.parse_args(argv)

    if not os.path.exists(args.input):
        print(f"waiting for {args.input} to appear...", file=sys.stderr)
        while not os.path.exists(args.input):
            time.sleep(args.poll)

    stop_requested = False

    def request_stop(signum, frame):  # type: ignore[no-untyped-def]
        nonlocal stop_requested
        stop_requested = True

    signal.signal(signal.SIGINT, request_stop)
    signal.signal(signal.SIGTERM, request_stop)

    def should_stop() -> bool:
        return stop_requested

    writer: Optional[RootWriter] = None
    if not args.no_root:
        writer = RootWriter(args.output)

    count = 0
    try:
        for event in iter_events(args.input, not args.no_follow, args.poll, should_stop):
            for det in event.detectors:
                det.quantities = decode_detector(det)
            print(format_event_diagnostic(event), flush=True)
            if writer is not None:
                writer.fill(event)
            count += 1
            if args.max_events and count >= args.max_events:
                break
    finally:
        if writer is not None:
            writer.close()

    print(f"decoded {count} events")
    if writer is not None:
        print(f"wrote {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
