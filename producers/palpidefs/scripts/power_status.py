#!/usr/bin/env python

import serial
import sys
import time
import hameg as h

def main():
    sour=serial.Serial(h.dev, h.baud_rate, rtscts=h.rtscts)
    print ""
    print (time.strftime("%Y-%m-%d %H:%M:%S:"))
    print ""
    h.measure_all_print(sour)
    if h.check_trip(sour):
        print "Tripped at %s (%d)\n" % (time.strftime("%Y-%m-%d_%H-%M-%S"), int(time.time()))
        sys.exit(1)
        time.sleep(1.)
    sys.exit(0)

## execute the main
if __name__ == "__main__":
    sys.exit(main())
