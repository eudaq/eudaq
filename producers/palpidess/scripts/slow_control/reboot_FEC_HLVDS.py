#! /usr/bin/env python
##
## reboot the FEC HLVDS
##
import SlowControl # slow control code
import time

sc = SlowControl.SlowControl(0)

SlowControl.write_list(sc, 6007, [ 0xFFFFFFFF ], [ 0xFFFF8000 ], False, False)

quit()
