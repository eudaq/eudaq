# Parameters in `config/hidra.conf`

This document describes the parameters used by `config/hidra.conf` for a HiDRA
EUDAQ run. The configuration is split into sections; each section configures one
EUDAQ component.

Boolean values are normally written as `0` or `1`. Paths may be absolute or
relative to the directory from which the run script is started.

## Section `[RunControl]`

This section controls the EUDAQ Run Control behaviour.

| Parameter | Example | Meaning |
| --- | --- | --- |
| `config_log_path` | `logs/` | Directory where the Run Control GUI copies the configuration used for a run. |
| `EUDAQ_CTRL_PRODUCER_LAST_START` | `QTPDProducer` | Producer started last when `Start` is pressed. In the current setup this lets the FERS producer start before the QTPD/VME producer. |
| `EUDAQ_CTRL_PRODUCER_FIRST_STOP` | `QTPDProducer` | Producer stopped first when `Stop` is pressed. |
| `ADDITIONAL_DISPLAY_NUMBERS` | `"log,_SERVER"` | Optional. Adds status values to the EUDAQ GUI. The format is `processName,statusTag`. |

## Section `[Producer.QTPDProducer]`

This section configures `HidraQTPDProducer`, the VME/QDC producer used for the
CAEN V792 boards and the CAEN V977 I/O module.

| Parameter | Example | Meaning |
| --- | --- | --- |
| `EUDAQ_DC` | `HidraDataCollector` | DataCollector instance that receives QTPD events. It must match the producer target name used by EUDAQ. |
| `HIDRA_MUTE_DEBUG` | `0` | EUDAQ log level for this producer. `0` is the most verbose setting used here. |
| `Iped` | `100` | Pedestal/current value written to each enabled V792 board. |
| `V977_BASE` | `0x01000000` | VME base address of the V977 module used for trigger, veto and spill handling. |

### VME Board Blocks

Each V792 board is configured with a `BoardN` block. `N` is only the
configuration index; enabled boards are discovered from `BoardN.Enable` and read
in increasing `N` order.

| Parameter | Example | Meaning |
| --- | --- | --- |
| `BoardN.Enable` | `1` | Enables (`1`) or disables (`0`) this board. Disabled boards can remain in the file without being used. |
| `BoardN.BaseAddress` | `0x06000000` | VME base address of the board. Must match the hardware address. |
| `BoardN.GeoAddress` | `2` | GEO address programmed/read for the V792. This must also be listed in `VME_CRATE_1` in the DataCollector section. |
| `BoardN.CrateNumber` | `1` | VME crate number written to the board. |
| `BoardN.Type` | `V792` | Board type. The current QTPD producer supports the V792 readout path. |
| `BoardN.MulticastRole` | `FIRST` | Optional. Overrides the automatically assigned multicast role. Allowed values are `FIRST`, `MIDDLE` and `LAST`. |

Example:

```ini
Board0.Enable = 1
Board0.BaseAddress = 0x06000000
Board0.GeoAddress = 2
Board0.CrateNumber = 1
Board0.Type = V792
```

To keep a board in the file but exclude it from the run:

```ini
Board0.Enable = 0
```

## Section `[Producer.FERS2Producer]`

This section configures `HidraFERS2Producer`. Board-level FERS settings are read
from the external FERSlib configuration file selected with `FERS_CONF_FILE`.

| Parameter | Example | Meaning |
| --- | --- | --- |
| `EUDAQ_DC` | `HidraDataCollector` | DataCollector instance that receives FERS2 events. |
| `HIDRA_MUTE_DEBUG` | `0` | EUDAQ log level for this producer. |
| `FERS_CONF_FILE` | `/path/FERSlib_Config_oneboard.txt` | FERSlib configuration file. This is where FERS boards, connections, thresholds and FERS-specific parameters are defined. |
| `FERS_START_MODE` | `ASYNC` | Optional. Start mode for FERSlib. Supported values are `ASYNC`, `CHAIN_T0`, `CHAIN_T1`, `TDL`, or the explicit FERSlib names `STARTRUN_ASYNC`, `STARTRUN_CHAIN_T0`, `STARTRUN_CHAIN_T1`, `STARTRUN_TDL`. Default: `STARTRUN_ASYNC`. |
| `FERS_CONFIGURE_MODE` | `CFG_HARD` | Optional. FERS configuration mode. Supported values are `CFG_HARD` and `CFG_SOFT`. Default: `CFG_HARD`. |
| `FERS_READOUT_MODE` | `0` | Optional. Readout mode passed to FERSlib when connecting to the boards. Default: `0`. |
| `FERS_POLL_SLEEP_US` | `1` | Sleep time, in microseconds, between polling loops. Lower values reduce latency but increase CPU usage. |
| `FERS_MAX_EVENTS_PER_BOARD` | `0` | Optional. Maximum number of events read per board in each polling loop. Leave commented or set to `0` for no limit. |
| `FERS_STATUS_POLL_INTERVAL_S` | `10` | Interval, in seconds, for polling monitor values such as voltage, current, temperatures and HV status. `0` disables status polling. |
| `POLL_MONITOR_OUT_OF_SPILL` | `0` | If `1`, also polls monitor status once after 2 seconds without read events, regardless of the `FERS_STATUS_POLL_INTERVAL_S` timeout. |
| `FERS_STATUS_ATTACH_TAGS` | `0` | If `1`, attaches the latest status values as tags to outgoing EUDAQ events. If `0`, status values are only logged. |
| `FERS_SEND_TIMESTAMP` | `1` | If `1`, uses the FERS timestamp as the EUDAQ event timestamp. |

