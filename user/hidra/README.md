# HiDRA2

Prerequisites and build

- Main external requirements:
  - **CAENVMELib** >= 4.1.3
  - **CAENComm** >= 1.8.0
  These libraries are provided by CAEN: install them following the vendor's
  instructions and place headers and libraries in locations visible to the
  compiler and linker.
  - **CAENFERSlib** >= 1.3.0 (see also [fers2 section](fers2/README.md))

- Other prerequisites:
  - `fmt` formatting library (`sudo apt install libfmt-dev`, or `sudo dnf install fmt fmt-devel`, or...)

- Quick install (recommended — uses local helpers):

```sh
# from the repository root (e.g. eudaq_hidra)
source user/hidra/misc/setup.sh
# configure cmake (uses HiDRA presets if available)
cmake_config
# build and install (calls the full build)
build_hidra
```

If the system does not support the required CMake Presets, the script provides
a fallback that runs `cmake -S <root> -B build` and `cmake --build build -j 10`.

- VSCode: to create local (non-committed) presets use:

```sh
source user/hidra/misc/setup.sh
setup_vscode_hidra
```

Note: communication with scopes/instruments may require VISA:

```sh
sudo apt install ni-visa ni-visa-devel
```

Brief description of the `user/hidra` structure (main folders)

- `dc/`        : code for `HidraDataCollector` (collector)
- `dry/`       : "dry" producers for testing/simulation (HidraDryFERSProducer, HidraDryXDCProducer)
- `fers/`      : drivers and libraries for FERS (hardware specific) (inherited by other EuDAQ users -- not for HiDRA TB)
- `fers2/      : software for HiDRA 2026 TB
- `misc/`      : helper scripts and presets (e.g. `setup.sh`, `CMakePresets.hidra.json`)
- `rc/`        : RunControl (HidraRunControl)
- `run/`       : scripts to start the system (e.g. `hidra_startrun.sh`, `hidra_startrun_dry.sh`)
- `xdc/`       : XDC producers/decoders
- `common/`    : utilities and shared declarations

The collector documentation now lives in [dc/README.md](dc/README.md). It
describes the asynchronous binary writer, the optional ROOT writer, and the
runtime toggles used in the `DataCollector` config.

The main part for running the software after compilation is the `user/hidra/run`
folder. The scripts in that folder launch the binaries installed into `bin/` and
are the easiest way to start a test run, for example:

```sh
cd user/hidra/run
# start a "dry" run (no hardware)
./hidra_startrun_dry.sh
# or the full run
./hidra_startrun.sh
```

For more details about the available helpers see `user/hidra/misc/setup.sh`.

## Git workflow procedure

- Everyone works on its own branch
- Files should be formatted according to specifications in `user/hidra/.clang-format`. From command line, close the file and run `clang-format -i <path-to-file>`. On VSCode, `Alt+Shift+F`
- When a new feature is ready it needs to be merged to master with a Pull Request:
   - Commit any relevant changes
   - `git switch master` and `git pull` to update local master branch
   - `git switch <your branch>` and `git rebase master` to include changes on master on your own branch
   - Solve any merge conflict and finally `git push` to push your changes to your remote branch
   - In the web browser go to pull request, select master as target branch and your branch as source and create the pull request
   - When request is ready to merge -----> merge it.

## Tricks

- List processes listening to a port: `lsof -i :44000`

## Data format


### 2026 Data format: [DataFormat.md](DataFormat.md)




#### For reference, XDC 2025 event format

Decoder in [DRCalo/DreamDaqMon](https://github.com/DRCalo/DreamDaqMon)

TB data in [cernbox](https://cernbox.cern.ch/s/H6yDF4TNRez6jsw), or `/eos/user/i/ideadr/TB2025_H8`.

- **Event header**: 14 words, 32 bits each: (in brackets fixed expected values)
 ```
 eventMarker (0xccaaffee) | eventNumber | spillNumber |
 headerSize (0xe) | trailerSize (0x1) |  dataSize | eventSize (=header+trailer+data) |
 eventTimeSec | eventTimeMicrosec |
 triggerMask (0x1 or 0x2) | isPedMask | isPedFromScaler |
 sanityFlag | headerEndMarker (0xaccadead)
 ```
- **VME modules data** CAEN data format (QDCs V792, V792N; TDCs V775, V775N)
  - For each module: 1 header word + $n_{chan}$ data words + 1 trailer. 32 bits each word

- **Event trailer**: 1 word, 32 bits
 ```
 eventTrailer (0xbbeeddaa)
 ```


