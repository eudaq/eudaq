#!/usr/bin/env python3
"""
Parse HIDRA .raw files written by EventSerializer.

This tool focuses on the container format documented in user/hidra/DataFormat.md:
it validates event and detector headers, prints aggregate header statistics, and
can produce a colored byte-by-byte hex dump where event headers, detector headers,
payloads, detector trailers, and event trailers are highlighted separately.
"""

from __future__ import annotations

import argparse
import collections
import os
import statistics
import struct
import sys
from dataclasses import dataclass, field
from typing import BinaryIO, Counter, Dict, Iterable, List, Optional, Sequence, Set, Tuple


EVENT_MARKER = 0xB0BF
EVENT_HEADER_END_MARKER = 0xBBBB
EVENT_TRAILER = 0xD04E
DETECTOR_EVENT_MARKER = 0xDEDE
DETECTOR_EVENT_END_MARKER = 0xDDDD

DATA_FORMAT_VERSION = 7
EVENT_HEADER_SIZE = 65
EVENT_TRAILER_SIZE = 2
DETECTOR_HEADER_SIZE = 31
MAX_DETECTORS = 8

ENDIANNESS_NAMES = {
    0x01: "little",
    0x02: "big",
    0xFF: "unspecified",
}

ANSI_RESET = "\033[0m"
ANSI_DIM = "\033[2m"
ANSI_BOLD = "\033[1m"
REGION_STYLES = {
    "event_header": "\033[1;34m",
    "event_trailer": "\033[1;35m",
    "detector_end": "\033[1;31m",
    "unknown": "\033[2m",
}
DETECTOR_HEADER_STYLES = [
    "\033[36m",
    "\033[96m",
    "\033[94m",
    "\033[35m",
    "\033[95m",
    "\033[33m",
    "\033[31m",
    "\033[37m",
]
DETECTOR_PAYLOAD_STYLES = [
    "\033[32m",
    "\033[92m",
    "\033[33m",
    "\033[93m",
    "\033[36m",
    "\033[96m",
    "\033[35m",
    "\033[95m",
]


class HidraRawError(RuntimeError):
    """Raised when a raw file does not match the HIDRA binary container."""


@dataclass(frozen=True)
class Span:
    start: int
    end: int
    kind: str
    label: str
    det_id: Optional[int] = None

    def contains(self, offset: int) -> bool:
        return self.start <= offset < self.end


@dataclass
class DetectorRecord:
    offset: int
    size_source: str
    marker: int
    detector_id: int
    event_number: int
    spill_number: int
    event_time: int
    native_event_time: int
    reserved16: int
    reserved8: int
    endianness: int
    payload_offset: int
    payload_size: int
    end_marker_offset: int
    end_marker: int
    warnings: List[str] = field(default_factory=list)

    @property
    def end_offset(self) -> int:
        return self.end_marker_offset + 2

    @property
    def native_time_delta(self) -> int:
        if self.native_event_time == 0xFFFFFFFFFFFFFFFF:
            return 0
        return self.native_event_time - self.event_time

    @property
    def endianness_name(self) -> str:
        return ENDIANNESS_NAMES.get(self.endianness, f"unknown(0x{self.endianness:02x})")


@dataclass
class EventRecord:
    index: int
    file_offset: int
    marker: int
    data_version: int
    header_size: int
    trailer_size: int
    event_size: int
    run_number: int
    event_number: int
    spill_number: int
    event_time: int
    trigger_mask: int
    reserved64: int
    reserved32: int
    detector_mask: int
    detector_sizes: List[int]
    header_end_marker: int
    detectors: List[DetectorRecord]
    trailer_marker: int
    warnings: List[str] = field(default_factory=list)
    record: bytes = b""

    @property
    def active_detector_ids(self) -> List[int]:
        return [det_id for det_id in range(MAX_DETECTORS) if self.detector_mask & (1 << det_id)]

    @property
    def payload_bytes(self) -> int:
        return sum(det.payload_size for det in self.detectors)

    @property
    def spans(self) -> List[Span]:
        spans = [Span(0, self.header_size, "event_header", "event header")]
        for det in self.detectors:
            spans.append(
                Span(
                    det.offset,
                    det.payload_offset,
                    "detector_header",
                    f"det{det.detector_id} header",
                    det.detector_id,
                )
            )
            spans.append(
                Span(
                    det.payload_offset,
                    det.payload_offset + det.payload_size,
                    "detector_payload",
                    f"det{det.detector_id} payload",
                    det.detector_id,
                )
            )
            spans.append(
                Span(
                    det.end_marker_offset,
                    det.end_marker_offset + 2,
                    "detector_end",
                    f"det{det.detector_id} end",
                    det.detector_id,
                )
            )
        trailer_start = self.event_size - self.trailer_size
        spans.append(Span(trailer_start, self.event_size, "event_trailer", "event trailer"))
        return spans


@dataclass
class ParseOptions:
    strict: bool = False
    use_size_table: bool = True
    resync: bool = False
    keep_records: bool = False


