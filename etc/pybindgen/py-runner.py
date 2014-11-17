#!/usr/bin/env python2
import sys, os, time
import argparse # argument parsing
from numpy import *
from ctypes import POINTER, c_uint64

sys.path.append( '../lib/' )
from libPyPyPacket import *


def takeData():
    delay = 1.0 / args.rate - 0.0002
    if delay < 0 :
        delay = 0
    i = 0
    data = arange( args.data_size, dtype=uint64 )
#   data = data.astype(numpy.uint64)
    data_p = data.ctypes.data_as(POINTER(c_uint64))
    prev_t = time.time()
    prev_i = 0
    while not pp.Error and not pp.StoppingRun and not pp.Terminating and i < args.packets:
        p = eudaq.PyPacket( '-TLU2-', 'test' )
        p.addMetaData( True, 4, i )
        p.addMetaData( False, 3, long(time.time() * 1000) )
        p.setDataSize( len(data) )
        p.nextToSend()
        pp.sendPacket()    
        sleep( delay )
        i += 1
        if i % args.rate == 0:
            now = time.time()
            dt = now - prev_t
            if dt > 0 :
                rate = (i - prev_i) / dt
            else:
                rate = 0
            print '{0:5d} {1:.0f}Hz'.format( i, rate )
            prev_t = now
            prev_i = i



parser = argparse.ArgumentParser(prog=sys.argv.pop(0), description="A script for EUDAQ tests")
parser.add_argument("--eudaq-pypath", help="Path to EUDAQ Python ctypes wrapper (usually /path/to/eudaq/python)", metavar="PATH")
parser.add_argument("--data-size", help="size of data to be sent per packet in uint64_t", default=1000, type=int )
parser.add_argument("--packets", help="number of packets to be sent", default=1000, type=int )
parser.add_argument("--rate", help="packet rate in Hz", default=100, type=int )
parser.add_argument("--collector", help="protocol, host, port where data collector is listening", default="tcp://44001" )
parser.add_argument("--no-collector", help="don't run a data collector", action="store_true" )
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
rc = PyRunControl() # using defaults: listen to address 44000

print "Starting (Test-) Producer"
# create PyProducer instance
pp = PyProducer("testproducer")

if args.no_collector :
    print "NO DataCollector started"
else:
    print "Starting DataCollector"
    pdc = PyDataCollector( "testCollector", "tcp://localhost:44000", str(args.collector)  ) # create main data collector


# wait for more than one active connection to appear
while rc.NumConnections < 2 or not rc.AllOk:
    sleep(1)
    rc.PrintConnections()
    print " ----- "

rc.PrintConnections()
print "Current Run Number: " + str(rc.RunNumber)
runnr = rc.RunNumber # remember the current number

# load configuration file
rc.Configure("test")

# counter variables for simple timeout
i = 0
maxwait = 100
waittime = .5
# wait until all connections answer with 'OK'
while i<maxwait and not rc.AllOk:
    sleep(waittime)
    i+=1
    print "Waiting for configure for ",i*waittime," seconds"
    rc.PrintConnections()
    # our producer is not connected to actual hardware, so we just set the configured status here
    if pp.Configuring:
        pp.Configuring = True
# check if all are configured
if rc.AllOk:
    print "Successfullly configured!"
else:
    print "Not all connections returned 'OK' after ",maxwait*waittime," seconds!"
    abort()
# start the run
rc.StartRun()

# wait for run start
i = 0
while i<maxwait and not rc.AllOk:
    sleep(waittime)
    print "Waiting for run start for ",i*waittime," seconds, number of active connections: ", rc.NumConnections
    i+=1
    # producer not connected to hardware, so we can start the run immediately:
    if pp.StartingRun:
        pp.StartingRun = True # set status and send BORE
# check if all connected commandreceivers started up
if rc.AllOk:
    print "Successfullly started run!"
else:
    print "Not all connections returned 'OK' after ",maxwait*waittime," seconds!"
    abort()

print "New Run Number: ", rc.RunNumber
if rc.RunNumber == runnr:
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

takeData()

if not pp.Error:
    print "Successfullly finished run!"

# stop run
#rc.StopRun()
