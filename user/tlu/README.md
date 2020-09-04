# Trigger Logic Units (TLU) supported in EUDAQ2

## Building

By default the cmake-flag is activated ```USER_TLU_BUILD=ON```.
This builds the converter module for a standard event, for the online monitor for example.

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

* `tof_scint0`: Double time-of-flight of the 0th scintillator in nanoseconds. This value is rounded into 781.125ps bins and added to the fine timestamp of the 0th scintillator. Defaults to `0` (pass only value without units).
* `tof_scint1`: Double time-of-flight of the 1st scintillator in nanoseconds. This value is rounded into 781.125ps bins and added to the fine timestamp of the 1st scintillator. Defaults to `0` (pass only value without units).
* `tof_scint2`: Double time-of-flight of the 2nd scintillator in nanoseconds. This value is rounded into 781.125ps bins and added to the fine timestamp of the 2nd scintillator. Defaults to `0` (pass only value without units).
* `tof_scint3`: Double time-of-flight of the 3rd scintillator in nanoseconds. This value is rounded into 781.125ps bins and added to the fine timestamp of the 3rd scintillator. Defaults to `0` (pass only value without units).
* `tof_scint4`: Double time-of-flight of the 4th scintillator in nanoseconds. This value is rounded into 781.125ps bins and added to the fine timestamp of the 4th scintillator. Defaults to `0` (pass only value without units).
* `tof_scint5`: Double time-of-flight of the 5th scintillator in nanoseconds. This value is rounded into 781.125ps bins and added to the fine timestamp of the 5th scintillator. Defaults to `0` (pass only value without units).
