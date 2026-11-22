#!/usr/bin/env python3
"""
EUDAQ Event Parser for Hidra
Parses and displays EUDAQ event format files in human-readable format.

The EUDAQ format uses little-endian byte order for all multi-byte values.
"""

import struct
import sys
import os
from typing import Dict, List, Tuple, Optional, Any
from dataclasses import dataclass
from enum import IntFlag
import argparse


class EventFlags(IntFlag):
    """EUDAQ Event flags"""
    FLAG_BORE = 0x1
    FLAG_EORE = 0x2
    FLAG_FAKE = 0x4
    FLAG_PACK = 0x8
    FLAG_TRIG = 0x10
    FLAG_TIME = 0x20


@dataclass
class EventData:
    """Represents a parsed EUDAQ event"""
    type: int
    version: int
    flags: int
    stream_n: int
    run_n: int
    event_n: int
    trigger_n: int
    extend: int
    timestamp_begin: int
    timestamp_end: int
    description: str
    tags: Dict[str, str]
    blocks: Dict[int, bytes]
    sub_events: List['EventData']
    file_offset: int = 0


class BinaryDeserializer:
    """Deserializes binary data in EUDAQ format (little-endian)"""
    
    def __init__(self, data: bytes, offset: int = 0):
        self.data = data
        self.offset = offset
        self.start_offset = offset
    
    def read_uint8(self) -> int:
        val = self.data[self.offset]
        self.offset += 1
        return val
    
    def read_uint16(self) -> int:
        val = struct.unpack('<H', self.data[self.offset:self.offset+2])[0]
        self.offset += 2
        return val
    
    def read_uint32(self) -> int:
        val = struct.unpack('<I', self.data[self.offset:self.offset+4])[0]
        self.offset += 4
        return val
    
    def read_uint64(self) -> int:
        val = struct.unpack('<Q', self.data[self.offset:self.offset+8])[0]
        self.offset += 8
        return val
    
    def read_string(self) -> str:
        length = self.read_uint32()
        val = self.data[self.offset:self.offset+length].decode('utf-8', errors='replace')
        self.offset += length
        return val
    
    def read_bytes(self, size: int) -> bytes:
        val = self.data[self.offset:self.offset+size]
        self.offset += size
        return val
    
    def read_map_string_string(self) -> Dict[str, str]:
        """Read a map<string, string>"""
        result = {}
        size = self.read_uint32()
        for _ in range(size):
            key = self.read_string()
            value = self.read_string()
            result[key] = value
        return result
    
    def read_map_uint32_bytes(self) -> Dict[int, bytes]:
        """Read a map<uint32_t, vector<uint8_t>>"""
        result = {}
        size = self.read_uint32()
        for _ in range(size):
            key = self.read_uint32()
            data = self._read_vector_uint8()
            result[key] = data
        return result
    
    def _read_vector_uint8(self) -> bytes:
        """Read a vector<uint8_t>"""
        size = self.read_uint32()
        return self.read_bytes(size)
    
    def has_data(self, min_bytes: int = 1) -> bool:
        return self.offset + min_bytes <= len(self.data)
    
    def peek_uint32(self) -> Optional[int]:
        if self.has_data(4):
            return struct.unpack('<I', self.data[self.offset:self.offset+4])[0]
        return None
    
    def get_offset(self) -> int:
        return self.offset


