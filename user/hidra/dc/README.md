# HidraDataCollector

This folder contains the HiDRa EUDAQ `DataCollector` implementation and the
pluggable asynchronous writers used by the collector.

The collector receives merged EUDAQ events from the producers, builds the final
HiDRa merged event, and then forwards it to one or both of the available output
paths:

- binary writer: the existing HIDRA merged binary format handled by
  `EventSerializer`
- ROOT writer: an optional ROOT ntuple output for quick analysis

## Components

- `HidraDataCollector.cc` - merges producer subevents and routes completed
  events to the configured writers via thread-safe queues
- `HidraMergedBinaryWriter.*` - asynchronous binary writer for the HIDRA event
  format with dedicated worker thread
- `HidraRootEventWriter.*` - asynchronous ROOT ntuple writer with dedicated
  worker thread
- `HidraRootPayloadDecoders.*` - detector-specific payload decoders used by the
  ROOT writer
- `HidraXdcDecoder.*` / `HidraFersDecoder.*` - byte decoders turning a detector
  payload into a `HidraXdcEvent` / `HidraFersEvent`
- `HidraMetaDecoder.*` - reads per-event metadata (trigger mask, spill,
  timestamps, …) from the EUDAQ event into `HidraEventMeta`. `HidraEvent`
  (`HidraEventMeta.hh`, `HidraXdcEvent.hh`, `HidraFersEvent.hh`) is the decoded
  bundle consumed by the monitor; see `../monitor/README.md`.

## Architecture

The collector implements a **producer-consumer pattern** with dedicated I/O threads:

1. **Producer thread** (main DataCollector):
   - Receives events from detector producers via EUDAQ
   - Merges sub-events from multiple sources into unified events
   - Enqueues complete merged events to writer queues
   - Returns immediately (non-blocking)

2. **Consumer threads** (one per writer):
   - Each writer (`HidraMergedBinaryWriter`, `HidraRootEventWriter`) owns a
     dedicated worker thread
   - Dequeues events from the writer's queue
   - Performs all I/O operations (file open/write/close, tree operations)
   - Handles flushing and cleanup on shutdown

**Key design principle**: All I/O for a writer happens exclusively in that
writer's worker thread. The main DataCollector thread never touches file
handles or ROOT objects directly. This eliminates:

- Race conditions on file/ROOT object access
- Complexity of cross-thread synchronization
- Deadlocks from mutex contention
- ROOT thread-safety violations (ROOT requires single-threaded access)

The two writers are completely independent, so the system naturally supports
binary-only, ROOT-only, or dual output modes without additional locking.

## Build options

The dc module is built from `user/hidra/dc/CMakeLists.txt` and supports an
optional ROOT writer build.

- `USER_HIDRA_DC_ROOT_OUTPUT=ON` enables compilation of the ROOT writer
- if ROOT is not available, the ROOT writer is disabled automatically

The module still builds the binary writer path regardless of ROOT support.

## Run-time configuration

The collector reads these configuration keys from the `DataCollector` section
of the `.conf` file:

- `WRITE_BINARY_OUTPUT` - enable or disable the standard binary output
- `WRITE_ROOT_OUTPUT` - enable or disable the ROOT ntuple output
- `EUDAQ_FW_PATTERN` - file-name template (`$R` run number, `$D` timestamp, `$X`
  extension; an optional width prefix pads, e.g. `$3R`, `$12D`). The same
  pattern is used for both the `.raw` and `.root` streams.
- `BINARY_OUTPUT_DIR` - optional directory for the `.raw` file. When set, the
  name built from `EUDAQ_FW_PATTERN` is written inside this directory (created
  if missing). When empty (default), the pattern path is used as-is.
- `ROOT_OUTPUT_DIR` - optional directory for the `.root` file, same semantics.
  Set `BINARY_OUTPUT_DIR` and `ROOT_OUTPUT_DIR` to different paths to send the
  two streams to separate directories; with both empty the files stay together
  in the pattern's directory as before.
- `WRITER_FLUSH_INTERVAL_MS` - polling / flush interval used by the writers
- `WRITER_FLUSH_EVERY_EVENTS` - flush after this many queued events
- `XDC_CONFIG_JSON` - optional path to a JSON file used by the XDC payload decoders

Example:

```ini
[DataCollector.HidraDataCollector]
EUDAQ_FW = native
# Pattern is now just the base name; the directories below place each stream.
EUDAQ_FW_PATTERN = run$3R_$12D$X
BINARY_OUTPUT_DIR = out_data/raw
ROOT_OUTPUT_DIR = out_data/root
WRITE_BINARY_OUTPUT = 1
WRITE_ROOT_OUTPUT = 0
WRITER_FLUSH_INTERVAL_MS = 50
WRITER_FLUSH_EVERY_EVENTS = 32
XDC_CONFIG_JSON = user/hidra/run/config/root_decoder_config.json
```

