# ITS3 EUDAQ2 code and tools

## Installation

1. Install EUDAQ following the instructions in the main `README` file. To use EUDAQ for datataking, only `EUDAQ_BUILD_PYTHON` cmake flag is needed. To use online monitor enable `EUDAQ_BUILD_STDEVENT_MONITOR` and `USER_ITS3_BUILD` flags, e.g.:

        mkdir build && cd build
        cmake .. -DEUDAQ_BUILD_PYTHON=ON -DUSER_ITS3_BUILD=ON -DEUDAQ_BUILD_STDEVENT_MONITOR=ON
        make -j `nproc` && make install

2. Install Run Control dependencies:

        pip3 install urwid urwid_timed_progress libtmux rich

3. Install `alpide-daq-software` (<https://gitlab.cern.ch/alice-its3-wp3/alpide-daq-software.git>) following the instructions in the respective `README` file.
4. For trigger board clone <https://gitlab.cern.ch/alice-its3-wp3/trigger> and read respective `README`.
5. For PTH200 and HAMEG power supplies clone <https://gitlab.cern.ch/alice-its3-wp3/lab-equipment.git> and install (see respective `README`).
6. For DPTS clone <https://gitlab.cern.ch/alice-its3-wp3/dpts-utils.git> and follow the `README` for Picoscope driver installation.
7. For MLR1 DAQ board based planes clone <https://gitlab.cern.ch/alice-its3-wp3/apts-dpts-ce65-daq-software.git> and follow the `README` to install the software.

## Configuration

In `misc` dir create or modify `ITS3.ini` and `*.conf` files according to your setup. You can find examples for different producers in `misc/conf_examples`.

## Running

The following command will start the tmux session named ITS3 with multiple windows running all the necessary programs for DAQ:

    ./ITS3start.py path/to/file.ini

N.B. For DPTS trigger board must be connected to the same PC where DPTS is running.

## Stopping

From Run Control window:

    SHIFT+t : Stop the current run and terminate DAQ
    SHIFT+q : Exit Run Control

## Closing the tmux session

From tmux:

    CTRL+b : kill-session

From anywhere:

    tmux kill-session -t ITS3

## Additional information

In addition to the above mentioned `READMEs`, it is recommended to read the following documentation:

- [ALPIDE DAQ FPGA Firmware](https://gitlab.cern.ch/alice-its3-wp3/alpide-daq-fpga-firmware/-/blob/master/README.md)

### Hints on using Zaber moving stages

Manual: <https://www.zaber.com/protocol-manual?protocol=ascii>

Python serial terminal:

    python3 -m serial.tools.miniterm -e - 115200

Commands:

    /renumber # renumber all stages according to their position in daisy chain
    /get device.id # get all device ids, e.g reads back 50030 means X-LSM025A, see link below
    /home # needed after powering
    /get pos
    /1 move rel +1000
    /1 move abs 10234

Conversion of steps to mm: step size = 0.047625 um, see <https://www.zaber.com/manuals/X-LSM#m-14-specifications>.
Conversion table from Device ID to Device Name: <https://www.zaber.com/id-mapping?fw=7.22>.

### Hints on using the python scripts

To run, one typically needs to set the python path explicitly, also to the dependent packages in case that they are not installed via pip (e.g. for development purposes):

    PYTHONPATH=/tmp/eudaq2/lib/:/tmp/apts-dpts-ce65-daq-software/mlr1daqboard/ ./APTSDump.py

N.B. On **macos**, one needs to have the library as `.so` instead (or in addition to) the `.dylib`. You can do this by running e.g. the gollowing command:

    ln -s /tmp/eudaq2/lib/pyeudaq.{dylib,so}` (adjust the path).

### Using `StdEventMonitor` with config file

To decode DPTS events in online monitor you need to pass the configuration to `StdEventMonitor` via `-c /path/to/dpts.conf`. The configuration file must contain the a section as follows:

    [DPTS]
    calibration_file="/path/to/dpts.calib

You can create a calibration file using `scripts/DPTS_calib.py`, e.g.:

    ./DPTS_calib.py -i 0 30 DPTSOW22B7_VBB-1.2V_calibration.npy a 1.02 1.02 -i 1 30 DPTSOW22B6_VBB-1.2V_calibration.npy a 1.02 1.02 -o 2022-05_PS_B7_B6_1.2V.calib

The `npy` files can be obtained from CERNBox/EOS DPTS file storage.