class RawReader:
    def __init__(self, filename: str, options: ParseOptions):
        self.filename = filename
        self.options = options

    def iter_events(self, max_events: int = 0) -> Iterable[EventRecord]:
        with open(self.filename, "rb") as fh:
            index = 0
            while True:
                if max_events and index >= max_events:
                    return
                event = self._read_one(fh, index)
                if event is None:
                    return
                yield event
                index += 1

    def _read_one(self, fh: BinaryIO, index: int) -> Optional[EventRecord]:
        file_offset = fh.tell()
        prefix = fh.read(15)
        if not prefix:
            return None
        if len(prefix) < 15:
            raise HidraRawError(f"truncated event prefix at file offset {file_offset}")

        marker = u16(prefix, 0)
        if marker != EVENT_MARKER:
            if not self.options.resync:
                raise HidraRawError(
                    f"bad event marker 0x{marker:04x} at file offset {file_offset}; "
                    "use --resync to scan for the next marker"
                )
            found_offset = self._resync_to_marker(fh, prefix, file_offset)
            if found_offset is None:
                return None
            file_offset = found_offset
            prefix = fh.read(15)
            if len(prefix) < 15:
                raise HidraRawError(f"truncated event prefix at file offset {file_offset}")

        event_size = u32(prefix, 11)
        if event_size < EVENT_HEADER_SIZE + EVENT_TRAILER_SIZE:
            raise HidraRawError(f"invalid event size {event_size} at file offset {file_offset}")

        rest = fh.read(event_size - len(prefix))
        if len(rest) != event_size - len(prefix):
            raise HidraRawError(
                f"truncated event at file offset {file_offset}: "
                f"header says {event_size} bytes, only read {len(prefix) + len(rest)}"
            )

        return parse_event(prefix + rest, index, file_offset, self.options)

    @staticmethod
    def _resync_to_marker(fh: BinaryIO, prefix: bytes, file_offset: int) -> Optional[int]:
        marker_bytes = struct.pack("<H", EVENT_MARKER)
        window = bytearray(prefix)
        scan_offset = file_offset
        while True:
            pos = bytes(window).find(marker_bytes)
            if pos >= 0:
                found_offset = scan_offset + pos
                fh.seek(found_offset)
                return found_offset
            chunk = fh.read(4096)
            if not chunk:
                return None
            if len(window) > 1:
                scan_offset += len(window) - 1
                window = window[-1:]
            window.extend(chunk)


def u16(data: bytes, offset: int) -> int:
    return struct.unpack_from("<H", data, offset)[0]


def u32(data: bytes, offset: int) -> int:
    return struct.unpack_from("<I", data, offset)[0]


def u64(data: bytes, offset: int) -> int:
    return struct.unpack_from("<Q", data, offset)[0]


def parse_event(record: bytes, index: int, file_offset: int, options: ParseOptions) -> EventRecord:
    warnings: List[str] = []
    if len(record) < EVENT_HEADER_SIZE + EVENT_TRAILER_SIZE:
        raise HidraRawError(f"event {index} at {file_offset} is too short")

    marker = u16(record, 0)
    data_version = record[2]
    header_size = u32(record, 3)
    trailer_size = u32(record, 7)
    event_size = u32(record, 11)
    run_number = u16(record, 15)
    event_number = u32(record, 17)
    spill_number = u32(record, 21)
    event_time = u64(record, 25)
    trigger_mask = record[33]
    reserved64 = u64(record, 34)
    reserved32 = u32(record, 42)
    detector_mask = record[46]
    detector_sizes = [u16(record, 47 + 2 * det_id) for det_id in range(MAX_DETECTORS)]
    header_end_marker = u16(record, 63)

    if marker != EVENT_MARKER:
        raise HidraRawError(f"event {index}: bad event marker 0x{marker:04x}")
    add_warning(warnings, data_version != DATA_FORMAT_VERSION, f"dataVersion is {data_version}, expected {DATA_FORMAT_VERSION}")
    add_warning(warnings, header_size != EVENT_HEADER_SIZE, f"headerSize is {header_size}, expected {EVENT_HEADER_SIZE}")
    add_warning(warnings, trailer_size != EVENT_TRAILER_SIZE, f"trailerSize is {trailer_size}, expected {EVENT_TRAILER_SIZE}")
    add_warning(warnings, event_size != len(record), f"eventSize is {event_size}, record length is {len(record)}")
    add_warning(warnings, header_end_marker != EVENT_HEADER_END_MARKER, f"bad header end marker 0x{header_end_marker:04x}")
    add_warning(warnings, reserved64 != 0, f"reserved64 is nonzero: 0x{reserved64:016x}")
    add_warning(warnings, reserved32 != 0, f"reserved32 is nonzero: 0x{reserved32:08x}")

    if options.strict and warnings:
        raise HidraRawError(f"event {index}: " + "; ".join(warnings))

    trailer_offset = event_size - trailer_size
    if trailer_offset < header_size or trailer_offset + 2 > len(record):
        raise HidraRawError(f"event {index}: invalid trailer offset {trailer_offset}")
    trailer_marker = u16(record, trailer_offset)
    add_warning(warnings, trailer_marker != EVENT_TRAILER, f"bad event trailer marker 0x{trailer_marker:04x}")

    detectors = parse_detectors(record, header_size, trailer_offset, detector_mask, detector_sizes, options, warnings, index)
    parsed_ids = [det.detector_id for det in detectors]
    active_ids = [det_id for det_id in range(MAX_DETECTORS) if detector_mask & (1 << det_id)]
    add_warning(warnings, sorted(parsed_ids) != active_ids, f"detector mask {active_ids} but parsed detector ids {parsed_ids}")

    if options.strict and warnings:
        raise HidraRawError(f"event {index}: " + "; ".join(warnings))

    return EventRecord(
        index=index,
        file_offset=file_offset,
        marker=marker,
        data_version=data_version,
        header_size=header_size,
        trailer_size=trailer_size,
        event_size=event_size,
        run_number=run_number,
        event_number=event_number,
        spill_number=spill_number,
        event_time=event_time,
        trigger_mask=trigger_mask,
        reserved64=reserved64,
        reserved32=reserved32,
        detector_mask=detector_mask,
        detector_sizes=detector_sizes,
        header_end_marker=header_end_marker,
        detectors=detectors,
        trailer_marker=trailer_marker,
        warnings=warnings,
        record=record if options.keep_records else b"",
    )


