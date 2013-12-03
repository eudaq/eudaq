#! /usr/bin/env python
##
## code testing of class SlowControl
##
import SlowControl # slow control code

master = SlowControl.SlowControl(0)

# open I2C on connector 8
SlowControl.write_list(master, 0x1877, [0xE0300000], [0x80], False)

# set the reset voltages
# 12 bit precision
# Vout = 2*Vref*x/4096
# Vref = 2.5V
#
# value has to be shifted two bits to the left (*4) (see explanation below)
#
# 0x51e = 0.4V
# 0xa3c = 0.8V
#
# a write access looks like:
# - I2C address
# - pointer (channel address)
# - two data bytes:
#    15:14 register = 11 for the output register
#    13:2  DAC value (Vout)
#     1:0  padding = 00
SlowControl.write_list(master, 0x1877,
                       [0xA8320000, 0xA8320000, 0xA8320000, 0xA8320000],
                       [0x03CA3C, 0x06CA3C, 0x07CA3C, 0x0BCA3C], False)

# set the voltages to zero
#SlowControl.write_list(master, 0x1877,
#                       [0xA8320003, 0xA8320006, 0xA8320007, 0xA832000B],
#                       [0x03C000, 0x06C000, 0x07C000, 0x0bC000], False)

# readback registers
# SlowControl.read_list(master, 0x1877, [0xA9320000, 0xA9320006, 0xA9320007, 0xA932000B], False)
# not working so far ( TODO )

# control register access ( TODO )
#REG1=REG0=0
#Pointer (msb): A3:A0=1100
#CR10: internal reference 0 - 1.25, 1 - 2.5
#C8: 0 -> external reference 1 -> internal reference

quit()
