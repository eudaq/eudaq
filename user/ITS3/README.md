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
5. For PTH200, RTD23, ZABER moving stages and HAMEG power supplies clone <https://gitlab.cern.ch/alice-its3-wp3/lab-equipment.git> and install (see respective `README`).
6. For MLR1 DAQ board based planes clone <https://gitlab.cern.ch/alice-its3-wp3/apts-dpts-ce65-daq-software.git> and follow the `README` to install the software.

## Configuration

In `misc` dir create or modify `ITS3.ini` and `*.conf` files according to your setup. You can find examples for different producers in `misc/conf_examples`.

## Running

The following command will start the tmux session named ITS3 with multiple windows running all the necessary programs for DAQ:

    ./ITS3start.py path/to/file.ini

N.B. For DPTS trigger board must be connected to the same PC where DPTS is running.

### QA information in Run Control

The following information can be read from the Run Control window to asses the quality of the data being acquired:

- `DATA EV#` counters should be the same among all the telescope planes. Discrepancies can be allowed while triggers/spills are incoming, but within 2 seconds of spill end / stopping the trigger they should all match.
- `Warning! Out of sync!` message from data collector is related to the above point. It can appear during a spill but must dissaper within 2 seconds after the spill ends.

In case behavour is different than described, there is a problem and run is to be considered *BAD*.

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

### Hints on setting up trigger and busy

In every trigger/busy chain there should be one and only one trigger/busy logic controller. All the other devices should just receive and propagate trigger and busy signals.
The most common trigger/busy logic controller is the trigger board. Alternative options are ALPIDE DAQ board set in `primary` mode or a Picoscope as a part of DPTS Producer.

To use DPTS / Picoscope as trigger/busy logic controller:
- DPTS Producer has to be set up to trigger on one of the two Picoscope channels connected to DPTS
- AWG output of the Picoscope provides the TRIGGER signal (for other devices)
- AUX input of the Picoscope is the BUSY input - to be connected to the output of the busy chain

The Picoscope acquisition window length acts as trigger seperation time (equivalent to `dt_trg` in trigger board).


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

### Reading TRGMON counters

Every beginning of run event (BORE) and end of run event (EORE) of an ALPIDE plane includes the status of the trigger monitoring registers.
The trigger monitoring counts the number of incoming trigger reuqests and the decisions on their acceptance or rejection, as well as the reasons for the latter. In addition, the accumulated time spent in any of the busy conditions is counted.

The EORE and BORE status events for each plane appear in the datacollector window of the tmux session.
Offline, the TRGMON counters can be accessed by reading the run file via `euCliReader` and piping it to grep to search for the relevant register.

    ./euCliReader -d run123456.raw | grep 'NTRGACC' (or any other TRGMON register)

Alternatively, without having eudaq2 installed, a terminal file viewer of one's choice can be used in place of `euCliReader`, piping the output then to `grep -a`, e.g.:

    less run123456.raw | grep -a 'NTRGBSY'

The number of triggers requested (`NTRGREQ`) and accepted (`NTRGACC`) correspond to the number of events that were recorded and should be **equal**.
The number of triggers rejected for any reason (`NTRGBSY_*`) should be 0.
Should this not be the case, the trigger logic/chain needs to be adjusted.

Details on the output can be found here: <https://gitlab.cern.ch/alice-its3-wp3/alpide-daq-fpga-firmware#trigger-monitor>