def parse_detectors(
    record: bytes,
    start: int,
    trailer_offset: int,
    detector_mask: int,
    detector_sizes: Sequence[int],
    options: ParseOptions,
    event_warnings: List[str],
    event_index: int,
) -> List[DetectorRecord]:
    detectors: List[DetectorRecord] = []
    pos = start
    active_ids = [det_id for det_id in range(MAX_DETECTORS) if detector_mask & (1 << det_id)]

    for expected_det_id in active_ids:
        if pos >= trailer_offset:
            event_warnings.append(f"missing detector block for det{expected_det_id}")
            break
        detector = parse_detector_at(
            record,
            pos,
            trailer_offset,
            expected_det_id,
            detector_sizes[expected_det_id],
            options,
            event_index,
        )
        detectors.append(detector)
        pos = detector.end_offset

    if pos != trailer_offset:
        message = f"{trailer_offset - pos} unparsed bytes before event trailer"
        if pos > trailer_offset:
            message = f"detector parsing overran trailer by {pos - trailer_offset} bytes"
        event_warnings.append(message)
        if options.strict:
            raise HidraRawError(f"event {event_index}: {message}")

    return detectors


def parse_detector_at(
    record: bytes,
    pos: int,
    trailer_offset: int,
    expected_det_id: int,
    expected_detector_size: int,
    options: ParseOptions,
    event_index: int,
) -> DetectorRecord:
    if pos + DETECTOR_HEADER_SIZE + 2 > trailer_offset:
        raise HidraRawError(f"event {event_index}: truncated detector header at record offset {pos}")

    warnings: List[str] = []
    marker = u16(record, pos)
    detector_id = record[pos + 2]
    event_number = u32(record, pos + 3)
    spill_number = u32(record, pos + 7)
    event_time = u64(record, pos + 11)
    native_event_time = u64(record, pos + 19)
    reserved16 = u16(record, pos + 27)
    reserved8 = record[pos + 29]
    endianness = record[pos + 30]
    payload_offset = pos + DETECTOR_HEADER_SIZE

    add_warning(warnings, marker != DETECTOR_EVENT_MARKER, f"bad detector marker 0x{marker:04x}")
    add_warning(warnings, detector_id != expected_det_id, f"detector id is {detector_id}, expected {expected_det_id}")
    add_warning(warnings, reserved16 != 0, f"reserved16 is nonzero: 0x{reserved16:04x}")
    add_warning(warnings, reserved8 != 0, f"reserved8 is nonzero: 0x{reserved8:02x}")
    add_warning(
        warnings,
        endianness not in ENDIANNESS_NAMES,
        f"unknown endianness byte 0x{endianness:02x}",
    )

    payload_size = 0
    size_source = "detectorSize"
    end_marker_offset = payload_offset
    end_marker = None

    if (
        options.use_size_table
        and expected_detector_size != 0xFFFF
        and expected_detector_size >= DETECTOR_HEADER_SIZE + 2
        and pos + expected_detector_size <= trailer_offset
        and u16(record, pos + expected_detector_size - 2) == DETECTOR_EVENT_END_MARKER
    ):
        end_marker_offset = pos + expected_detector_size - 2
        payload_size = expected_detector_size - DETECTOR_HEADER_SIZE - 2
        end_marker = u16(record, end_marker_offset)
    elif (
        options.use_size_table
        and expected_detector_size != 0xFFFF
        and payload_offset + expected_detector_size + 2 <= trailer_offset
        and u16(record, payload_offset + expected_detector_size) == DETECTOR_EVENT_END_MARKER
    ):
        # Compatibility with files/tools that interpreted detectorSize as payload
        # bytes only, even though EventSerializer writes the full detector block.
        payload_size = expected_detector_size
        end_marker_offset = payload_offset + payload_size
        size_source = "detectorSize-payload"
        end_marker = u16(record, end_marker_offset)
    else:
        if options.use_size_table:
            warnings.append(
                f"detectorSize[{expected_det_id}]={expected_detector_size} did not land on 0xDDDD as block or payload size; scanned marker"
            )
        end_marker_offset = find_detector_end_marker(record, payload_offset, trailer_offset)
        payload_size = end_marker_offset - payload_offset
        size_source = "end-marker-scan"
        end_marker = u16(record, end_marker_offset)

    add_warning(warnings, end_marker != DETECTOR_EVENT_END_MARKER, f"bad detector end marker 0x{end_marker:04x}")

    if options.strict and warnings:
        raise HidraRawError(f"event {event_index}, detector at {pos}: " + "; ".join(warnings))

    return DetectorRecord(
        offset=pos,
        size_source=size_source,
        marker=marker,
        detector_id=detector_id,
        event_number=event_number,
        spill_number=spill_number,
        event_time=event_time,
        native_event_time=native_event_time,
        reserved16=reserved16,
        reserved8=reserved8,
        endianness=endianness,
        payload_offset=payload_offset,
        payload_size=payload_size,
        end_marker_offset=end_marker_offset,
        end_marker=end_marker,
        warnings=warnings,
    )


def find_detector_end_marker(record: bytes, payload_offset: int, trailer_offset: int) -> int:
    marker = struct.pack("<H", DETECTOR_EVENT_END_MARKER)
    pos = payload_offset
    while True:
        idx = record.find(marker, pos, trailer_offset)
        if idx < 0:
            raise HidraRawError(f"detector end marker 0xDDDD not found after record offset {payload_offset}")
        next_offset = idx + 2
        if next_offset == trailer_offset:
            return idx
        if next_offset + 2 <= trailer_offset and u16(record, next_offset) == DETECTOR_EVENT_MARKER:
            return idx
        pos = idx + 1


def add_warning(warnings: List[str], condition: bool, message: str) -> None:
    if condition:
        warnings.append(message)