class EUDAQEventParser:
    """Parses EUDAQ event files"""
    
    def __init__(self, filename: str):
        self.filename = filename
        self.events: List[EventData] = []
        self.file_size = os.path.getsize(filename)
    
    def parse_file(self) -> List[EventData]:
        """Parse all events from file"""
        self.events = []
        with open(self.filename, 'rb') as f:
            data = f.read()
        
        deserializer = BinaryDeserializer(data)
        event_index = 0
        while deserializer.has_data(4):
            file_offset = deserializer.get_offset()
            try:
                event = self._parse_event(deserializer)
                event.file_offset = file_offset
                self.events.append(event)
                event_index += 1
            except Exception as e:
                # End of file or corrupted data
                break
        
        return self.events
    
    def _parse_event(self, deser: BinaryDeserializer) -> EventData:
        """Parse a single event"""
        event = EventData(
            type=deser.read_uint32(),
            version=deser.read_uint32(),
            flags=deser.read_uint32(),
            stream_n=deser.read_uint32(),
            run_n=deser.read_uint32(),
            event_n=deser.read_uint32(),
            trigger_n=deser.read_uint32(),
            extend=deser.read_uint32(),
            timestamp_begin=deser.read_uint64(),
            timestamp_end=deser.read_uint64(),
            description=deser.read_string(),
            tags=deser.read_map_string_string(),
            blocks=deser.read_map_uint32_bytes(),
            sub_events=[]
        )
        
        # Read sub-events
        num_subevents = deser.read_uint32()
        for _ in range(num_subevents):
            sub_event = self._parse_event(deser)
            event.sub_events.append(sub_event)
        
        return event
    
    def get_events(self) -> List[EventData]:
        """Get parsed events"""
        return self.events
    
    def get_event_count(self) -> int:
        """Get total number of events"""
        return len(self.events)
    
    def get_subevent_count(self, event: Optional[EventData] = None) -> int:
        """Get total number of events including sub-events"""
        if event is None:
            total = 0
            for ev in self.events:
                total += 1 + self.get_subevent_count(ev)
            return total
        else:
            total = 0
            for sub_ev in event.sub_events:
                total += 1 + self.get_subevent_count(sub_ev)
            return total


def hash_to_string(hash_val: int) -> str:
    """Convert EUDAQ hash to approximate string (for known values)"""
    known_hashes = {
        0x8026656d: "RawEvent",
        0xde7d06e0: "XDCEvent",
        0x80258cd6: "XDCEvent",  # Another variant
        0xb310c1eb: "FERSEvent",
    }
    return known_hashes.get(hash_val, f"0x{hash_val:08x}")


def format_timestamp_ns(timestamp_ns: int) -> Tuple[str, float]:
    """Format nanosecond timestamp"""
    if timestamp_ns == 0:
        return "Not set", 0.0
    
    seconds = timestamp_ns // 1_000_000_000
    remaining_ns = timestamp_ns % 1_000_000_000
    
    from datetime import datetime, timezone
    try:
        dt = datetime.fromtimestamp(seconds, tz=timezone.utc)
        time_str = dt.strftime('%Y-%m-%d %H:%M:%S')
        ms = remaining_ns / 1_000_000
        return f"{time_str}.{ms:06.0f}ms", seconds + remaining_ns / 1_000_000_000
    except (ValueError, OSError):
        return f"{seconds}s + {remaining_ns}ns", seconds + remaining_ns / 1_000_000_000


def get_flag_names(flags: int) -> List[str]:
    """Get human-readable flag names"""
    names = []
    if flags & EventFlags.FLAG_BORE:
        names.append("BORE")
    if flags & EventFlags.FLAG_EORE:
        names.append("EORE")
    if flags & EventFlags.FLAG_FAKE:
        names.append("FAKE")
    if flags & EventFlags.FLAG_PACK:
        names.append("PACK")
    if flags & EventFlags.FLAG_TRIG:
        names.append("TRIG")
    if flags & EventFlags.FLAG_TIME:
        names.append("TIME")
    return names if names else ["NONE"]


def format_hex_block(data: bytes, group_size: int = 4, bytes_per_line: int = 16) -> str:
    """Format binary data as hex (similar to od -t x4)"""
    if not data:
        return "(empty)"
    
    lines = []
    for i in range(0, len(data), bytes_per_line):
        chunk = data[i:i+bytes_per_line]
        # Format with group_size bytes per group
        hex_parts = []
        for j in range(0, len(chunk), group_size):
            group = chunk[j:j+group_size]
            # Interpret each group as a little-endian integer (like od -t x4)
            if len(group) == group_size:
                # For 4-byte groups, read as little-endian uint32
                if group_size == 4:
                    val = struct.unpack('<I', group)[0]
                    hex_str = f'{val:08x}'
                else:
                    hex_str = ''.join(f'{b:02x}' for b in group)
            else:
                # For incomplete groups at end, show bytes individually
                hex_str = ''.join(f'{b:02x}' for b in group)
            hex_parts.append(hex_str)
        
        offset_str = f"0x{i:08x}"
        hex_str = ' '.join(hex_parts)
        
        # Append ASCII representation
        ascii_str = ''.join(chr(b) if 32 <= b < 127 else '.' for b in chunk)
        line_str = f"{offset_str}  {hex_str:<{bytes_per_line * 2 + bytes_per_line // group_size}}  {ascii_str}"
        lines.append(line_str)
    
    return '\n'.join(lines)


