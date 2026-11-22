# Hidra QTPD Producer

[!WARNING]
This document is partially out of date. It is kept as a reference for developers. A version aligned with the current code will be prepared as soon as the code will be finalized.

This document describes the current implementation in
`user/hidra/xdc/src/HidraQTPDProducer.cc`.

`HidraQTPDProducer` is an EUDAQ producer for the HIDRA CAEN V792 QDC chain. It
opens a CAEN V2718 VME controller, configures the enabled V792 boards described
in the run configuration for multicast/block-transfer readout, uses a CAEN V977
I/O module for trigger and spill latches, and sends raw QDC payloads as EUDAQ
events.

## Startup Names

The producer class registered with EUDAQ is:

```text
HidraQTPDProducer
```

The HIDRA run scripts currently start it with the EUDAQ producer instance name
`QTPDProducer`, for example:

```text
euCliProducer -n HidraQTPDProducer -t QTPDProducer
```

The corresponding configuration sections are therefore
`[Producer.QTPDProducer]` in the `.ini` and `.conf` files.

## Init Configuration

`DoInitialise()` reads the EUDAQ init configuration and opens the VME
controller. The current code supports only:

| Key | Default | Meaning |
| --- | --- | --- |
| `ControllerType` | `V2718` | CAEN controller type. Any other value throws. |
| `LinkOrPid` | `0` | Value passed to `CAENVME_Init2`; older USB setups may use `49086`. |

Example:

```ini
[Producer.QTPDProducer]
ControllerType = V2718
LinkOrPid = 0
```

## Run Configuration

`DoConfigure()` reads the run configuration, resets the run counters, programs
the V792 boards, clears/configures the V977, and leaves the trigger veto active.

| Key | Default | Meaning |
| --- | --- | --- |
| `HIDRA_MUTE_DEBUG` | `0` | Passed to `EUDAQ_LOG_LEVEL`. |
| `Iped` | `100` | Written to each V792 `V792_IPED_REG`. |
| `V977_BASE` | `0x01000000` | VME base address of the V977 I/O module. |
| `BoardN.Enable` | disabled | Enables board `N`; disabled if `0`, `false`, or `False`. |
| `BoardN.BaseAddress` | required when enabled | VME base address for board `N`. |
| `BoardN.GeoAddress` | required when enabled | V792 GEO address for board `N`. |
| `BoardN.CrateNumber` | required when enabled | V792 crate number for board `N`. |
| `BoardN.Type` | required when enabled | Board type. Currently only `V792` is supported. |
| `BoardN.MulticastRole` | derived | Optional override: `FIRST`, `MIDDLE`, `LAST`, or the corresponding numeric values `2`, `3`, `1`. |

The code discovers board indices from the configured `BoardN.Enable` keys, so
the number of configured board slots is no longer hard-coded. At least one board
must be enabled. Enabled boards are read in ascending `N` order.

## V977 Signal Mapping

The producer decodes the V977 single-hit register with this input mapping:

| Signal | Enum | Channel |
| --- | --- | --- |
| Fast trigger gate | `V977IN::cFastGate` | `0` |
| Physics trigger flag | `V977IN::cPhy` | `1` |
| Pedestal trigger flag | `V977IN::cPed` | `2` |
| Spill start latch | `V977IN::cSpillStart` | `3` |
| Spill end latch | `V977IN::cSpillEnd` | `4` |

The active output mapping is:

| Output | Enum | Channel |
| --- | --- | --- |
| Global trigger veto | `V977OUT::cVeto` | `0` |
| Pedestal veto | `V977OUT::cPedVeto` | `5` |

There is no active physics-veto output in the current code.

`ClearV977FlipFlops()` writes `0xFFFF` to `V977_OUTPUT_CLEAR_REG`. The producer
uses this during configuration, at run start, after a trigger is processed, and
when it detects a spill with no trigger.

## V792 Board Setup

Each enabled board is initialized by `InitBoard()`:

1. Set GEO address.
2. Reset via `V792_BIT_SET_1_REG` / `V792_BIT_CLEAR_1_REG`.
3. Read and log the model number.
4. Set crate number and `Iped`.
5. Program the control and bit-set registers used by this readout mode.
6. Reset the V792 event counter.

After board initialization, the producer writes `0xAA` to each board's
`V792_BLT_EVENT_NUMBER_REG`, then programs the multicast chain through
`V792_MCST_CBLT_ADDRESS_REG`.

By default, multicast roles are derived from the enabled-board order:

| Enabled-board position | Role |
| --- | --- |
| first enabled board | `FIRST` |
| last enabled board | `LAST` |
| all boards between them | `MIDDLE` |

`BoardN.MulticastRole` can override the derived role for a board when the
hardware chain needs an explicit assignment.

## Run Start And Stop

At `DoStartRun()` the producer:

