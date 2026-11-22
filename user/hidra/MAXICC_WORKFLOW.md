# MAXICC Producer And ROOT Decoder Workflow

## Summary Of Changes

A new producer named `MAXICCProducer` was added as a sibling of the existing FERS2 producer.

It reuses the same FERS2 readout implementation, but emits events tagged as:

```text
Producer = MAXICCProducer
```

A new ROOT payload decoder was also added. It mimics the existing FERS decoder, but fills separate ROOT branches with the `MAXICC` prefix instead of the `FERS` prefix.

The FERS decoder still assumes up to `20` boards:

```text
64 * 20 = 1280 channels
```

The MAXICC decoder assumes up to `3` boards:

```text
64 * 3 = 192 channels
```

## New Producer

The new EUDAQ producer factory name is:

```text
MAXICCProducer
```

Launch it with:

```sh
euCliProducer -n MAXICCProducer -t MAXICCProducer
```

Here:

```text
-n MAXICCProducer
```

selects the C++ producer factory.

```text
-t MAXICCProducer
```

sets the EUDAQ connection/instance name. This is the name used by `HidraDataCollector` to assign detector IDs.


## Configuration Sections

Each producer instance needs its own configuration section. The section name must match the `-t` name.

Example:

```ini
[Producer.FERS2Producer]
EUDAQ_DC = HidraDataCollector
FERS_CONF_FILE = /path/to/fers_config.txt
FERS_STATUS_POLL_INTERVAL_S = 15

[Producer.MAXICCProducer]
EUDAQ_DC = HidraDataCollector
FERS_CONF_FILE = /path/to/maxicc_config.txt
FERS_STATUS_POLL_INTERVAL_S = 15
```

Both producers use `FERS_CONF_FILE`, because MAXICC reuses the FERS2 hardware/readout implementation.

## Detector ID Assignment


```ini
[DataCollector.HidraDataCollector]
EXPECTED_SOURCES = 1:QTPDProducer,2:FERS2Producer,5:MAXICCProducer
```


## ROOT Branch Selection

The ROOT writer decides which decoder to use in `HidraRootEventWriter`.

The dispatch order is:

```text
XDC decoder
MAXICC decoder
FERS decoder
Tracker decoder
Generic decoder
```

The MAXICC decoder is selected when:

```text
Producer == MAXICCProducer
```

or:

```text
detID == 5
```

The FERS decoder is selected when:

```text
detID == 2
```


## ROOT Output Branches

Normal FERS events fill:

```text
FERStsamp_us
FERSrel_tsamp_us
FERStrigger_id
FERSboard_id
FERShg
FERSlg
FERStoa
FERStot
```

MAXICC events fill:

```text
MAXICCtsamp_us
MAXICCrel_tsamp_us
MAXICCtrigger_id
MAXICCboard_id
MAXICChg
MAXICClg
MAXICCtoa
MAXICCtot
```

The branch content is decoded from the same payload format as FERS.

## Channel Vector Sizes

FERS ROOT vectors are sized for 20 boards:

```text
64 * 20 = 1280 entries
```

MAXICC ROOT vectors are sized for 3 boards:

```text
64 * 3 = 192 entries
```

If a board ID is outside the allowed range, the decoder skips that board block.

For MAXICC, valid board IDs are therefore:

```text
0, 1, 2
```



## Practical Notes

Use different `FERS_CONF_FILE` values for FERS and MAXICC if they read out different hardware sets.

Keep `-t MAXICCProducer` unless the decoder matching rule is updated, because the ROOT decoder currently recognizes MAXICC by `Producer == MAXICCProducer` or `detID == 5`.

Use `detID == 5` for MAXICC to keep the workflow explicit and stable.

Use `detID == 2` for the existing FERS stream.