@dataclass
class Stats:
    events: int = 0
    bytes_read: int = 0
    warnings: int = 0
    versions: Counter[int] = field(default_factory=collections.Counter)
    runs: Counter[int] = field(default_factory=collections.Counter)
    trigger_masks: Counter[int] = field(default_factory=collections.Counter)
    detector_masks: Counter[int] = field(default_factory=collections.Counter)
    detectors_per_event: Counter[int] = field(default_factory=collections.Counter)
    event_sizes: List[int] = field(default_factory=list)
    payload_sizes: List[int] = field(default_factory=list)
    event_numbers: List[int] = field(default_factory=list)
    event_times: List[int] = field(default_factory=list)
    det_counts: Counter[int] = field(default_factory=collections.Counter)
    det_payload_bytes: Counter[int] = field(default_factory=collections.Counter)
    det_payload_sizes: Dict[int, List[int]] = field(default_factory=lambda: collections.defaultdict(list))
    det_endianness: Dict[int, Counter[int]] = field(default_factory=lambda: collections.defaultdict(collections.Counter))
    det_size_source: Counter[str] = field(default_factory=collections.Counter)
    det_event_times: Dict[int, List[int]] = field(default_factory=lambda: collections.defaultdict(list))
    det_native_event_times: Dict[int, List[int]] = field(default_factory=lambda: collections.defaultdict(list))
    det_native_time_deltas: Dict[int, List[int]] = field(default_factory=lambda: collections.defaultdict(list))

    def add(self, event: EventRecord) -> None:
        self.events += 1
        self.bytes_read += event.event_size
        self.warnings += len(event.warnings) + sum(len(det.warnings) for det in event.detectors)
        self.versions[event.data_version] += 1
        self.runs[event.run_number] += 1
        self.trigger_masks[event.trigger_mask] += 1
        self.detector_masks[event.detector_mask] += 1
        self.detectors_per_event[len(event.detectors)] += 1
        self.event_sizes.append(event.event_size)
        self.payload_sizes.append(event.payload_bytes)
        self.event_numbers.append(event.event_number)
        self.event_times.append(event.event_time)
        for det in event.detectors:
            self.det_counts[det.detector_id] += 1
            self.det_payload_bytes[det.detector_id] += det.payload_size
            self.det_payload_sizes[det.detector_id].append(det.payload_size)
            self.det_endianness[det.detector_id][det.endianness] += 1
            self.det_size_source[det.size_source] += 1
            self.det_event_times[det.detector_id].append(det.event_time)
            if det.native_event_time != 0xFFFFFFFFFFFFFFFF:
                self.det_native_event_times[det.detector_id].append(det.native_event_time)
                self.det_native_time_deltas[det.detector_id].append(det.native_time_delta)


@dataclass
class AlignmentOptions:
    enabled: bool = False
    detail: str = "anomalies"
    max_details: int = 200
    time_tolerance: int = 0
    reference_detector: Optional[int] = None
    expected_detectors: Optional[Set[int]] = None


@dataclass
class AlignmentEvent:
    index: int
    file_offset: int
    detectors: Dict[int, DetectorRecord]
    duplicate_detector_ids: List[int]
    expected_detector_ids: List[int]
    reference_detector: Optional[int]

    @property
    def present_detector_ids(self) -> List[int]:
        return sorted(self.detectors)

    @property
    def missing_detector_ids(self) -> List[int]:
        return [det_id for det_id in self.expected_detector_ids if det_id not in self.detectors]

    @property
    def trigger_ids(self) -> Dict[int, int]:
        return {det_id: det.event_number for det_id, det in self.detectors.items()}

    @property
    def unique_trigger_ids(self) -> Set[int]:
        return set(self.trigger_ids.values())

    @property
    def one_to_one_trigger_match(self) -> bool:
        return (
            not self.missing_detector_ids
            and not self.duplicate_detector_ids
            and bool(self.detectors)
            and len(self.unique_trigger_ids) == 1
        )

    @property
    def offset_pattern(self) -> Tuple[Tuple[int, int], ...]:
        if self.reference_detector is None or self.reference_detector not in self.detectors:
            return ()
        ref_trigger = self.detectors[self.reference_detector].event_number
        return tuple(
            (det_id, det.event_number - ref_trigger)
            for det_id, det in sorted(self.detectors.items())
            if det_id != self.reference_detector
        )

    def status(self) -> str:
        parts = []
        if self.one_to_one_trigger_match:
            parts.append("1-to-1 trigger match")
        else:
            if self.missing_detector_ids:
                parts.append("missing " + ",".join(f"det{det_id}" for det_id in self.missing_detector_ids))
            if self.duplicate_detector_ids:
                parts.append("duplicate " + ",".join(f"det{det_id}" for det_id in self.duplicate_detector_ids))
            if len(self.unique_trigger_ids) > 1:
                parts.append("trigger mismatch")
            if not parts:
                parts.append("no detector events")
        if self.offset_pattern:
            offsets = ", ".join(f"det{det_id}:{offset:+d}" for det_id, offset in self.offset_pattern)
            parts.append(f"offsets vs det{self.reference_detector} [{offsets}]")
        return "; ".join(parts)