1. Stops any previous acquisition thread.
2. Stores the EUDAQ run number.
3. Resets counters and timestamps.
4. Sends a BORE event.
5. Clears V977 flip-flops and writes `0x0000` to `V977_INPUT_SET_REG`.
6. Starts the acquisition thread running `MainLoop()`.
7. Releases the global trigger veto.

At `DoStopRun()` it:

1. Sets `m_running` and `m_onspill` false.
2. Joins the acquisition thread.
3. Sends an EORE event.
4. Sets the global trigger veto and pedestal veto.

`DoReset()` stops the acquisition thread and resets counters. `DoTerminate()`
stops the thread and closes the CAEN controller.

## Main Loop

`MainLoop()` owns acquisition. The EUDAQ `RunLoop()` is intentionally idle.

Per iteration, the loop:

1. Skips work until the VME controller handle is valid.
2. Reads the V977 single-hit register.
3. If spill-start and spill-end are both latched without a trigger, logs a
   warning, increments the spill counter, and clears V977 flip-flops.
4. If a fast trigger is latched, timestamps the event, builds the trigger mask,
   reads one QDC block, updates counters and status tags, adjusts the pedestal
   veto, updates the spill counter if the event also latched spill-start, clears
   V977 flip-flops, and releases the global trigger veto.

There is currently no sleep in the idle path.

## Trigger Mask

For every fast trigger, the producer sets `m_TriggerMask` from the V977
classification bits:

| Mask | Meaning |
| --- | --- |
| `0` | fast gate without physics or pedestal flag |
| `1` | physics flag latched |
| `2` | pedestal flag latched |
| `3` | both physics and pedestal flags latched |

If both classification bits are latched, the producer logs a warning and still
sends the event with `triggerMask=3`.

## Pedestal Selection

After a triggered event, the producer decides whether to request a pedestal next:

```cpp
((double)m_evt_phy / (double)m_evt_ped) > 10
```

It then writes channel 5 through `SetSingleV977OutputReg()`:

| Condition | Channel 5 value | Intended effect |
| --- | --- | --- |
| `requestPedestalNext()` is true | low | allow pedestal trigger |
| `requestPedestalNext()` is false | high | veto pedestal trigger |

At the beginning of a run `m_evt_ped` is zero, so this decision relies on the
platform's floating-point division-by-zero behavior.

## QDC Readout

`ReadOneBlockAndSendEvent()` first waits for all enabled boards to be ready and
for none to be busy.

| Parameter | Value |
| --- | --- |
| Timeout | `100 us` |
| Poll interval | `10 us` |
| Status register | `V792_STATUS_1_REG` at each board base address |
| Ready bit | bit `0` |
| Busy bit | bit `2` |

If the boards are not ready before the timeout, the producer logs an error and
does not send a data event.

If the boards are ready, it performs:

```text
CAENVME_FIFOMBLTReadCycle(..., 0xAA000000, ..., cvA32_U_MBLT, ...)
```

The maximum requested transfer size is `4096` bytes. `cvSuccess` and
`cvBusError` are both accepted as non-fatal return codes. A zero or negative
byte count is treated as an error and no event is sent.

## EUDAQ Events

The BORE event uses type:

```text
CAENQTPDRaw
```

The EORE event uses type:

```text
CAENQTPRaw
```

The data events use type:

```text
CAENQTPDRaw
```

Data events contain one raw block, block `0`, with the exact bytes returned by
the VME block transfer. They carry:

| Field/tag | Value |
| --- | --- |
| trigger number | `m_evt` before it is incremented |
| event number | `m_evt` before it is incremented |
| run number | current EUDAQ run number |
| block `0` | raw QDC bytes |
| `spillNumber` | current `m_spillCount` |
| `triggerMask` | current trigger mask |
| `endianness` | `BE32` |
| timestamp begin | `hidra::utils::getTimens()` |
| timestamp end | timestamp begin + `100 ns` |
| `detectorDataSize` | raw payload size in bytes |

The BORE event includes `NumBoards` and, for each enabled-board position,
`BoardX_ConfigIndex`, `BoardX_Base`, `BoardX_Geo`, `BoardX_Crate`,
`BoardX_Type`, and `BoardX_MulticastRole`.

After `ReadOneBlockAndSendEvent()` returns, `m_evt` is incremented even if no
data event was sent. `m_evt_phy` is incremented only for mask `1`; `m_evt_ped`
is incremented only for mask `2`.

Status tags sent after each triggered event are:

| Tag | Value |
| --- | --- |
| `PhyTrigN` | `m_evt_phy` |
| `PedTrigN` | `m_evt_ped` |
| `SpillN` | `m_spillCount` |

The EORE event tags `EventsSent` with `m_evt`; this is the number of fast
trigger paths handled, not necessarily the number of data events successfully
sent.

## Current Caveats

- `ReadOneBlockAndSendEvent()` returns a success flag, but `MainLoop()` does not
  use it when updating `m_evt`.
- The spill counter increments on trigger events that include `spillStart`, and
  also on the special `spillStart && spillEnd && !trigger` path. There is no
  separate spill state machine in the active code.

