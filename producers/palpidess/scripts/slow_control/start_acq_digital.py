#! /usr/bin/env python
##
## starting continous readout of the FEC HLVDS only
##
import SlowControl # slow control code

m = SlowControl.SlowControl(0) # HLVDS FEC (master)

# enable digital I/Os on the master
SlowControl.write_list(m, 0x1977, [0x2, 0x1], [ 0x300, 0x1FF ], False)

# enable sync on the master
# start readout
SlowControl.write_list(m, 6039, [ 0x16 ], [ 0x1 ], False)

quit()