class AlignmentAnalyzer:
    def __init__(self, options: AlignmentOptions):
        self.options = options
        self.events: List[EventRecord] = []

    def add(self, event: EventRecord) -> None:
        self.events.append(event)

    def print_report(self) -> None:
        print()
        print("Detector alignment report")
        print("  Source fields: detector-local trigger IDs and timestamps only; event header trigger/time are ignored.")
        if not self.events:
            print("  No events parsed.")
            return

        expected = self._expected_detectors()
        if not expected:
            print("  No detector events were found.")
            return
        reference = self._reference_detector(expected)
        rows = [self._event_alignment(event, expected, reference) for event in self.events]
        one_to_one = sum(1 for row in rows if row.one_to_one_trigger_match)
        anomalous_rows = [row for row in rows if not row.one_to_one_trigger_match]

        print(f"  Expected detectors: {detector_list_text(expected)}")
        print(f"  Reference detector: det{reference}")
        print(f"  Events checked: {len(rows)}")
        print(f"  Events with 1-to-1 trigger-ID match: {one_to_one}")
        print(f"  Events with missing/duplicate/mismatched detector triggers: {len(anomalous_rows)}")
        self._print_per_detector_trigger_ranges(expected)
        self._print_trigger_coverage(expected)
        self._print_pairwise_alignment(expected)
        self._print_offset_segments(rows, reference)
        self._print_event_details(rows)

    def _expected_detectors(self) -> List[int]:
        if self.options.expected_detectors is not None:
            return sorted(self.options.expected_detectors)
        observed = {det.detector_id for event in self.events for det in event.detectors}
        return sorted(observed)

    def _reference_detector(self, expected: Sequence[int]) -> int:
        if self.options.reference_detector is not None:
            return self.options.reference_detector
        return expected[0]

    def _event_alignment(
        self,
        event: EventRecord,
        expected: Sequence[int],
        reference: Optional[int],
    ) -> AlignmentEvent:
        detectors: Dict[int, DetectorRecord] = {}
        duplicates: List[int] = []
        for det in event.detectors:
            if det.detector_id in detectors:
                duplicates.append(det.detector_id)
                continue
            detectors[det.detector_id] = det
        return AlignmentEvent(
            index=event.index,
            file_offset=event.file_offset,
            detectors=detectors,
            duplicate_detector_ids=duplicates,
            expected_detector_ids=list(expected),
            reference_detector=reference,
        )

    def _detector_entries(self) -> Dict[int, List[Tuple[int, int, int]]]:
        entries: Dict[int, List[Tuple[int, int, int]]] = collections.defaultdict(list)
        for event in self.events:
            for det in event.detectors:
                entries[det.detector_id].append((det.event_number, det.event_time, event.index))
        return entries

    def _trigger_index(self) -> Dict[int, Dict[int, List[Tuple[int, int]]]]:
        trigger_index: Dict[int, Dict[int, List[Tuple[int, int]]]] = collections.defaultdict(
            lambda: collections.defaultdict(list)
        )
        for event in self.events:
            for det in event.detectors:
                trigger_index[det.event_number][det.detector_id].append((event.index, det.event_time))
        return trigger_index

    def _print_per_detector_trigger_ranges(self, expected: Sequence[int]) -> None:
        entries = self._detector_entries()
        print()
        print("  Per-detector trigger streams:")
        for det_id in expected:
            det_entries = entries.get(det_id, [])
            triggers = [trigger for trigger, _time, _event_index in det_entries]
            duplicate_count = len(triggers) - len(set(triggers))
            monotonic_breaks = sum(
                1 for previous, current in zip(triggers, triggers[1:]) if current <= previous
            )
            print(
                f"    det{det_id}: events={len(triggers)} triggers={range_text(triggers)} "
                f"duplicates={duplicate_count} nonmonotonic_steps={monotonic_breaks}"
            )

    def _print_trigger_coverage(self, expected: Sequence[int]) -> None:
        trigger_index = self._trigger_index()
        complete = 0
        incomplete: List[Tuple[int, List[int], List[int]]] = []
        duplicate_triggers: List[Tuple[int, List[int]]] = []
        for trigger_id in sorted(trigger_index):
            present = sorted(det_id for det_id in trigger_index[trigger_id] if det_id in expected)
            missing = [det_id for det_id in expected if det_id not in trigger_index[trigger_id]]
            duplicates = [
                det_id for det_id in present if len(trigger_index[trigger_id][det_id]) > 1
            ]
            if missing:
                incomplete.append((trigger_id, present, missing))
            else:
                complete += 1
            if duplicates:
                duplicate_triggers.append((trigger_id, duplicates))

        print()
        print("  Trigger-ID coverage across detectors:")
        print(f"    unique detector-local trigger IDs: {len(trigger_index)}")
        print(f"    trigger IDs present in all expected detectors: {complete}")
        print(f"    trigger IDs missing at least one detector: {len(incomplete)}")
        print(f"    trigger IDs duplicated within a detector stream: {len(duplicate_triggers)}")

        self._print_limited_detail(
            incomplete,
            "    Missing trigger-ID details:",
            lambda item: (
                f"      trigger={item[0]} present={detector_list_text(item[1])} "
                f"missing={detector_list_text(item[2])}"
            ),
        )
        self._print_limited_detail(
            duplicate_triggers,
            "    Duplicate trigger-ID details:",
            lambda item: f"      trigger={item[0]} duplicated_in={detector_list_text(item[1])}",
        )

    def _print_pairwise_alignment(self, expected: Sequence[int]) -> None:
        entries = self._detector_entries()
        by_detector: Dict[int, Dict[int, List[int]]] = {}
        for det_id, det_entries in entries.items():
            by_trigger: Dict[int, List[int]] = collections.defaultdict(list)
            for trigger_id, timestamp, _event_index in det_entries:
                by_trigger[trigger_id].append(timestamp)
            by_detector[det_id] = by_trigger

        print()
        print("  Pairwise trigger and timestamp alignment:")
        drift_details: List[str] = []
        for i, det_a in enumerate(expected):
            for det_b in expected[i + 1 :]:
                triggers_a = set(by_detector.get(det_a, {}))
                triggers_b = set(by_detector.get(det_b, {}))
                common = sorted(triggers_a & triggers_b)
                deltas = [
                    by_detector[det_b][trigger_id][0] - by_detector[det_a][trigger_id][0]
                    for trigger_id in common
                    if by_detector[det_a][trigger_id] and by_detector[det_b][trigger_id]
                ]
                aligned = timestamp_deltas_aligned(deltas, self.options.time_tolerance)
                status = "aligned" if aligned else "timestamp drift"
                print(
                    f"    det{det_a}<->det{det_b}: common_triggers={len(common)} "
                    f"missing_in_det{det_a}={len(triggers_b - triggers_a)} "
                    f"missing_in_det{det_b}={len(triggers_a - triggers_b)} "
                    f"delta_t(det{det_b}-det{det_a})={series_text(deltas)} status={status}"
                )
                if deltas:
                    baseline = deltas[0]
                    for trigger_id, delta in zip(common, deltas):
                        if abs(delta - baseline) > self.options.time_tolerance:
                            drift_details.append(
                                f"      det{det_a}<->det{det_b} trigger={trigger_id}: "
                                f"delta_t={delta} expected={baseline} "
                                f"diff={delta - baseline:+d}"
                            )
        self._print_limited_detail(
            drift_details,
            "    Timestamp-drift details:",
            lambda item: item,
        )

    def _print_offset_segments(self, rows: Sequence[AlignmentEvent], reference: int) -> None:
        segments: List[Tuple[int, int, Tuple[Tuple[int, int], ...]]] = []
        start = None
        previous_pattern: Optional[Tuple[Tuple[int, int], ...]] = None
        previous_index = None
        for row in rows:
            pattern = row.offset_pattern
            if not pattern:
                continue
            if previous_pattern is None:
                start = row.index
                previous_pattern = pattern
            elif pattern != previous_pattern:
                assert start is not None and previous_index is not None
                segments.append((start, previous_index, previous_pattern))
                start = row.index
                previous_pattern = pattern
            previous_index = row.index
        if previous_pattern is not None and start is not None and previous_index is not None:
            segments.append((start, previous_index, previous_pattern))

        nonzero_or_changed = [
            segment for segment in segments if len(segments) > 1 or any(offset != 0 for _det_id, offset in segment[2])
        ]
        print()
        print(f"  Trigger offset segments versus det{reference}:")
        if not nonzero_or_changed:
            print("    all checked events have zero trigger offset")
            return
        self._print_limited_detail(
            nonzero_or_changed,
            "    Offset segment details:",
            lambda item: (
                f"      events {item[0]}..{item[1]}: "
                + ", ".join(f"det{det_id}:{offset:+d}" for det_id, offset in item[2])
            ),
        )

    def _print_event_details(self, rows: Sequence[AlignmentEvent]) -> None:
        if self.options.detail == "none":
            return
        selected = rows if self.options.detail == "all" else [row for row in rows if not row.one_to_one_trigger_match]
        title = "  Per-event alignment details:" if self.options.detail == "all" else "  Per-event anomaly details:"
        self._print_limited_detail(selected, title, alignment_event_text)

    def _print_limited_detail(self, rows: Sequence[object], title: str, formatter) -> None:
        if not rows:
            return
        print(title)
        limit = self.options.max_details
        visible_rows = rows if limit == 0 else rows[:limit]
        for row in visible_rows:
            print(formatter(row))
        if limit and len(rows) > limit:
            print(f"      ... {len(rows) - limit} more entries suppressed; use --alignment-max-details 0 to print all")


