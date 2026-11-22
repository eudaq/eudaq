HiDRa FERS2 module
===================

Purpose
-------
This folder contains a modular reimplementation of the CAEN FERS integration
used by the HiDRa project. It provides:

- `FERSConfiguration` — parse and load CAEN FERSlib-style configuration files
- `FERSBoard` — a thin wrapper for a single FERS board lifecycle
- `FERSBoardManager` — multi-board orchestration and polling
- `HidraFERS2Producer` — EuDAQ producer integrating the manager into the run loop

Usage
-----
The module is built as a shared eudaq module and is enabled from the
`user/hidra/CMakeLists.txt` with the build option `USER_HIDRA_FERS2_BUILD`.


Notes
-----
- The code expects a system-installed `libcaenferslib.so` (1.3.0) available in
  standard library search paths (/usr/local/lib, /usr/lib).
- The implementation intentionally keeps all logic within `fers2/` to make
  switching between legacy and new producers straightforward.

Data format
-----------

The producer does not reinterpret the board data into a custom binary format.
Instead, each board payload is copied as the raw CAEN event struct returned by
FERSlib and stored as one EuDAQ block. That makes the payload easy to inspect,
but it also means the layout follows the vendor struct exactly and should be
treated as an ABI-level format, not as a portable serialization.

Top-level EuDAQ event
---------------------

For each trigger, the FERS2 producer emits one EuDAQ event with these useful
fields:

- `Description` = `FERSProducer`
- `Producer` tag = `HidraFERS2Producer`
- `TriggerN` = hardware trigger id for the aligned event
- `EventN` = same value as the trigger id
- `Timestamp` = first board timestamp seen for the trigger, when enabled
- `detectorDataSize` tag = total payload size in bytes across all board blocks

The event then contains one block per FERS board. The block id is the board
index used inside the producer (`0`, `1`, `2`, ...), and the block payload is
the raw vendor event struct for that board.

Collector-facing behavior
-------------------------

The HiDRa DataCollector can consume the producer output as a single source.
That means the pipeline is valid for one-board runs and for aligned multi-board
runs, but the collector still sees the FERS2 producer as one source unless you
introduce extra source splitting logic. The `detectorDataSize` tag is important here:
the collector uses it when it re-wraps received events into the legacy merged
format.

The producer also prints a short debug summary for each decoded board event so
you can confirm trigger flow and payload contents in the log window during
hardware tests.

Slow-Control Status Polling
---------------------------

The FERS2 producer can independently poll slow-control monitor values from each
configured board without changing the raw event payload. Enable it in the
producer `.conf` section with:

```ini
FERS_STATUS_POLL_INTERVAL_S = 2
POLL_MONITOR_OUT_OF_SPILL = 1
```

Set the interval value to `0` to disable interval polling. When
`POLL_MONITOR_OUT_OF_SPILL` is enabled, the producer also polls once after 2
seconds without read events, regardless of the interval timeout. When enabled,
the producer reads:

- HV monitor voltage/current: `Vmon`, `Imon`
- temperatures: FPGA, board, TDC0/TDC1 when available, HV module, detector
- HV status bits: on/off, ramping, over-current, over-voltage

The latest values are logged and attached to outgoing EuDAQ events as tags such
as `FERS_STATUS_B0_VMON_V`, `FERS_STATUS_B0_IMON_MA`,
`FERS_STATUS_B0_TEMP_FPGA_C`, and `FERS_STATUS_B0_TEMP_DETECTOR_C`. To keep the
polling logs but avoid adding event tags, set:

```ini
FERS_STATUS_ATTACH_TAGS = 0
```

Raw board payloads
------------------

All payload structs below are copied directly from the CAEN FERSlib event types.
On the current Linux/x86 setup they are emitted in native little-endian layout.
Do not expect the payload to be portable across architectures with different
endianness or compiler packing rules.

Whatever the pyload struct, 9 bytes are manually appended on top of each board block when building the EuDAQ event in the FERSProducer:

- `marker = 0xAAAA`, `uint16_t`, offset 0
- `block size`, `uint16_t`, offset 2 (counting the number of bytes of this Board data, including these 5 bytes header)
- `data qualifier`, `uint32_t`, offset 4
- `board id`, `uint8_t`, offset 8

Spectroscopy / SPECT-TIMING: `SpectEvent_t`

- `tstamp_us` `double` at byte offset `9`
- `rel_tstamp_us` `double` 
- `tstamp_clk` `uint64_t` 
- `Tref_tstamp` `uint64_t` 
- `trigger_id` `uint64_t`
- `chmask` `uint64_t` 
- `qdmask` `uint64_t` 
- `energyHG[64]` `uint16_t[64]` starting at offset `...`
- `energyLG[64]` `uint16_t[64]` after `energyHG`
- `tstamp[64]` `uint32_t[64]` after `energyLG`
- `ToT[64]` `uint16_t[64]` after `tstamp`

This event type is used for spectroscopy and mixed spectroscopy+timing
operation. The `trigger_id` is the field used for alignment across boards.

Timing list: `ListEvent_t`

- `tstamp_us` `double`
- `Tref_tstamp` `uint64_t`
- `tstamp_clk` `uint64_t`
- `trigger_id` `uint64_t`
- `nhits` `uint16_t`
- `header1[8]` `uint32_t[8]`
- `header2[8]` `uint32_t[8]`
- `ow_trailer` `uint32_t`
- `trailer[8]` `uint32_t[8]`
- `channel[MAX_LIST_SIZE]` `uint8_t[]`
- `edge[MAX_LIST_SIZE]` `uint8_t[]`
- `tstamp[MAX_LIST_SIZE]` `uint32_t[]`
- `ToA[MAX_LIST_SIZE]` `uint32_t[]`
- `ToT[MAX_LIST_SIZE]` `uint16_t[]`

This is the format used when the board runs in timing/list mode. The producer
logs the first few channel, ToA and ToT entries to make it easy to validate the
stream.

Counting: `CountingEvent_t`

- `tstamp_us` `double`
- `rel_tstamp_us` `double`
- `trigger_id` `uint64_t`
- `chmask` `uint64_t`
- `counts[64]` `uint32_t[64]`
- `t_or_counts` `uint32_t`
- `q_or_counts` `uint32_t`

This is a compact count-rate payload. The summary log prints the first few
channel counts plus the OR counters.

Service: `ServEvent_t`

- `tstamp_us` `double`
- `update_time` `uint64_t`
- `pkt_size` `uint16_t`
- `version` `uint8_t`
- `format` `uint8_t`
- `ch_trg_cnt[]` channel trigger counters
- `q_or_cnt` / `t_or_cnt`
- temperature, HV monitor and status fields
- total trigger counters and rejection/suppression counters

This event is mostly diagnostic. The producer logs the most useful counters and
the HV/temperature values when such an event appears.

Test mode: `TestEvent_t`

- `tstamp_us` `double`
- `trigger_id` `uint64_t`
- `nwords` `uint16_t`
- `test_data[]` fixed test pattern words

Current limitation
------------------

The producer currently aligns board payloads by `trigger_id` before sending the
EuDAQ event. That works with one board and with multiple boards, but the output
still uses a single producer source from the DataCollector point of view. If you
want one downstream subevent per physical board, the collector side would need
an additional split step.

Contact
-------
For questions about behaviour, configuration format or field tests, ping the
HiDRa maintainers in the repository or open an issue.
