#!/usr/bin/env python

import serial
import sys
import hameg2030 as h
import time

def main():
    vlight_dst=float(sys.argv[1]) # value to which the vlight should be ramped
    sour=serial.Serial(h.dev, h.baud_rate, rtscts=h.rtscts)
    vlight_src=h.meas_voltage(sour, h.vlight_chan)
    if abs(vlight_dst - vlight_src) < 1.3e-3:
        return 0
    h.ramp_from_to(sour, h.vlight_chan, (vlight_src, vlight_dst), 100)
    time.sleep(1)
    vlight_meas=h.meas_voltage(sour, h.vlight_chan)
    if abs(vlight_dst - vlight_meas) < 1.e-3:
        return 0
    else:
        return 1

## execute the main
if __name__ == "__main__":
    sys.exit(main())
