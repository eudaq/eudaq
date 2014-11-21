#!/usr/bin/env python2
from PyEUDAQWrapper import * # load the ctypes wrapper
from StandardEvent_pb2 import StandardEvent

import random
from time import sleep
import numpy as np # for data handling

print "Starting PyProducer"
# create PyProducer instance
pp = PyProducer("GENERIC","tcp://localhost:44000")

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
    sleep(2)
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
    sleep(2)
    pp.StartingRun = True # set status and send BORE
# starting to run

tluevent = 0
while not pp.Error and not pp.StoppingRun and not pp.Terminating:

    event = StandardEvent()
    plane = event.plane.add() #add one plane

    plane.type = "OO"
    plane.id = 0
    plane.tluevent = tluevent
    plane.xsize = 64
    plane.ysize = 32

    frame = plane.frame.add()
    for _ in range(random.randint(1,16)):
        pix = frame.pixel.add()
        pix.x = random.randint(0,63)
        pix.y = random.randint(0,31)
        pix.val = 1
    
    tluevent =  tluevent + 1

    data = np.fromstring(event.SerializeToString(), dtype=np.uint8)
    print tluevent, data

    pp.SendEvent(data)        # send event off
    sleep(2)                  # wait for a little while

# check if the run is stopping regularly
if pp.StoppingRun:
    pp.StoppingRun=True # set status and send EORE
