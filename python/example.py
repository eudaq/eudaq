#!/usr/bin/env python2
execfile('PyEUDAQWrapper.py')
from time import sleep

print "Starting RunControl"
prc = PyRunControl("44000")
sleep(1)
print "Number of active connections: ", prc.NumConnections()
sleep(1)
print "Starting Producer"
pp = PyProducer("testproducer","tcp://localhost:44000")
sleep(1)
print "Starting DataCollector"
pdc = PyDataCollector("tcp://localhost:44000","43000")

while True:
    sleep(0.1)
    print "Number of active connections: ", prc.NumConnections()
    if prc.NumConnections() > 1:
        break

prc.Configure("ExampleConfig")
sleep(2) # wait for a couple of seconds so that the connections can receive the config

i = 0
maxwait = 100
waittime = .5
while i<maxwait and not prc.AllOk():
    sleep(waittime)
    print "Waiting for configure for ",i*waittime," seconds, number of active connections: ", prc.NumConnections()
    i+=1
if prc.AllOk:
    print "Successfullly configured!"
else:
    print "Not all connections returned 'OK' after ",maxwait*waittime," seconds!"
    abort()
prc.StartRun()

i = 0
while i<maxwait and not prc.AllOk():
    sleep(waittime)
    print "Waiting for run start for ",i*waittime," seconds, number of active connections: ", prc.NumConnections()
    i+=1
if prc.AllOk:
    print "Successfullly started run!"
else:
    print "Not all connections returned 'OK' after ",maxwait*waittime," seconds!"
    abort()

while True:
    sleep(.1)
    pp.RandomEvent(500)
    print "running"

prc.StopRun()
