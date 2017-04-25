#!/usr/bin/env python
##
## code testing of class SlowControl
##
import sys
import SlowControl # slow control code


###############################################################################
## set bias voltage
###############################################################################
def set_bias_voltage(channel, voltage, sc):
    v = float(voltage)
    c = int(channel)

    #print "channel=%d" % c
    #print "voltage=%f" % v

    adc_val = int(4096.*v/2.5/2.)
    # Vout = 2*Vref*x/4096
    # Vref = 2.5V
    if adc_val < 0 or adc_val > 4095:
        "ADC value out of range, possible values are 0 ... 4.99 V"

    #print "ADC value=0x%x" % adc_val

    i2c_address = 0xA8320000
    # a write access looks like:
    # - I2C address
    # - pointer (channel address)
    # - two data bytes:
    #    15:14 register = 11 for the output register
    #    13:2  DAC value (Vout)
    #     1:0  padding = 00
    i2c_data    = (c & 0xFF)<<16 | 0xC000 | (adc_val & 0xFFF)<<2

    # open I2C on connector 8
    SlowControl.write_list(sc, 0x1877, [0xE0300000], [0x80], False)

    # set the DAC value
    SlowControl.write_list(sc, 0x1877, [i2c_address], [i2c_data], False)



###############################################################################
## main
###############################################################################
#
#  require two parameters
#  1.: channel number (0..15)
#  2.: voltage
#
def main():
    if not (len(sys.argv) == 3):
        print 'received {0:d} '.format(len(sys.argv)), \
              'instead of 2 parameters!'
        print './biasDAC.py <channel> <voltage>'
        print '\tchannel: 0..15'
        print '\tvoltage: 0 .. 4.99 (in Volts)'
        return 1
    else:
        sc = SlowControl.SlowControl(0)
        set_bias_voltage(sys.argv[1], sys.argv[2], sc)
        return 0

## execute the main
if __name__ == "__main__":
    sys.exit(main())
