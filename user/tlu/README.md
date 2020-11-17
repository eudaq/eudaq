# Trigger Logic Units (TLU) supported in EUDAQ2

## Building

By default the cmake-flag is activated ```USER_TLU_BUILD=ON```.
This builds the converter module for a standard event, for the online monitor for example.

To build only the converters the cmake-flag
```USER_BUILD_TLU_ONLY_CONVERTER``` can be set to ON.

If cmake finds the EUDET TLU or the AIDA TLU dependencies, these producers are automatically build (by switching on the flags ```USER_TLU_BUILD_EUDET``` or ```USER_TLU_BUILD_AIDA```).

## EUDET TLU producer and dependencies

### Dependencies

It is only built if cmake finds the external dependencies:
- tlufirmware (copy these in user/tlu/extern)
- ZestSC1 (copy these in user/tlu/extern)
- libusb (Ubuntu: ```sudo apt install libusb-dev```)
On Unix: Copy the [udev-rule](misc/eudet_tlu/54-tlu.rules) in the proper folder of the OS.

Please find here the hardware [manual](https://telescopes.desy.de/File:EUDET-MEMO-2009-04.pdf).

### Usage

With the command line tool ```EudetTluControl``` all functionalities can be tested.
To start the EUDET TLU producer: ```euCliProducer -n EudetTluProducer```
The usage with EUDET-type telescopes is described [here](https://telescopes.desy.de/User_manual#3._Starting_EUDAQ_NI_and_TLU_producer).

## AIDA TLU producer and dependencies

### Dependencies

It is only built if cmake finds the external dependencies:
- Cactus/Ipbus (Follow these [instructions](https://ipbus.web.cern.ch/ipbus/doc/user/html/software/install/compile.html#instructions). Cactus should be installed to /opt/cactus which is the default.)

Please find here the hardware [manual](https://www.ohwr.org/project/fmc-mtlu/blob/master/Documentation/Main_TLU.pdf).

### Usage

To start the EUDET TLU producer ```euCliProducer -n AidaTluProducer```.
The usage with EUDAQ2 and EUDET-type telescopes is described [here](https://telescopes.desy.de/User_manual#Running_with_EUDAQ_2). Find the application (starting scripts and conf-file) for EUDET-type telescope in [user/eudet/misc](../../user/eudet/misc)

## Conversion

The conversion of raw data containing **TluRawDataEvent** is the same for both types of TLU. For example and if LCIO is built, you can convert by: ```euCliConverter -i data.raw -o data.slcio```
The following parameters can be passed in the configuration in order to influence the decoding behavior of this module:

* `trigger_mask`: Trigger mask to allow for an exclusion of particular trigger inputs for the calculation of the precise TLU trigger timestamp. Specified in hexadecimal format, with the i-th trigger corresponding to the i-th bit. Defaults to `0x3F` (all triggers enabled).
* `delay_scint0`, `delay_scint1`, ..., `delay_scint5`: Delay (time-of-flight + cable delays) of the i-th scintillator in 781.25ps bins as integer. This value is subtracted from the fine timestamp of the i-th scintillator in order to calculate the correct precise TLU trigger timestamp. Defaults to `0`. Please note that the most upstream scintillator should have a delay of zero.

The following flags are forwarded directly from the raw event to the standard event:
* `FINE_TS0`, `FINE_TS1`, ..., `FINE_TS5`: Fine timestamp of the i-th scintillator in 781.25ps bins.

The following flags are added in addition the the standard event:
* `DIFF_FINETS01_del_ns`, ..., `DIFF_FINETS45_del_ns`: Difference of the i-th and the j-th scinitllator timestamp converted into nanoseconds and including the delay values provided by `delay_scint0` etc.
