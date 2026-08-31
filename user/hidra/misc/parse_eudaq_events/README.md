# EUDAQ Event File Parser

A Python utility to parse and display EUDAQ event format files from the Hidra DAQ system.

## Features

### Display Modes
- **Interactive Viewer**: Browse events one at a time with navigation controls (like `less`)
- **Batch Mode**: Display multiple events sequentially and exit
- **Statistics Mode**: Get a comprehensive summary of the file without viewing individual events
- **Header-Only Mode**: Show event metadata without raw data blocks for faster browsing

### Information Display

#### Human-Readable Headers
- Event number and file offset
- Event type and version
- Flags (BORE, EORE, FAKE, PACK, TRIG, TIME)
- Run, stream, event, trigger, and device numbers
- Timestamps with human-readable date/time when available
- Event duration (for timestamped events)
- Custom tags (metadata added by producers)
- Block information (size and ID of data blocks)

#### Raw Data Display
- Hexadecimal dump similar to `od -t x4`
- 32-bit word grouping for clarity
- ASCII representation for readable bytes
- Offset markers for navigation
- One block per event
- Care should be taken in validating how the specific producer set data endianess for its buffer. This could lead to swapping when reading. Specifc ordering should be set for each producer by updating this script when the producer is stable

#### File Statistics
- Total event count (including sub-events)
- Run numbers used
- Event number range
- Detector breakdown (XDC, FERS, or combined)
- Timestamp range and duration
- Average time between events
- File size information

## Installation

```bash
# Ensure Python 3.7+ is available
which python3

# Make script executable
chmod +x user/hidra/misc/parse_eudaq_events.py

# Optional: Add to PATH for system-wide access
sudo ln -s /full/path/to/parse_eudaq_events.py /usr/local/bin/parse_eudaq_events
```

## Usage

### Interactive Viewer (Default)
```bash
# View first event, navigate with commands
./parse_eudaq_events.py run024_260422162755.raw

# Start at event 100
./parse_eudaq_events.py run024_260422162755.raw -e 100

# Hide raw data blocks (show headers only) for faster scrolling
./parse_eudaq_events.py run024_260422162755.raw -H
```

### Batch Mode
```bash
# Display first 5 events and exit
./parse_eudaq_events.py run024_260422162755.raw -n 5

# Display events 50-54
./parse_eudaq_events.py run024_260422162755.raw -e 50 -n 5

# Display events without raw blocks
./parse_eudaq_events.py run024_260422162755.raw -n 10 -H

# Force batch mode (no interactive prompts)
./parse_eudaq_events.py run024_260422162755.raw --no-interactive
```

### Statistics Mode
```bash
# Get file summary without viewing events
./parse_eudaq_events.py run024_260422162755.raw -s

# Useful for quickly checking file properties
./parse_eudaq_events.py run024_260422162755.raw -s | head -20
```

### Pipe to Files
```bash
# Save statistics to file
./parse_eudaq_events.py run024_260422162755.raw -s > file_info.txt

# Save first 10 events with hex dumps
./parse_eudaq_events.py run024_260422162755.raw -n 10 > events.txt

# View hex data only, filter other output
./parse_eudaq_events.py run024_260422162755.raw -n 1 | grep -A 30 "BLOCK"
```

## Interactive Viewer Commands

When running in interactive mode, use these commands:

| Command | Action |
|---------|--------|
| `SPACE` | Show next event |
| `B` | Show previous event |
| `G` | Go to specific event number |
| `S` | Show file statistics |
| `H` | Show help |
| `Q` | Quit viewer |

## Output Format Examples

### Event Header Example
```
====================================================================================================
EVENT #42  |  File Offset: 0x000a5d40
====================================================================================================
Type: XDCEvent                 Version: 2        Flags: TRIG, TIME
Description: XDCEvent

RUN # 24                 | STREAM # 1234567890   | DEVICE # 0
EVENT # 42               | TRIGGER # 42          | EXTEND # 0

Timestamp Start: 2025-10-04 04:58:00.123456ms (1759553880123456000 ns)
Timestamp End:   2025-10-04 04:58:00.123456ms (1759553880123456100 ns)
Duration:                                 0.100 µs

TAGS (7):
  dataWords                        = 216
  detectorDataSize                 = 231
  isPedFromScaler                  = 0
  isPedMask                        = 0
  sanityFlag                       = 2
  spillNumber                      = 42
  triggerMask                      = 1

BLOCKS (1):
  Block #0:          864 bytes
```

