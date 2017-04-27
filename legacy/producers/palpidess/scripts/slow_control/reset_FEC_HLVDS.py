#! /usr/bin/env python
##
## synchronous reset via slow control of the FEC HLVDS
##
import SlowControl # slow control code

sc = SlowControl.SlowControl(0)

# send a reset pulse of 1 us to all parts of the application unit,
# with a synchronous reset
#
# bit 0: complete synchronous reset
#     1: send reset to all asynchronous reset lines
#     2: reset the DUT driver only
#     3: reset the trigger unit
#     4: reset the timer

# reset register is located at the address 0xFFFFFFFF
# value:
# 15 downto  0: reset signal lines
# 23 downto 16: length of the reset pulse (x+1)*100ns
# 31 downto 24: ???

# issue asynchronous reset for 1 us:
SlowControl.write_list(sc, 6039, [ 0xFFFFFFFF ], [ 0x90002 ], False)

quit()
