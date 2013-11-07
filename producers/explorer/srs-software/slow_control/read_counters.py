#! /usr/bin/env python
##
## code testing of class SlowControl
##
import SlowControl # slow control code

dHLVDS = SlowControl.SlowControl(0)
dADC = SlowControl.SlowControl(1)

SlowControl.read_burst(dHLVDS, 6039, 26, 4, False)
SlowControl.read_burst(dADC, 6039, 10, 4, False)


quit()
