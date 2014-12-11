#! /usr/bin/env python
##
## doing a warm initialisation of the FEC HLVDS
##
import SlowControl # slow control code
import time

sc = SlowControl.SlowControl(0)

SlowControl.write_list(sc, 6007, [ 0xFFFFFFFF ], [ 0xFFFF0001 ], False, False)

quit()
