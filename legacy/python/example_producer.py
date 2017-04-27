#!/usr/bin/env python2
from PyEUDAQWrapper import * # load the ctypes wrapper
from time import sleep
import numpy # for data handling

print "Starting PyProducer"
# create PyProducer instance
pp = PyProducer("testproducer","tcp://localhost:44000")

i = 0 # counter variables for wait routines
maxwait = 100
waittime = .5
# wait for configure cmd from RunControl
while i<maxwait and not pp.Configuring:
    sleep(waittime)
    print "Waiting for configure for ",i*waittime," seconds"
    i+=1
# check if configuration received
if pp.Configuring:
    print "Ready to configure, received config string 'Parameter'=",pp.GetConfigParameter("Parameter")
    # .... do your config stuff here ...
    sleep(5)
    pp.Configuring = True
# check for start of run cmd from RunControl
while i<maxwait and not pp.StartingRun:
    sleep(waittime)
    print "Waiting for run start for ",i*waittime," seconds"
    i+=1
# check if we are starting:
if pp.StartingRun:
    print "Ready to run!"
    # ... prepare your system for the immanent run start
    sleep(5)
    pp.StartingRun = True # set status and send BORE
# starting to run
while not pp.Error and not pp.StoppingRun and not pp.Terminating:
    # prepare an array of dim (1,3) for data storage
    data = numpy.ndarray(shape=[1,3], dtype=numpy.uint8)
    data[0] = ([123,456,999]) # add some (dummy) data
    pp.SendEvent(data)        # send event off
    sleep(2)                  # wait for a little while
# check if the run is stopping regularly
if pp.StoppingRun:
    pp.StoppingRun=True # set status and send EORE
