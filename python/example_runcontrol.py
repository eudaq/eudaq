#!/usr/bin/env python2
from PyEUDAQWrapper import * # load the ctypes wrapper

from time import sleep

print "Starting RunControl"
prc = PyRunControl("44000") # listen to address 44000

# wait for more than one active connection to appear
while prc.NumConnections < 2:
    sleep(1)
    print "Number of active connections: ", prc.NumConnections

# load configuration file
prc.Configure("ExampleConfig")
sleep(2) # wait for a couple of seconds so that the connections can receive the config

# counter variables for simple timeout
i = 0
maxwait = 100
waittime = .5
# wait until all connections answer with 'OK'
while i<maxwait and not prc.AllOk:
    sleep(waittime)
    print "Waiting for configure for ",i*waittime," seconds, number of active connections: ", prc.NumConnections
    i+=1
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
# check if all connected commandreceivers started up
if prc.AllOk:
    print "Successfullly started run!"
else:
    print "Not all connections returned 'OK' after ",maxwait*waittime," seconds!"
    abort()

# main run loop
while True:
    sleep(.1)

# stop run
prc.StopRun()