def print_event_header(event: EventData, event_number: int = 0, indent: int = 0) -> None:
    """Print event header in human-readable format"""
    ind = "  " * indent
    
    print(f"\n{ind}{'=' * 100}")
    print(f"{ind}EVENT #{event_number}  |  File Offset: 0x{event.file_offset:08x}")
    print(f"{ind}{'=' * 100}")
    
    # Main event info
    type_str = hash_to_string(event.type)
    flags_str = ", ".join(get_flag_names(event.flags))
    
    print(f"{ind}Type: {type_str:<25} Version: {event.version:<8} Flags: {flags_str}")
    print(f"{ind}Description: {event.description}")
    
    print(f"\n{ind}RUN # {event.run_n:<18} | STREAM # {event.stream_n:<12} | DEVICE # {event.extend}")
    print(f"{ind}EVENT # {event.event_n:<16} | TRIGGER # {event.trigger_n:<11} | EXTEND # {event.extend}")
    
    # Timestamps
    if event.flags & EventFlags.FLAG_TIME:
        ts_start_str, ts_start_float = format_timestamp_ns(event.timestamp_begin)
        ts_end_str, ts_end_float = format_timestamp_ns(event.timestamp_end)
        duration_us = (event.timestamp_end - event.timestamp_begin) / 1000
        
        print(f"\n{ind}Timestamp Start: {ts_start_str} ({event.timestamp_begin:>20} ns)")
        print(f"{ind}Timestamp End:   {ts_end_str} ({event.timestamp_end:>20} ns)")
        print(f"{ind}Duration:        {duration_us:>30.3f} µs")
    else:
        print(f"\n{ind}Timestamp Begin: {event.timestamp_begin:>20} ns")
        print(f"{ind}Timestamp End:   {event.timestamp_end:>20} ns")
    
    # Tags
    if event.tags:
        print(f"\n{ind}TAGS ({len(event.tags)}):")
        for key, value in sorted(event.tags.items()):
            print(f"{ind}  {key:<32} = {value}")
    
    # Blocks info
    if event.blocks:
        print(f"\n{ind}BLOCKS ({len(event.blocks)}):")
        for block_id, block_data in sorted(event.blocks.items()):
            print(f"{ind}  Block #{block_id}: {len(block_data):>12} bytes")


def print_event_blocks(event: EventData, indent: int = 0) -> None:
    """Print event blocks in hex format"""
    ind = "  " * indent
    
    if not event.blocks:
        return
    
    for block_id, block_data in sorted(event.blocks.items()):
        print(f"\n{ind}{'-' * 100}")
        print(f"{ind}BLOCK #{block_id}  |  {len(block_data)} bytes")
        print(f"{ind}{'-' * 100}")
        
        hex_output = format_hex_block(block_data, group_size=4, bytes_per_line=16)
        for line in hex_output.split('\n'):
            print(f"{ind}{line}")


def print_event_full(event: EventData, event_number: int = 0, show_blocks: bool = True, indent: int = 0) -> None:
    """Print complete event information"""
    print_event_header(event, event_number, indent)
    
    if show_blocks:
        print_event_blocks(event, indent)
    
    # Print sub-events
    if event.sub_events:
        ind = "  " * indent
        print(f"\n{ind}{'=' * 100}")
        print(f"{ind}SUB-EVENTS ({len(event.sub_events)})")
        print(f"{ind}{'=' * 100}")
        for i, sub_event in enumerate(event.sub_events):
            print_event_full(sub_event, i, show_blocks, indent + 1)


