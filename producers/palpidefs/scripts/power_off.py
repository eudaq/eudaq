#!/usr/bin/env python

import serial
import sys
import time
import hameg as h


def main():
    sour=serial.Serial(h.dev, h.baud_rate, rtscts=h.rtscts);
    h.power_off(sour)
    print "turned off: %s" % (time.strftime("%Y-%m-%d %H:%M:%S"))

## execute the main
if __name__ == "__main__":
    sys.exit(main())
