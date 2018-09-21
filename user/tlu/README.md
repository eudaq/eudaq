# Trigger Logic Units (TLU) supported in EUDAQ2

Find the application (starting scripts and conf-file) for EUDET telescope in [user/eudet/misc](../../user/eudet/misc)

## Building

By default the cmake-flag is activated ```USER_TLU_BUILD=ON```.
This builds the converter module for a standard event, for the online monitor for example.

If cmake finds the EUDET TLU or the AIDA TLU dependencies, these producers are automatically build (by switching on the flags ```USER_TLU_BUILD_EUDET``` or ```USER_TLU_BUILD_AIDA```).

## EUDET TLU producer and dependencies

It is only built if cmake finds the external dependencies:
- tlufirmware (copy these in user/tlu/extern)
- ZestSC1 (copy these in user/tlu/extern)
- libusb (Ubuntu: ```sudo apt install libusb-dev```)
On Unix: Copy the [udev-rule](misc/eudet-tlu/54-tlu.rules) in the proper folder of the OS.

To start the EUDET TLU producer: 
```euCliProducer -n EudetTluProducer```

## AIDA TLU producer and dependencies

It is only built if cmake finds the external dependencies:
- Cactus/Ipbus (Follow these [instructions](https://ipbus.web.cern.ch/ipbus/doc/user/html/software/install/compile.html#instructions). Cactus should be installed to /opt/cactus which is the default.)

To start the EUDET TLU producer: 
```euCliProducer -n AidaTluProducer```


## Conversion

If LCIO is built - which is done automatically - the conversion of raw data containing **TluRawDataEvent** events happens by:
```euCliConverter -i data.raw -o data.slcio```



