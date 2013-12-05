#!/usr/bin/env python2
execfile('PyEUDAQWrapper.py')
from time import sleep

print "Starting RunControl"
prc = PyRunControl("44000")
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

prc.Configure("test")
i = 0
maxwait = 10
while i<maxwait:
    sleep(.5)
    print "Waiting for configure",i,"/",maxwait,", Number of active connections: ", prc.NumConnections()
    i+=1
prc.StartRun()
sleep(10)
while True:
    sleep(.1)
    pp.RandomEvent(500)
    print "running"

prc.StopRun()
