# TrackerProducer

`TrackerProducer` watches a directory for delimiter-separated tracker files and
sends one EUDAQ event for every data row. A file is processed once its size is
unchanged across two directory polls.

The provisional ten column names are deliberately kept together near the top of
`src/TrackerProducer.cc` in `TRACKER_COLUMNS`. Change that array, plus
`TRIGGER_COLUMN` and `TIMESTAMP_COLUMN` when the final format is known.

Example CSV:

```text
TriggerId,Time stamp,X,Y,Column5,Column6,Column7,Column8,Column9,Column10
42,1710000000000,12.5,8.0,0,0,0,0,0,0
```

Example run configuration:

```ini
[Producer.Tracker]
EUDAQ_DC = HidraDataCollector
HIDRA_MUTE_DEBUG = 0
TRACKER_DIRECTORY = /path/to/tracker/files
TRACKER_FILE_EXTENSION = .csv
TRACKER_DELIMITER = ,
TRACKER_POLL_INTERVAL_MS = 100
TRACKER_TIMESTAMP_SCALE_NS = 1
```

Start it with:

```sh
euCliProducer -n HidraTrackerProducer -t TrackerProducer
```

Each event stores every column as an EUDAQ tag and stores the original data row
as bytes in block `0`. `TRACKER_TIMESTAMP_SCALE_NS` converts the integer value
in `Time stamp` to nanoseconds; for example, use `1000` for microseconds.
