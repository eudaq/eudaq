#! /usr/bin/env python
##
## code testing of class SlowControl
##
import SlowControl # slow control code

dHLVDS = SlowControl.SlowControl(0)
dADC = SlowControl.SlowControl(1)

SlowControl.read_burst(dHLVDS, 6039, 0, 32, False)
SlowControl.read_burst(dADC, 6039, 0, 16, False)

quit()