### Hex Dump Example
```
----------------------------------------------------------------------------------------------------
BLOCK #0  |  864 bytes
----------------------------------------------------------------------------------------------------
0x00000000  0020a0fa d34000f8 464010f8 a94001f8   . ...@..F@...@..
0x00000010  cd4011f8 bd4002f8 d44012f8 a24003f8   .@...@...@...@..
0x00000020  754013f8 c94004f8 ee4014f8 a84005f8   u@...@...@...@..
...
```

### Statistics Example
```
====================================================================================================
FILE STATISTICS
====================================================================================================
File: run024_260422162755.raw
File Size: 4.59 MB
Total Events: 4457
Total Events (including sub-events): 4457

Run Numbers: 24
Event Number Range: 0 - 4456

XDC Events: 4457
FERS Events: 0
Events with both XDC and FERS: 0

First Timestamp: 2025-10-04 04:57:40.000660ms
Last Timestamp:  2025-10-04 04:58:23.000792ms
Total Duration:  43.132s (43132ms)
Avg Time Between Events: 9.679ms (9679µs)
```

## Implementation Details

### Binary Format
- **Byte Order**: Little-endian (important for multi-byte values)
- **Encoding**: UTF-8 for strings
- **Integer Types**: uint8, uint16, uint32, uint64

### Event Structure Hierarchy
```
Event
├── Header (type, version, flags, run_n, event_n, etc.)
├── Metadata
│   ├── Description (string)
│   ├── Tags (map of string key-value pairs)
│   └── Blocks (map of uint32 ID to byte data)
└── Sub-Events (recursive structure)
```

### File Navigation
- Events are stored sequentially in a single binary file
- No index is pre-computed; file is parsed from beginning
- Large files may take a few seconds to parse completely
- Once parsed, navigation is instant

## Limitations & Notes

1. **Large Files**: Parsing may be slow for multi-GB files (optimization possible with indexing)
2. **Corrupted Data**: Parser will stop at first error; use batch mode with lower counts for debugging
3. **Memory Usage**: Entire file is loaded into memory; not suitable for very large files (>1GB on small systems)
4. **Event Ordering**: Events must be in valid EUDAQ format; tool doesn't validate or repair corrupted files
5. **Sub-Events**: Displayed hierarchically with indentation; may create long output

## Troubleshooting

### "No events found in file"
- File may be empty or corrupted
- Check file exists: `ls -lh filename`
- Try with `-n 1` to see first event

### Parser crashes or hangs
- File may be partially written or corrupted
- Try batch mode: `./parse_eudaq_events.py file.raw -n 1`
- Check file size: `wc -c filename`

### Timestamps show "Not set"
- Events may not have TIME flag set
- Check flags in event header
- Some events (BORE, EORE) may not have timestamps

### Hex dump looks wrong
- Verify file is not corrupted
- Check file offsets match expected data sizes
- Look at statistics for any anomalies

## Examples with Real Data

```bash
# Quickly scan a run to see what you have
./parse_eudaq_events.py run024_260422162755.raw -s

# Extract headers for analysis
./parse_eudaq_events.py run024_260422162755.raw -n 100 -H > headers_only.txt

# Look at specific event in detail
./parse_eudaq_events.py run024_260422162755.raw -e 1000 -n 1

# Compare raw data from different detectors
./parse_eudaq_events.py run024_260422162755.raw -e 0 -n 1 > event0.txt
./parse_eudaq_events.py run025_260422164432.raw -e 0 -n 1 > event0_run25.txt
diff <(grep BLOCK event0.txt) <(grep BLOCK event0_run25.txt)
```

## Related Files

- Input: EUDAQ raw files in `user/hidra/run/out_data/`
- Parser location: `user/hidra/misc/parse_eudaq_events.py`
- Event format: `main/lib/core/include/eudaq/Event.hh`
- Serialization: `main/lib/core/src/FileSerializer.cc`

## Version History

- v1.0 (2025-04): Initial implementation
  - Full EUDAQ event format parsing
  - Interactive and batch modes
  - Statistics generation
  - Hex dump display

## Author Notes

This parser was created to facilitate analysis of EUDAQ event data from the Hidra detector system. It implements the full EUDAQ serialization protocol (little-endian, with proper map and vector handling) and provides multiple viewing modes for different analysis needs.

Key design decisions:
- Python for rapid prototyping and cross-platform compatibility
- Entire file loaded in memory for fast navigation (appropriate for typical file sizes)
- Interactive viewer inspired by `less` for familiar UX
- Statistics calculated on-demand to minimize startup time
- Modular code structure allowing easy extension for custom analysis
