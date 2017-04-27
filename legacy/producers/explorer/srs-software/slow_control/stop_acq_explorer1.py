#! /usr/bin/env python
##
## code testing of class SlowControl
##
import SlowControl # slow control code
import time

m = SlowControl.SlowControl(0) # HLVDS FEC (master)
s = SlowControl.SlowControl(1) # ADC FEC (slave)

# enable digital I/Os on the master
SlowControl.write_list(m, 0x1977, [0x2, 0x1], [ 0x300, 0x100 ], False)


# disaable sync on the master
# stop readout
SlowControl.write_list(m, 6039, [ 0x16, 0x19 ], [ 0x0, 0x0 ], False)

time.sleep(1)

# disable slave
SlowControl.write_burst(s, 6039, 0x3, [ 0x0 ], False)


quit()