## Section `[DataCollector.HidraDataCollector]`

This section configures `HidraDataCollector`, which receives events from the
producers, synchronises them, sends monitor events and writes the HiDRA outputs.

| Parameter | Example | Meaning |
| --- | --- | --- |
| `EUDAQ_MN` | `my_mon` | EUDAQ monitor instance that receives monitoring events, if one is connected. |
| `MAX_EVENTS` | `0` | Maximum number of merged events before the DataCollector requests a stop. `0` disables the automatic stop. |
| `EUDAQ_FW` | `HidraNullFileWriter` | EUDAQ file-writer type. HiDRA handles its own binary/ROOT writing, so this is normally kept as `HidraNullFileWriter`. |
| `EUDAQ_FW_PATTERN` | `out_data/run$3R_$12D$X` | Output filename pattern used by the HiDRA writers. `$3R` is the run number with 3 digits, `$12D` is the date/time, and `$X` is the writer extension. |
| `EUDAQ_DATACOL_SEND_MONITOR_FRACTION` | `10` | Fraction of events sent to the EUDAQ monitor. `10` means about one event every 10. |
| `WRITE_BINARY_OUTPUT` | `1` | Enables HiDRA binary output. |
| `WRITE_ROOT_OUTPUT` | `1` | Enables ROOT output. |
| `WRITER_FLUSH_INTERVAL_MS` | `50` | Writer flush interval in milliseconds. Lower values flush more often. |
| `EXPECTED_SOURCES` | `2:FERS2Producer,1:QTPDProducer` | Map from detector ID to producer name. The DataCollector waits for these producers when building merged events. |
| `SYNC_TIMEOUT_US` | `1000000` | Timeout, in microseconds, for waiting for all expected sources for the same trigger. After the timeout, an incomplete event may be written. |
| `TIME_ALIGNMENT_ENABLE` | `1` | Enables time-alignment calibration between sources. |
| `TIME_ALIGNMENT_WINDOW_NS` | `300000` | Time window, in nanoseconds, used to match timestamps during alignment. |
| `TIME_ALIGNMENT_NEVENTS_CALIB` | `100` | Number of events used to estimate the time alignment. |
| `VME_CRATE_1` | `2:V792,4:V792,6:V792,8:V792,10:V792` | Map from VME GEO address to module type. Used by the decoder to map VME channels. |
| `HIDRA_MUTE_DEBUG` | `0` | EUDAQ log level for the DataCollector. |

### `EXPECTED_SOURCES`

The format is:

```ini
EXPECTED_SOURCES = DetectorID1:ProducerName1,DetectorID2:ProducerName2
```

For the current full VME + FERS run:

```ini
EXPECTED_SOURCES = 2:FERS2Producer,1:QTPDProducer
```

The producer names must match the names passed to `euCliProducer` with the `-t`
option in the run script.

If `EXPECTED_SOURCES` is empty, the DataCollector accepts only the first source
it receives and assigns it detector ID `0`.

### `VME_CRATE_1`

The format is:

```ini
VME_CRATE_1 = GeoAddress1:ModuleType,GeoAddress2:ModuleType
```

For the current setup:

```ini
VME_CRATE_1 = 2:V792,4:V792,6:V792,8:V792,10:V792
```

Keep this map consistent with the enabled `BoardN.GeoAddress` values in
`[Producer.QTPDProducer]`. If a V792 board is added, removed, disabled or moved
to a different GEO address, update both places.

## Common Changes

### Stop Automatically After N Events

```ini
MAX_EVENTS = 10000
```

With `MAX_EVENTS = 0`, the run continues until `Stop` is pressed manually.

### Enable Only One Output Format

Binary only:

```ini
WRITE_BINARY_OUTPUT = 1
WRITE_ROOT_OUTPUT = 0
```

ROOT only:

```ini
WRITE_BINARY_OUTPUT = 0
WRITE_ROOT_OUTPUT = 1
```

### Add a V792 Board

1. Add a new `BoardN` block in `[Producer.QTPDProducer]`.
2. Set `Enable = 1`, `BaseAddress`, `GeoAddress`, `CrateNumber` and `Type`.
3. Add the same GEO address and module type to `VME_CRATE_1`.
4. Press `Config` in Run Control before starting the next run.

### Disable a V792 Board

1. Set `BoardN.Enable = 0`.
2. Remove the corresponding GEO address from `VME_CRATE_1`.
3. Press `Config` in Run Control before starting the next run.
