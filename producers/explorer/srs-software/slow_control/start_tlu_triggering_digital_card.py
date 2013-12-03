#! /usr/bin/env python
##
## code testing of class SlowControl
##
import SlowControl # slow control code

master = SlowControl.SlowControl(0)
#slave = SlowControl.SlowControl(1)


### TESTING USING THE FUNCTIONS
#SlowControl.read_burst(master, 6007, 0, 24, False)

#addresses = [ 0x15, 0x16, 0x17, 0x18, 0x19, 0x20, 0x21, 0x22, 0x23 ]
#SlowControl.read_list(master, 6039, addresses, False)

#SlowControl.write_burst(master, 0x1977, 0x00000100, [ 0x3200 ], False)
SlowControl.write_list(master, 6039, [ 0x18, 0x16 ], [ 0x2, 0x1 ], False)


#addresses = [ 26, 27, 28 ]
#SlowControl.write_list(master, 6039, addresses, data, False)

quit()