def alignment_event_text(row: AlignmentEvent) -> str:
    trigger_text = ", ".join(
        f"det{det_id}:trg={det.event_number},t={det.event_time},native={native_time_text(det.native_event_time)}"
        for det_id, det in sorted(row.detectors.items())
    )
    if not trigger_text:
        trigger_text = "none"
    return f"    event[{row.index}] file=0x{row.file_offset:08x}: {row.status()} | {trigger_text}"


def detector_list_text(detector_ids: Sequence[int]) -> str:
    if not detector_ids:
        return "none"
    return ",".join(f"det{det_id}" for det_id in detector_ids)


def native_time_text(value: int) -> str:
    if value == 0xFFFFFFFFFFFFFFFF:
        return "unset"
    return str(value)


def timestamp_deltas_aligned(deltas: Sequence[int], tolerance: int) -> bool:
    if len(deltas) < 2:
        return True
    return max(deltas) - min(deltas) <= tolerance


def print_stats(stats: Stats, filename: str) -> None:
    print(f"File: {filename}")
    print(f"Events parsed: {stats.events}")
    print(f"Bytes in parsed events: {stats.bytes_read}")
    print(f"Warnings: {stats.warnings}")
    if stats.events == 0:
        return

    print(f"Runs: {format_counter(stats.runs)}")
    print(f"Data versions: {format_counter(stats.versions)}")
    print(f"Event numbers: {range_text(stats.event_numbers)}")
    print(f"Event timestamps: {range_text(stats.event_times)}")
    print(f"Event size bytes: {series_text(stats.event_sizes)}")
    print(f"Payload bytes per event: {series_text(stats.payload_sizes)}")
    print(f"Detectors per event: {format_counter(stats.detectors_per_event)}")
    print(f"Detector masks: {format_mask_counter(stats.detector_masks)}")
    print(f"Trigger masks: {format_mask_counter(stats.trigger_masks)}")
    print(f"Detector boundary source: {format_counter(stats.det_size_source)}")

    if stats.det_counts:
        print()
        print("Per-detector statistics:")
    for det_id in sorted(stats.det_counts):
        sizes = stats.det_payload_sizes[det_id]
        event_times = stats.det_event_times[det_id]
        native_times = stats.det_native_event_times[det_id]
        native_deltas = stats.det_native_time_deltas[det_id]
        endianness = {
            ENDIANNESS_NAMES.get(value, f"unknown(0x{value:02x})"): count
            for value, count in stats.det_endianness[det_id].items()
        }
        endianness_text = ", ".join(f"{name}:{count}" for name, count in sorted(endianness.items()))
        print(
            f"  det{det_id}: events={stats.det_counts[det_id]} "
            f"payload_total={stats.det_payload_bytes[det_id]}B "
            f"payload/event={series_text(sizes)} "
            f"event_time={range_text(event_times)} "
            f"native_time={range_text(native_times)} "
            f"native_delta={series_text(native_deltas)} "
            f"endianness={endianness_text}"
        )


