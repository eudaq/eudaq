# HiDRA run guide

This directory contains the scripts and configuration files used to run a HiDRA
acquisition with EUDAQ.

## Main files

| File | Description |
| --- | --- |
| `hidra_startrun_joint.sh` | Starts Run Control, Log Collector, DataCollector, QTPDProducer and FERS2Producer. Use this script for a full VME + FERS acquisition. |
| `hidra_startrun.sh` | Starts the HiDRA chain without FERS2. |
| `hidra_startrun_fers.sh` | Starts the FERS chain. |
| `hidra_startrun_dry.sh` | Starts a replay acquisition from test files. Useful for tests without hardware. |
| `config/hidra.ini` | Initialisation configuration: EUDAQ process names, VME controller and logging. |
| `config/hidra.conf` | Run configuration: enabled producers, VME boards, FERS settings, output and synchronisation. |
| `CONFIG.md` | Reference for the main parameters in `config/hidra.conf`. |

## Before starting

Make sure the HiDRA/EUDAQ environment is already configured and that the
`EUDAQHIDRA` environment variable points to the correct directory. The
`hidra_startrun_joint.sh` script also uses this variable to start the monitoring
dashboard.

Also check that these values in `config/hidra.conf` are correct for the current
setup:

- the FERS configuration path in `FERS_CONF_FILE`;
- the VME board addresses in `BoardN.BaseAddress`;
- the GEO addresses in `BoardN.GeoAddress`;
- the maximum number of events in `MAX_EVENTS`.

## Start a full acquisition

From this directory, run:

```sh
./hidra_startrun_joint.sh
```

The script starts the required EUDAQ processes. In the Run Control window:

1. Press `Init` to initialise the processes.
2. Press `Config` to load `config/hidra.conf` and configure the hardware and DataCollector.
3. Press `Start` to start the run.
4. Wait for the automatic stop, or press `Stop` manually.
5. To start another run with the same configuration, press `Start` again.
6. If you edit `config/hidra.conf`, press `Config` again before the next run.
7. Press `Terminate` to close EUDAQ.

The run stops automatically only when `MAX_EVENTS` is greater than zero. With
`MAX_EVENTS = 0`, the acquisition keeps running until it is stopped manually.

## Output

Output files are written according to the pattern configured in
`EUDAQ_FW_PATTERN`, normally under `out_data/`. Logs are written under `logs/`.

In `config/hidra.conf`, the output formats are controlled by:

- `WRITE_BINARY_OUTPUT = 1` enables HiDRA binary output;
- `WRITE_ROOT_OUTPUT = 1` enables ROOT output;
- if both are set to `0`, the DataCollector runs without writing output files.

## Hardware-free replay

To run a replay test from example files:

```sh
./hidra_startrun_dry.sh
```

This mode uses `config/dry.conf`, not `config/hidra.conf`.

## Change the configuration

The main run parameters are stored in `config/hidra.conf`. After editing the
file:

1. stop the current run, if one is running;
2. save the file;
3. press `Config` in Run Control;
4. press `Start`.

See `CONFIG.md` for the meaning of the parameters and for common examples.
