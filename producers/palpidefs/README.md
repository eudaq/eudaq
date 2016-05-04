pALPIDEfs producer for the ALICE ITS Upgrade
============================================

Description
-----------

This producer is for the use with the full-scale ALPIDE chips of the ALICE ITS Upgrade.
Please contact Magnus Mager (CERN) for further information.

Installation
------------

First you need to get the driver https://git.cern.ch/web/pALPIDEfs-software.git .

You need to install `tinyxml` `libusb-1.0-0` and `unbuffer` (Ubuntu: `sudo apt-get install libtinyxml-dev expect-dev libusb-1.0-0-dev`).
ROOT is needed for the S-Curve scan analysis in the converter.

Compile pALPIDEfs-software with `make lib'`.

You need to add the driver location as a variable to cmake with
`cd <<EUDAQ_ROOT>>/-build`
`cmake -DBUILD_palpidefs=on -DCMAKE_PALPIDEFS_DRIVER_INCLUDE:FILEPATH=<<DRIVER LOCATION>> -DUSE_TINYXML=on -DUSE_ROOT=on ../`
`make install`

Configuration
-------------

An example configuration file can be found in the conf folder. The automatic configuration file generator is part of the driver repository.