def get_file_statistics(parser: EUDAQEventParser) -> Dict[str, Any]:
    """Calculate statistics about the event file"""
    events = parser.get_events()
    
    if not events:
        return {}
    
    stats = {
        'total_events': len(events),
        'total_subevent_count': sum(1 + parser.get_subevent_count(ev) for ev in events),
        'run_numbers': set(),
        'event_number_range': (float('inf'), 0),
        'xdc_events': 0,
        'fers_events': 0,
        'xdc_and_fers_events': 0,
        'timestamps': [],
    }
    
    def process_event(ev: EventData):
        stats['run_numbers'].add(ev.run_n)
        stats['event_number_range'] = (
            min(stats['event_number_range'][0], ev.event_n),
            max(stats['event_number_range'][1], ev.event_n)
        )
        
        # Count XDC and FERS events
        has_xdc = any(key in ev.tags for key in ['dataWords', 'detectorDataSize', 'spillNumber'])
        has_fers = 'FERS' in ev.description or 'fers' in ev.description.lower()
        
        if has_xdc:
            stats['xdc_events'] += 1
        if has_fers:
            stats['fers_events'] += 1
        if has_xdc and has_fers:
            stats['xdc_and_fers_events'] += 1
        
        if ev.flags & EventFlags.FLAG_TIME and ev.timestamp_begin != 0:
            stats['timestamps'].append(ev.timestamp_begin)
        
        for sub_ev in ev.sub_events:
            process_event(sub_ev)
    
    for event in events:
        process_event(event)
    
    # Calculate time-based statistics
    if stats['timestamps']:
        stats['timestamps'].sort()
        stats['first_timestamp'] = stats['timestamps'][0]
        stats['last_timestamp'] = stats['timestamps'][-1]
        stats['total_duration_ns'] = stats['last_timestamp'] - stats['first_timestamp']
        
        if len(stats['timestamps']) > 1:
            time_diffs = [stats['timestamps'][i+1] - stats['timestamps'][i] 
                         for i in range(len(stats['timestamps']) - 1)]
            stats['avg_time_between_events_ns'] = sum(time_diffs) / len(time_diffs)
    
    return stats


def print_file_statistics(parser: EUDAQEventParser) -> None:
    """Print file statistics"""
    stats = get_file_statistics(parser)
    
    if not stats:
        print("No events found in file.")
        return
    
    print(f"\n{'=' * 100}")
    print(f"FILE STATISTICS")
    print(f"{'=' * 100}")
    print(f"File: {parser.filename}")
    print(f"File Size: {parser.file_size / (1024*1024):.2f} MB")
    print(f"Total Events: {stats['total_events']}")
    print(f"Total Events (including sub-events): {stats['total_subevent_count']}")
    
    print(f"\nRun Numbers: {', '.join(str(r) for r in sorted(stats['run_numbers']))}")
    if stats['event_number_range'][0] != float('inf'):
        print(f"Event Number Range: {int(stats['event_number_range'][0])} - {int(stats['event_number_range'][1])}")
    
    print(f"\nXDC Events: {stats['xdc_events']}")
    print(f"FERS Events: {stats['fers_events']}")
    print(f"Events with both XDC and FERS: {stats['xdc_and_fers_events']}")
    
    if stats.get('first_timestamp'):
        first_ts_str, _ = format_timestamp_ns(stats['first_timestamp'])
        last_ts_str, _ = format_timestamp_ns(stats['last_timestamp'])
        duration_s = stats['total_duration_ns'] / 1_000_000_000
        
        print(f"\nFirst Timestamp: {first_ts_str}")
        print(f"Last Timestamp:  {last_ts_str}")
        print(f"Total Duration:  {duration_s:.3f}s ({stats['total_duration_ns'] / 1_000_000:.0f}ms)")
        
        if stats.get('avg_time_between_events_ns'):
            avg_time_ms = stats['avg_time_between_events_ns'] / 1_000_000
            avg_time_us = stats['avg_time_between_events_ns'] / 1_000
            print(f"Avg Time Between Events: {avg_time_ms:.3f}ms ({avg_time_us:.0f}µs)")