def print_event_summary(event: EventRecord, verbose: bool = False) -> None:
    det_text = ", ".join(
        f"det{det.detector_id}:{det.payload_size}B/{det.endianness_name}" for det in event.detectors
    )
    print(
        f"event[{event.index}] file=0x{event.file_offset:08x} run={event.run_number} "
        f"event={event.event_number} spill={event.spill_number} time={event.event_time} "
        f"version={event.data_version} size={event.event_size} mask=0x{event.detector_mask:02x} "
        f"detectors=[{det_text}]"
    )
    if verbose:
        print(
            f"  headerSize={event.header_size} trailerSize={event.trailer_size} "
            f"triggerMask=0x{event.trigger_mask:02x} detectorSize={event.detector_sizes}"
        )
        for det in event.detectors:
            print(
                f"  det{det.detector_id}: localEvent={det.event_number} spill={det.spill_number} "
                f"eventTime={det.event_time} nativeTime={native_time_text(det.native_event_time)} "
                f"nativeDelta={det.native_time_delta} "
                f"offset=0x{det.offset:04x} payload=0x{det.payload_offset:04x}+{det.payload_size} "
                f"sizeSource={det.size_source}"
            )
    for warning in event.warnings:
        print(f"  warning: {warning}")
    for det in event.detectors:
        for warning in det.warnings:
            print(f"  warning det{det.detector_id}: {warning}")


def format_counter(counter: Counter[int] | Counter[str]) -> str:
    if not counter:
        return "none"
    return ", ".join(f"{key}:{value}" for key, value in sorted(counter.items()))


def format_mask_counter(counter: Counter[int]) -> str:
    if not counter:
        return "none"
    return ", ".join(f"0x{key:02x}:{value}" for key, value in sorted(counter.items()))


def range_text(values: Sequence[int]) -> str:
    if not values:
        return "none"
    return f"min={min(values)} max={max(values)} first={values[0]} last={values[-1]}"


def series_text(values: Sequence[int]) -> str:
    if not values:
        return "none"
    if len(values) == 1:
        return f"min=max=mean={values[0]}"
    mean = statistics.fmean(values)
    return f"min={min(values)} max={max(values)} mean={mean:.1f}"


def dump_event(
    event: EventRecord,
    width: int,
    color_mode: str,
    absolute_offsets: bool,
    payload_bits: bool,
    payload_only: bool,
) -> None:
    if not event.record:
        raise HidraRawError("internal error: event bytes were not retained for dumping")
    use_color = should_color(color_mode)
    if not payload_only:
        print()
        print(
            f"Hex dump for event[{event.index}] at file offset 0x{event.file_offset:x}, "
            f"eventNumber={event.event_number}, size={event.event_size}B"
        )
        print_legend(event, use_color)
        dump_record_rows(
            event.record,
            event.spans,
            width,
            event.file_offset if absolute_offsets else 0,
            use_color,
        )

    if payload_bits:
        dump_detector_payloads(event, width, use_color)


def dump_record_rows(
    data: bytes,
    spans: Sequence[Span],
    width: int,
    display_base_offset: int,
    use_color: bool,
) -> None:
    for row_start in range(0, len(data), width):
        row = data[row_start : row_start + width]
        display_offset = display_base_offset + row_start
        hex_cells: List[str] = []
        ascii_cells: List[str] = []
        for idx, byte in enumerate(row):
            offset = row_start + idx
            style = style_for_offset(offset, spans) if use_color else ""
            reset = ANSI_RESET if use_color else ""
            hex_cells.append(f"{style}{byte:02x}{reset}")
            char = chr(byte) if 32 <= byte <= 126 else "."
            ascii_cells.append(f"{style}{char}{reset}")
        if len(row) < width:
            hex_cells.extend(["  "] * (width - len(row)))
        midpoint = width // 2
        left = " ".join(hex_cells[:midpoint])
        right = " ".join(hex_cells[midpoint:])
        labels = labels_for_row(row_start, row_start + len(row), spans)
        print(f"{display_offset:08x}  {left}  {right}  |{''.join(ascii_cells)}|  {labels}")


def dump_detector_payloads(event: EventRecord, width: int, use_color: bool) -> None:
    print()
    print("Detector payload bit dumps:")
    for det in event.detectors:
        payload = event.record[det.payload_offset : det.payload_offset + det.payload_size]
        payload_span = Span(0, len(payload), "detector_payload", f"det{det.detector_id} payload", det.detector_id)
        style = style_for_offset(0, [payload_span]) if use_color and payload else ""
        reset = ANSI_RESET if use_color and payload else ""
        print(
            f"det{det.detector_id} payload: record offset 0x{det.payload_offset:04x}, "
            f"{det.payload_size} bytes, endianness={det.endianness_name}, boundary={det.size_source}"
        )
        if not payload:
            continue
        for row_start in range(0, len(payload), width):
            row = payload[row_start : row_start + width]
            bit_cells = [f"{style}{byte:08b}{reset}" for byte in row]
            hex_cells = [f"{style}{byte:02x}{reset}" for byte in row]
            print(f"  +0x{row_start:04x}  bits {' '.join(bit_cells)}  hex {' '.join(hex_cells)}")


def print_legend(event: EventRecord, use_color: bool) -> None:
    parts = [
        ("event header", REGION_STYLES["event_header"]),
        ("event trailer", REGION_STYLES["event_trailer"]),
        ("detector end", REGION_STYLES["detector_end"]),
    ]
    for det in event.detectors:
        parts.append((f"det{det.detector_id} header", DETECTOR_HEADER_STYLES[det.detector_id % len(DETECTOR_HEADER_STYLES)]))
        parts.append((f"det{det.detector_id} payload", DETECTOR_PAYLOAD_STYLES[det.detector_id % len(DETECTOR_PAYLOAD_STYLES)]))
    if use_color:
        print("Legend: " + "  ".join(f"{style}{label}{ANSI_RESET}" for label, style in parts))
    else:
        print("Legend: " + "  ".join(label for label, _style in parts))


def labels_for_row(start: int, end: int, spans: Sequence[Span]) -> str:
    labels = []
    for span in spans:
        if span.start < end and start < span.end:
            labels.append(span.label)
    return "[" + ", ".join(labels) + "]"


def style_for_offset(offset: int, spans: Sequence[Span]) -> str:
    for span in spans:
        if not span.contains(offset):
            continue
        if span.kind == "detector_header":
            assert span.det_id is not None
            return DETECTOR_HEADER_STYLES[span.det_id % len(DETECTOR_HEADER_STYLES)]
        if span.kind == "detector_payload":
            assert span.det_id is not None
            return DETECTOR_PAYLOAD_STYLES[span.det_id % len(DETECTOR_PAYLOAD_STYLES)]
        return REGION_STYLES.get(span.kind, REGION_STYLES["unknown"])
    return REGION_STYLES["unknown"]


