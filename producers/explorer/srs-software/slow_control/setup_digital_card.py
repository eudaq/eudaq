#! /usr/bin/env python
##
## code testing of class SlowControl
##
import SlowControl # slow control code

master = SlowControl.SlowControl(0)

SlowControl.write_list(master, 0x1977, [0x2, 0x1], [ 0x300, 0x1FF ], False)

quit()
