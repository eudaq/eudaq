#!/usr/bin/env python2
import sys, os
import argparse # argument parsing

parser = argparse.ArgumentParser(prog=sys.argv.pop(0), description="A script for EUDAQ regression testing")
parser.add_argument("--eudaq-pypath", help="Path to EUDAQ Python ctypes wrapper (usually /path/to/eudaq/python)", metavar="PATH")
args = parser.parse_args(sys.argv)

if args.eudaq_pypath:
    modpath = str(args.eudaq_pypath)
    sys.path.append(args.eudaq_pypath)
else:
    modpath = ""

try:
    from PyEUDAQWrapper import *
except ImportError:
    print "ERROR: Could not find EUDAQ ctypes wrapper (PyEUDAQWrapper.py) -- please use --eudaq-pypath argument to set correct path"
    sys.exit(1)
    
from time import sleep

print "Starting RunControl"
prc = PyRunControl() # using defaults: listen to address 44000

print "Starting (Test-) Producer"
# create PyProducer instance
pp = PyProducer("testproducer")

print "Starting DataCollector"
# create PyDataCollector instance with default options
pdc = PyDataCollector() # create main data collector


# wait for more than one active connection to appear
while prc.NumConnections < 2 or not prc.AllOk:
    sleep(1)
    prc.PrintConnections()
    print " ----- "

prc.PrintConnections()
print "Current Run Number: " + str(prc.RunNumber)
runnr = prc.RunNumber # remember the current number

# load configuration file
prc.Configure("test")

# counter variables for simple timeout
i = 0
maxwait = 100
waittime = .5
# wait until all connections answer with 'OK'
while i<maxwait and not prc.AllOk:
    sleep(waittime)
    i+=1
    print "Waiting for configure for ",i*waittime," seconds"
    prc.PrintConnections()
    # our producer is not connected to actual hardware, so we just set the configured status here
    if pp.Configuring:
        pp.Configuring = True
# check if all are configured
if prc.AllOk:
    print "Successfullly configured!"
else:
    print "Not all connections returned 'OK' after ",maxwait*waittime," seconds!"
    abort()
# start the run
prc.StartRun()

# wait for run start
i = 0
while i<maxwait and not prc.AllOk:
    sleep(waittime)
    print "Waiting for run start for ",i*waittime," seconds, number of active connections: ", prc.NumConnections
    i+=1
    # producer not connected to hardware, so we can start the run immediately:
    if pp.StartingRun:
        pp.StartingRun = True # set status and send BORE
# check if all connected commandreceivers started up
if prc.AllOk:
    print "Successfullly started run!"
else:
    print "Not all connections returned 'OK' after ",maxwait*waittime," seconds!"
    abort()

print "New Run Number: ", prc.RunNumber
if prc.RunNumber == runnr:
    print "ERROR: new run number identical to previous one!"
    abort()
    
# prepare an array of dim (1,60) for data storage
data = numpy.ndarray(shape=[1,60], dtype=numpy.uint64)
data[0] = ([31415926535,271828182845,124567890,
                31415926535,271828182845,124567890,
                31415926535,271828182845,124567890,
                31415926535,271828182845,124567890,
                31415926535,271828182845,124567890,
                31415926535,271828182845,124567890,
                31415926535,271828182845,124567890,
                31415926535,271828182845,124567890,
                31415926535,271828182845,124567890,
                31415926535,271828182845,124567890,
                31415926535,271828182845,124567890,
                31415926535,271828182845,124567890,
                31415926535,271828182845,124567890,
                31415926535,271828182845,124567890,
                31415926535,271828182845,124567890,
                31415926535,271828182845,124567890,
                31415926535,271828182845,124567890,
                31415926535,271828182845,124567890,
                31415926535,271828182845,124567890,
                31415926535,271828182845,124567890
]) # add some (dummy) data


# main run loop
i = 0
while not pp.Error and not pp.StoppingRun and not pp.Terminating and i < 1:
    pp.SendEvent(data)        # send event with some (fixed) dummy data off
    sleep(0.01) # wait a little while
    i+=1

i = 0
metadata = numpy.ndarray(shape=[1,3], dtype=numpy.uint64)
metadata[0] = ([31415926535,271828182845,124567890])
while not pp.Error and not pp.StoppingRun and not pp.Terminating and i < 1000:
    pp.SendPacket( metadata, data )    
    sleep(0.01) # wait a little while
    i+=1
    if i % 100 == 0:
        print str(i)

if not pp.Error:
    print "Successfullly finished run!"

# stop run
prc.StopRun()
