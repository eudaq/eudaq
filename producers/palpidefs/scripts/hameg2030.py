#!/usr/bin/env python

import time
import datetime
import sys

###############
# configuration
###############

current_limit=( 4*700., 3*700, 1 ) # in mA
voltages=( 5.0, 5.0, 0.0 ) # in V
vbb_chan=( 2 ) # back-bias voltage channel, counting starts with 0
measurement_interval=60 # in seconds

# set up serial port
dev="/dev/ttyHAMEG0"
baud_rate=int(9600)
rtscts=True # needed, otherwise Hameg might miss a command

###########
# functions
###########

###############################################################################
def power_off(sour):
    sour.write("INST OUT1\n")
    sour.write("OUTP OFF\n")
    sour.write("INST OUT2\n")
    sour.write("OUTP OFF\n")
    sour.write("INST OUT3\n")
    sour.write("OUTP OFF\n")

###############################################################################
def power_on(hameg, i_max, voltages):
    hameg.write("*IDN?\n")
    idn = hameg.readline()
    if not ("HAMEG" and "HMP2030") in idn:
        sys.stderr.write("WRONG DEVICE: %s" % idn)
        return
    #print idn
    #print "maximum current: %f %f %f" % i_max
    hameg.write("*RST\n")
    time.sleep(1)
    # activate digital fuse and set outputs to 0V
    # CH3
    hameg.write("INST OUT3\n");
    hameg.write("OUTP OFF\n")
    hameg.write("FUSE:LINK 1\n")
    hameg.write("FUSE:LINK 2\n")
    hameg.write("FUSE on\n")
    hameg.write("FUSE:DEL 50\n")
    hameg.write("SOUR:VOLT %f\n" % voltages[2])
    hameg.write("SOUR:CURR %f\n" % (float(i_max[2])/1000.))
    time.sleep(0.5)
    hameg.write("OUTP ON\n")
    # CH1
    hameg.write("INST OUT1\n");
    hameg.write("OUTP OFF\n")
    hameg.write("FUSE:LINK 2\n")
    hameg.write("FUSE:LINK 3\n")
    hameg.write("FUSE:DEL 50\n")
    hameg.write("FUSE on\n")
    hameg.write("SOUR:VOLT %f\n" % voltages[0])
    hameg.write("SOUR:CURR %f\n" % (float(i_max[0])/1000.))
    time.sleep(0.5)
    hameg.write("OUTP ON\n")
    # CH2
    hameg.write("INST OUT2\n");
    hameg.write("OUTP OFF\n")
    hameg.write("FUSE:LINK 1\n")
    hameg.write("FUSE:LINK 3\n")
    hameg.write("FUSE:DEL 50\n")
    hameg.write("FUSE on\n")
    hameg.write("SOUR:VOLT %f\n" % voltages[1])
    hameg.write("SOUR:CURR %f\n" % (float(i_max[1])/1000.))
    time.sleep(0.5)
    hameg.write("OUTP ON\n")

###############################################################################
def check_trip(sour):
    # check compliance
    tripped = False
    for c in range(3):
        sour.write("INST OUT%d\n" % (c+1))
        sour.write("FUSE:TRIP?\n")
        if (int(sour.readline())!=0):
            sys.stderr.write("Channel %d tripped\n" % (c+1))
            tripped = True
    return tripped

###############################################################################
def ramp_from_to(sour, channel, v, nsteps):
    for step in range(0, nsteps+1):
        sour.write("INST OUT%d\n" % (channel+1))
        sour.write("SOUR:VOLT %f\n" %
                   (float(v[0]+(float(v[1]-float(v[0]))*step/nsteps))))
        time.sleep(0.1);

###############################################################################
def meas_voltage(sour, channel):
    sour.write("INST OUT%d\n" % (channel+1))
    sour.write("MEAS:VOLT?\n")
    return float(sour.readline())

###############################################################################
def measure_all(sour, of):
    current=([0.0, 0.0, 0.0])
    voltage=([0.0, 0.0, 0.0])
    for c in range(3):
        sour.write("INST OUT%d\n" % (c+1))
        sour.write("MEAS:CURR?\n")
        current[c] = float(sour.readline())
        sour.write("MEAS:VOLT?\n")
        voltage[c] = float(sour.readline())
    of.write("%d\t%0.4fV\t%0.4fA\t%0.4fV\t%0.4fA\t%0.4fV\t%0.4fA\n" %
             ( int(time.time()), voltage[0], current[0], voltage[1],
               current[1], voltage[2], current[2]))
    of.flush()

###############################################################################
def measure_all_print(sour):
    current=([0.0, 0.0, 0.0])
    voltage=([0.0, 0.0, 0.0])
    for c in range(3):
        sour.write("INST OUT%d\n" % (c+1))
        sour.write("MEAS:CURR?\n")
        current[c] = float(sour.readline())
        sour.write("MEAS:VOLT?\n")
        voltage[c] = float(sour.readline())
    print "V1 (4DUTs)\tI1\t| V2 (3DUTs)\tI2\t| Vbb\t\tIbb"
    print "%0.4fV\t\t%0.4fA\t| %0.4fV\t%0.4fA\t| %0.4fV\t%0.4fA\n" % ( voltage[0], current[0], voltage[1], current[1], voltage[2], current[2])
