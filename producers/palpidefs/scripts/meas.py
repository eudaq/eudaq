#!/usr/bin/env python

import serial
import sys
import time
import hameg2030 as h

def main():
    of_filepath=sys.argv[1] # where should the log file be written to
    sour=serial.Serial(h.dev, h.baud_rate, rtscts=h.rtscts)
    of = open(of_filepath, "a")
    while True:
        try:
            h.measure_all(sour, of)
            for sec in range(int(h.measurement_interval)):
                if h.check_trip(sour):
                    tf = open("TRIPPED.txt", "a")
                    tf.write("%d\n" % int(time.time()))
                    tf.close()
                    sys.exit(1)
                time.sleep(1.)
        except KeyboardInterrupt:
            of.close()
            sys.exit(0)

## execute the main
if __name__ == "__main__":
    sys.exit(main())
