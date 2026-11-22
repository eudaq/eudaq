# HIDRA Binary Event Format (v10)

The binary event format is produced by the `EventSerializer` utility.

All fields except the detector payload are encoded in **little-endian** format. The detector payload itself is stored using the native byte ordering provided by the front-end system. Its endianness is specified by the dedicated `endianness` field and must be used to correctly interpret the payload data.


## Event Structure

```text
+----------------------+
| Event Header         |
+----------------------+
| Detector Event #0    |
+----------------------+
| Detector Event #1    |
+----------------------+
| ...                  |
+----------------------+
| Event Trailer        |
+----------------------+
```

The event may contain up to 8 detector subevents.



## Event Header

| Offset | Type      | Field            | Description                 |
| ------ | --------- | ---------------- | --------------------------- |
| 0      | uint16    | `marker`         | `0xB0BF`                    |
| 2      | uint8     | `dataVersion`    | Format version `0x0A`	      |
| 3      | uint32    | `headerSize`     | Header size in bytes        |
| 7      | uint32    | `trailerSize`    | Trailer size in bytes       |
| 11     | uint32    | `eventSize`      | Total event size, bytes     |
| 15     | uint16    | `runNumber`      | Run number                  |
| 17     | uint32    | `eventNumber`    | Trigger/event number        |
| 21     | uint32    | `spillNumber`    | Spill number                |
| 25     | uint64    | `eventTime`      | Merged event begin timestamp, ns |
| 33     | uint8     | `triggerMask`    | Trigger mask                |
| 34     | uint32    | `reserved32`     | Reserved                    |
| 38     | uint32    | `eventFlags`     | Bit flags                   |
| 42     | uint32    | `timeSpread`     | Time spread among subevents, ns |
| 46     | uint8     | `detectorMask`   | Active detectors bitmap     |
| 47     | uint16[8] | `detectorSize[]` | Detector size metadata      |
| 63     | uint16    | `endMarker`      | `0xBBBB`                    |

The event header has a fixed size of `65 bytes (0x41)`.

### Event Timestamp

The event-header `eventTime` is written from `event.GetTimestampBegin()`. For merged events this is the earliest begin timestamp among the detector subevents selected by the `HidraDataCollector`. The value is encoded as a `uint64` in nanoseconds.

The `timeSpread` field is the difference between the merged event end timestamp and begin timestamp, also in nanoseconds. If the spread is not valid or does not fit in `uint32`, it is written as `0xFFFFFFFF`.

### Detector Mask

The `detectorMask` field encodes which Producers (detectors) are present in the event.
Each bit corresponds to a detector ID (`detID`), where bit `N` represents detector `N`.

Detector IDs are assigned in the `DataCollector` configuration through the `EXPECTED_SOURCES` entry in the `.conf` file:

```ini
EXPECTED_SOURCES = 6:DryXDCProducer,7:DryFERSProducer
```

Each source is specified as:

```text
detectorID:ProducerRuntimeName
```

where:

* `detectorID` is an integer between `0` and `7`
* `ProducerRuntimeName` is the runtime name of the EUDAQ Producer application

The following detector ID convention is recommended:

| detID (bit) | Producer |
| ----------- | -------- |
| 0           | Don't use |
| 1           | XDC       |
| 2           | FERS      |
| ...         | ...       |
| 6           | Dry XDC   |
| 7           | Dry FERS  |


Index 0 is assigned when the collector is run in `single producer` mode.

Example: `detectorMask = 0b00000110` means that detectors with `detID = 1` and `detID = 2` are present in the event. The corresponding detector event data blocks follow after the Header endMarker, ordered by increasing `detID`.

### Event flags

At most 24 bits used to flag events in specific cases. Events with no warning should have the bitmap `= 0x0`.
This is still work in progress, for the bit meaning see the `HidraEventFlags` struct in `HidraUtils.hh`


## Detector Event

Each detector payload is stored as:

| Offset   | Type   | Field            | Description                |
| -------- | ------ | ---------------- | -------------------------- |
| 0        | uint16 | `marker`         | `0xDEDE`                   |
| 2        | uint8  | `detectorID`     | Detector ID                |
| 3        | uint32 | `eventNumber`    | Local event/trigger number |
| 7        | uint32 | `spillNumber`    | Spill number       |
| 11       | uint64 | `eventTime`      | Detector begin timestamp, ns |
| 19       | uint64 | `nativeEvtTime`  | Native detector begin timestamp, ns |
| 27       | uint16 | `reserved`       | Reserved           |
| 29       | uint8  | `triggerMask`    | Trigger mask       |
| 30       | uint8  | `endianness`     | Endianness         |
| 31       | byte[] | `payload`        | Detector payload   |
| variable | uint16 | `endMarker`      | `0xDDDD`           |

The payload is the concatenation of all EUDAQ blocks associated to the detector.

### Native Timestamp

The detector-event `eventTime` field is written from the EUDAQ subevent timestamp, `sub_ev->GetTimestampBegin()`. This is the timestamp used by the collector and writers as the aligned/common event time.

The detector-event `nativeEvtTime` field is written from the subevent tag `nativeTimestampBegin`. It preserves the producer-local timestamp before, or alongside, any conversion to the common timestamp used for event building. If the producer does not provide the `nativeTimestampBegin` tag, the serializer writes `0xFFFFFFFFFFFFFFFF`.

Current producer conventions:

- `HidraFERS2Producer`: `eventTime` is the FERS library event timestamp, in ns, shifted by the producer's host-side start reference taken immediately before `FERS_StartAcquisition()`. `nativeEvtTime` is the unshifted FERS timestamp in ns, i.e. `eventTime - start_reference_ns`. It is relative to the FERS time reset/start reference, not Unix epoch time.
- `HidraQTPDProducer` and dry producers: `nativeEvtTime` is currently the same value as the event time.


The `endianness` field specifies the byte ordering used by the detector payload:

- `0x1`: Little-endian
- `0x2`: Big-endian
- `0xFF`: Not specified

The payload byte ordering is preserved as received from the front-end electronics and is **not** converted during event building or serialization. Therefore, the payload endianness may depend on the architecture or firmware implementation of the front-end system producing the data.







## Event Trailer

| Type   | Value    |
| ------ | -------- |
| uint16 | `0xD04E` |


## Detector-specific payload

### FERS

See [fers2/README.md](fers2/README.md)

### XDC

...


## Data version change log

- `v11` (2026, Jun 05): adding event flags as 32-bit word in the header 
- `v10` (2026, Jun 03): document detector `nativeEvtTime` as the serialized `nativeTimestampBegin` tag and clarify event timestamp fields
- `v8` (2026, May 22): adding data qualifier as `uint32` inside each FERS payload block
- `v7` (2026, May 20): adding two more bytes as marker (`0xAAAA`) at the beginning of each FERS Board payload
- `v6`
