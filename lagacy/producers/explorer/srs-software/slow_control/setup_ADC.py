#! /usr/bin/env python
##
## code testing of class SlowControl
##
import SlowControl # slow control code

dADC = SlowControl.SlowControl(1)

#SlowControl.write_burst(dADC, 6519, 0x1, [0x0, 0x0, 0x0, 0x0], False)
SlowControl.write_list(dADC, 6519, [0x2, 0x3, 0x4], [0x0f, 0x0, 0x0], False)

# 01 = power down CH0
# 02 = power down CH1
# 03 = equalizer level 0
# 04 = equalizer level 1
# 05 = TRGOUT enable
# 06 = BCLK enable

# Register Bit    7 6 5 4 3 2 1 0
# HDMI connector  4 5 6 7 0 1 2 3

quit()
