#! /usr/bin/env python
##
## code testing of class SlowControl
##
import SlowControl # slow control code
import time

sc = SlowControl.SlowControl(1)

rply = SlowControl.read_burst(sc, 6039, 7, 1)

dcm_status = 0x3ffff

while rply.success == False or rply.errors[0] != 0x0 or rply.data[0] != dcm_status:
    print "Rebooting FEC ADC as it doesn't seem to be ready!"
    if rply.success == True:
        print "DCM status: 0x%x" % rply.data[0]
    else:
        print "Readout not successful!"
    SlowControl.write_list(sc, 6007, [ 0xFFFFFFFF ], [ 0xFFFF8000 ])
    time.sleep(15)
    rply = SlowControl.read_burst(sc, 6039, 7, 1)

print "ADC DCM seems to be alright"


quit()