def should_color(color_mode: str) -> bool:
    if color_mode == "always":
        return True
    if color_mode == "never":
        return False
    return sys.stdout.isatty()


def positive_int(text: str) -> int:
    value = int(text, 0)
    if value <= 0:
        raise argparse.ArgumentTypeError("must be positive")
    return value


def nonnegative_int(text: str) -> int:
    value = int(text, 0)
    if value < 0:
        raise argparse.ArgumentTypeError("must be non-negative")
    return value


def detector_id(text: str) -> int:
    value = int(text, 0)
    if value < 0 or value >= MAX_DETECTORS:
        raise argparse.ArgumentTypeError(f"must be between 0 and {MAX_DETECTORS - 1}")
    return value


def detector_id_list(text: str) -> Set[int]:
    values: Set[int] = set()
    for part in text.split(","):
        part = part.strip()
        if not part:
            continue
        values.add(detector_id(part))
    if not values:
        raise argparse.ArgumentTypeError("must contain at least one detector ID")
    return values


def build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Parse HIDRA EventSerializer .raw files and optionally colored-hex-dump records."
    )
    parser.add_argument("input", help="HIDRA .raw file")
    parser.add_argument("-n", "--max-events", type=int, default=0, help="parse at most N events")
    parser.add_argument("--strict", action="store_true", help="treat format warnings as errors")
    parser.add_argument("--resync", action="store_true", help="scan forward to the next 0xB0BF event marker after garbage")
    parser.add_argument(
        "--scan-detector-end",
        action="store_true",
        help="ignore detectorSize[] for boundaries and scan for 0xDDDD markers",
    )
    parser.add_argument("--events", action="store_true", help="print one summary line per event")
    parser.add_argument("-v", "--verbose", action="store_true", help="print detailed per-event header fields")
    parser.add_argument(
        "--alignment",
        action="store_true",
        help="summarize detector-local trigger/timestamp alignment, ignoring event-header trigger/time fields",
    )
    parser.add_argument(
        "--alignment-detail",
        choices=("none", "anomalies", "all"),
        default="anomalies",
        help="control per-event alignment detail output",
    )
    parser.add_argument(
        "--alignment-max-details",
        type=nonnegative_int,
        default=200,
        help="maximum detailed alignment rows to print per section; 0 means unlimited",
    )
    parser.add_argument(
        "--alignment-time-tolerance",
        type=nonnegative_int,
        default=0,
        help="allowed spread of pairwise detector timestamp deltas before flagging drift",
    )
    parser.add_argument(
        "--alignment-reference-detector",
        type=detector_id,
        default=None,
        help="detector ID used as trigger-offset reference; defaults to the lowest expected detector",
    )
    parser.add_argument(
        "--alignment-detectors",
        type=detector_id_list,
        default=None,
        help="comma-separated detector IDs expected in every aligned event; defaults to all observed detectors",
    )
    parser.add_argument(
        "--dump",
        action="store_true",
        help="hex-dump parsed events with highlighted event and detector regions",
    )
    parser.add_argument(
        "--payload-bits",
        action="store_true",
        help="also dump each subdetector payload as byte-by-byte bit strings",
    )
    parser.add_argument(
        "--payload-only",
        action="store_true",
        help="with --payload-bits, skip the full event dump and print only payload bit dumps",
    )
    parser.add_argument(
        "--dump-event",
        type=int,
        action="append",
        default=[],
        help="only dump this zero-based event index; may be used multiple times",
    )
    parser.add_argument("--dump-width", type=positive_int, default=16, help="bytes per hex-dump row")
    parser.add_argument(
        "--absolute-offsets",
        action="store_true",
        help="show file offsets in hex dumps instead of event-relative offsets",
    )
    parser.add_argument(
        "--color",
        choices=("auto", "always", "never"),
        default="auto",
        help="control ANSI colors in the hex dump",
    )
    return parser


def main(argv: Optional[Sequence[str]] = None) -> int:
    args = build_arg_parser().parse_args(argv)
    if not os.path.exists(args.input):
        print(f"error: input file does not exist: {args.input}", file=sys.stderr)
        return 2
    if args.payload_only and not args.payload_bits:
        print("error: --payload-only requires --payload-bits", file=sys.stderr)
        return 2

    dump_indices = set(args.dump_event)
    keep_records = args.dump or args.payload_bits or bool(dump_indices)
    options = ParseOptions(
        strict=args.strict,
        use_size_table=not args.scan_detector_end,
        resync=args.resync,
        keep_records=keep_records,
    )

    stats = Stats()
    alignment = AlignmentAnalyzer(
        AlignmentOptions(
            enabled=args.alignment,
            detail=args.alignment_detail,
            max_details=args.alignment_max_details,
            time_tolerance=args.alignment_time_tolerance,
            reference_detector=args.alignment_reference_detector,
            expected_detectors=args.alignment_detectors,
        )
    )
    reader = RawReader(args.input, options)

    try:
        for event in reader.iter_events(args.max_events):
            stats.add(event)
            if args.alignment:
                alignment.add(event)
            should_print_event = args.events or args.verbose
            should_dump = (args.dump or args.payload_bits) and (not dump_indices or event.index in dump_indices)
            if should_print_event:
                print_event_summary(event, args.verbose)
            if should_dump:
                dump_event(
                    event,
                    args.dump_width,
                    args.color,
                    args.absolute_offsets,
                    args.payload_bits,
                    args.payload_only,
                )
    except HidraRawError as exc:
        print(f"parse error: {exc}", file=sys.stderr)
        return 1

    if args.events or args.verbose or args.dump:
        print()
    print_stats(stats, args.input)
    if args.alignment:
        alignment.print_report()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
