#! /usr/bin/env python
##
## prepare the SRS for the readout of an Explorer-1 in single-event
## readout mode
##
import SlowControl # slow control code
import time

m = SlowControl.SlowControl(0) # HLVDS FEC (master)

# enable digital I/Os on the master (0x1977 = 6519)
SlowControl.write_list(m, 0x1977, [0x2, 0x1], [ 0x300, 0x1FF ], False)

# activate the single-event readout mode (0x16 -> 0x4)
# this bit has to be set to make trigger/timer unit running
SlowControl.write_list(m, 6039, [ 0x16 ], [ 0x4 ], False)

quit()