class InteractiveViewer:
    """Interactive event viewer (like 'less' command)"""
    
    def __init__(self, parser: EUDAQEventParser, start_event: int = 0, show_blocks: bool = True):
        self.parser = parser
        self.events = parser.get_events()
        self.current_event = min(start_event, len(self.events) - 1) if self.events else 0
        self.show_blocks = show_blocks
    
    def run(self):
        """Run interactive viewer"""
        if not self.events:
            print("No events to display.")
            return
        
        print(f"\nInteractive Event Viewer - {len(self.events)} events total")
        print("Commands: [SPACE] next event, [B] previous, [G] go to event number, [S] statistics, [H] help, [Q] quit")
        print()
        
        self.display_current_event()
        
        while True:
            try:
                cmd = input("\n> ").strip().lower()
                
                if cmd == '' or cmd == ' ':  # Space
                    self.current_event += 1
                    if self.current_event >= len(self.events):
                        print("(End of file reached)")
                        self.current_event = len(self.events) - 1
                    else:
                        self.display_current_event()
                
                elif cmd == 'b':  # Back
                    self.current_event -= 1
                    if self.current_event < 0:
                        print("(Beginning of file reached)")
                        self.current_event = 0
                    else:
                        self.display_current_event()
                
                elif cmd == 'g':  # Go to event
                    try:
                        event_num = int(input(f"Go to event (0-{len(self.events)-1}): "))
                        if 0 <= event_num < len(self.events):
                            self.current_event = event_num
                            self.display_current_event()
                        else:
                            print(f"Invalid. Valid range: 0-{len(self.events)-1}")
                    except ValueError:
                        print("Invalid input.")
                
                elif cmd == 's':  # Statistics
                    print_file_statistics(self.parser)
                
                elif cmd == 'h':  # Help
                    self.print_help()
                
                elif cmd == 'q':  # Quit
                    break
                
                else:
                    print("Unknown command. Type 'h' for help.")
            
            except KeyboardInterrupt:
                print("\nExiting...")
                break
            except EOFError:
                print("\nExiting...")
                break
    
    def print_help(self):
        """Print help message"""
        print(f"""
Viewer Commands:
  [SPACE]  - Show next event
  [B]      - Show previous event
  [G]      - Go to specific event number
  [S]      - Show file statistics
  [H]      - Show this help
  [Q]      - Quit the viewer
        """)
    
    def display_current_event(self):
        """Display current event"""
        print(f"\n{'#' * 100}")
        print_event_full(self.events[self.current_event], self.current_event, self.show_blocks)
        print(f"{'#' * 100}")
        print(f"Event {self.current_event + 1} of {len(self.events)}")


def main():
    parser = argparse.ArgumentParser(
        description="Parse and display EUDAQ event format files from Hidra",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s run024_260422162755.raw              # Interactive viewer
  %(prog)s run024_260422162755.raw -e 5         # Start at event 5
  %(prog)s run024_260422162755.raw -s           # Show file statistics only
  %(prog)s run024_260422162755.raw -H           # Hide raw data blocks (headers only)
  %(prog)s run024_260422162755.raw -e 10 -H     # Jump to event 10, show headers only
  %(prog)s run024_260422162755.raw -n 3         # Show first 3 events in batch mode
        """
    )
    
    parser.add_argument('filename', help='Raw event file to parse')
    parser.add_argument('-e', '--event', type=int, default=0, 
                       help='Start at event number (default: 0)')
    parser.add_argument('-s', '--stats-only', action='store_true',
                       help='Print file statistics only, do not show events')
    parser.add_argument('-H', '--hide-blocks', action='store_true',
                       help='Hide raw data blocks (show headers only)')
    parser.add_argument('-n', '--num-events', type=int,
                       help='Display only N events and exit (batch mode)')
    parser.add_argument('--no-interactive', action='store_true',
                       help='Batch mode: display events sequentially and exit')
    
    args = parser.parse_args()
    
    # Validate file
    if not os.path.exists(args.filename):
        print(f"Error: File '{args.filename}' not found.", file=sys.stderr)
        sys.exit(1)
    
    # Parse file
    print(f"Parsing {args.filename}...")
    event_parser = EUDAQEventParser(args.filename)
    events = event_parser.parse_file()
    print(f"✓ Found {len(events)} events.\n")
    
    # Stats-only mode
    if args.stats_only:
        print_file_statistics(event_parser)
        return
    
    # Batch mode (non-interactive)
    if args.no_interactive or args.num_events:
        num_to_show = args.num_events if args.num_events else len(events)
        start_idx = min(args.event, len(events) - 1) if events else 0
        
        for i in range(start_idx, min(start_idx + num_to_show, len(events))):
            print_event_full(events[i], i, not args.hide_blocks)
        return
    
    # Interactive mode
    viewer = InteractiveViewer(event_parser, args.event, not args.hide_blocks)
    viewer.run()


if __name__ == '__main__':
    main()
