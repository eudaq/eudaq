#! /usr/bin/env python
##
## code testing of class SlowControl
##
import SlowControl # slow control code
import time

m = SlowControl.SlowControl(0) # HLVDS FEC (master)

# request a single event
# in order to make the trigger/timer unit running, 0x4 (3rd bit) in
# register 0x15 has to be set
SlowControl.write_list(m, 6039, [ 0x16 ], [ 0x6 ], True, False)
SlowControl.write_list(m, 6039, [ 0x16 ], [ 0x4 ], True, False)

quit()
