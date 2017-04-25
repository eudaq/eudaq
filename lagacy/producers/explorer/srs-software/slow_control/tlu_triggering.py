#! /usr/bin/env python
##
## code testing of class SlowControl
##
import SlowControl # slow control code

dHLVDS = SlowControl.SlowControl(0)
dADC = SlowControl.SlowControl(1)

# HLVDS: trigger is in register 24
#  1: 0 triggering mode: 0 = auto, 1 = ext 2 = TLU
#  2    TLU reset signal enable
#  3    busy mode: 0 = excluding driver, 1 = including driver
# 10: 4 clk div
# 18:12 wait length
trg_set = 2 # = TLU + busy w/o driver
trg_set = trg_set | (0x18 << 4)  # clk divider (0x18) (minimum working 0x5 in the lab)
trg_set = trg_set | (0x2f << 12) # wait length (0x2f)
SlowControl.write_list(dHLVDS, 6039, [ 24 ], [ trg_set ], False)
SlowControl.write_list(dADC,   6039, [  2 ], [ trg_set ], False)


quit()
