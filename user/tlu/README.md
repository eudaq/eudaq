# Trigger Logic Units (TLU) supported in EUDAQ2

Find the application (starting scripts and conf-file) for EUDET telescope in user/eudet/misc

## EUDET TLU 

It is only built if cmake finds the external dependencies:
- tlufirmware (copy these in user/tlu/extern)
- ZestSC1 (copy these in user/tlu/extern)
- libusb (Ubuntu: ```sudo apt install libusb-dev```)
On Unix: Copy udev-rule in the proper folder of the OS.

## AIDA TLU

It is only built if cmake finds the external dependencies:
- Cactus/Ipbus

Follow these instructions: https://ipbus.web.cern.ch/ipbus/doc/user/html/software/install/compile.html#instructions




