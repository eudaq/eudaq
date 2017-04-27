#! /usr/bin/env python
##
## code testing of class SlowControl
##
import SlowControl # slow control code

dHLVDS = SlowControl.SlowControl(0)

SlowControl.read_burst(dHLVDS, 6039, 0, 32, False)

quit()
