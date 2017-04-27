#!/usr/bin/env python

import serial
import sys
import hameg2030 as h



def main():
    sour=serial.Serial(h.dev, h.baud_rate, rtscts=h.rtscts);
    h.power_off(sour)

## execute the main
if __name__ == "__main__":
    sys.exit(main())