## Output modes

- Binary only: set `WRITE_BINARY_OUTPUT = 1` and `WRITE_ROOT_OUTPUT = 0`
- ROOT only: set `WRITE_BINARY_OUTPUT = 0` and `WRITE_ROOT_OUTPUT = 1`
- Dual output: set both to `1`

The two writers are independent, so the collector can write to either output or
to both in parallel.

## Binary format

The binary writer uses the existing HIDRA merged format documented in
[../DataFormatv3.md](../DataFormatv3.md).

That document describes the event header, detector block layout, detector mask,
and trailer written by `EventSerializer`.

## ROOT layout

The ROOT writer keeps the same general structure used in the 2025 TB
conversion flow:

- event-level metadata branches for run/event/timing information
- detector quantities stored as branch vectors (`q_det`, `q_name`, `q_value`,
  `q_unit`)
- detector-specific parsing is separated so XDC and FERS handling can evolve
  independently

The ROOT writer is intended for quick inspection and analysis rather than for
preserving the raw binary payload as-is.

## ROOT decoder JSON configuration

The ROOT payload decoders can optionally read a JSON file before the ROOT writer
starts. This is intended for simple hardware lookup tables that influence the
decoder logic without hard-coding every module choice in C++.

Enable it from the `DataCollector` section:

```ini
[DataCollector.HidraDataCollector]
WRITE_ROOT_OUTPUT = 1
XDC_CONFIG_JSON = /path/to/root_decoder_config.json
```

The current parser keeps the schema intentionally open. A minimal XDC module
map can look like this:

```json
{
  "XDCModules": {
    "1": "V792",
    "5": "V792N"
  }
}
```

Decoder code can then query values with:

```cpp
const std::string module_type = GetRootDecoderConfigValue("XDCModules", detector.det_id);
```

For example, detector/module `"1"` returns `"V792"` for the JSON above. Missing
sections or keys return the supplied fallback, or an empty string if no fallback
is provided.

## Notes on payload decoding

The payload decoders are intentionally kept modular:

- XDC payload handling lives in the XDC decoder path
- FERS payload handling lives in the FERS decoder path
- the generic decoder provides fallback quantities for unknown payloads

This structure makes it easy to refine the decoding later without changing the
collector or writer orchestration logic.

### Modifying payload parsing for ROOT output

The decoders are implemented in `HidraRootPayloadDecoders.hh/cc`. To
check or modify how payloads are parsed:

1. **Locate the relevant decoder**: 
   - For XDC detectors: `HidraXdcPayloadDecoder`
   - For FERS detectors: `HidraFersPayloadDecoder`
   - For unknown detectors: `HidraGenericPayloadDecoder` (fallback)

2. **Modify the `Decode()` method**:
   Each decoder has a `Decode(..., std::vector<RootQuantity>&, RootBranchValues&)`
   method that extracts quantities from the subevent payload. The
   `RootQuantity` list is kept for the generic `q_*` branches, while
   `RootBranchValues` is used for detector-specific ROOT branches.
   If decoding depends on the hardware module, read the JSON value with
   `GetRootDecoderConfigValue(...)` and branch on that string.

3. **Add new quantities**:
   - Extract the value in the `Decode()` method from the subevent payload
   - Push scalar summaries into the `RootQuantity` list when you want them in
     `q_det/q_name/q_value/q_unit`
   - Push vectors into `branches["branch_name"]` when you want a dedicated
     `std::vector<double>` ROOT branch
   - The `HidraRootEventWriter::WriteEvent()` method will automatically create
     and fill custom ROOT tree branches as they appear

4. **ROOT tree branches**:
   The ROOT writer stores generic quantities as vectors in branches:
   - `q_det` - detector ID for each quantity
   - `q_name` - quantity name (e.g., "energy", "amplitude")
   - `q_value` - numeric value
   - `q_unit` - unit string
   It also stores detector-specific branches produced by decoders, for example:
   - `xdc_words`
   - `fers2_energy_hg`
   - `fers2_tstamp_us`

Example: to add a new XDC quantity, modify `HidraXdcPayloadDecoder::Decode()`
to extract and store the value in either the generic quantities or a named
`RootBranchValues` vector.

The worker thread that owns the ROOT I/O will handle writing all quantities to
the ntuple automatically.
