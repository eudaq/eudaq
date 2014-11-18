#!/usr/bin/env python

import serial
import sys
import hameg2030 as h
import time

def main():
    vbb_dst=float(sys.argv[1]) # value to which the back-bias should be ramped
    sour=serial.Serial(h.dev, h.baud_rate, rtscts=h.rtscts)
    vbb_src=h.meas_voltage(sour, h.vbb_chan)
    h.ramp_from_to(sour, h.vbb_chan, (vbb_src, vbb_dst), 100)
    time.sleep(1)
    vbb_meas=h.meas_voltage(sour, h.vbb_chan)
    if abs(vbb_dst - vbb_meas) < 1.e-3:
        return 0
    else:
        return 1

## execute the main
if __name__ == "__main__":
    sys.exit(main())
