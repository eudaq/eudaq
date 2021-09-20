#!/usr/bin/env python3
import pyeudaq
from time import sleep
from collections import deque

class ITS3DataCollector(pyeudaq.DataCollector):
    def __init__(self, name, runctrl):
        pyeudaq.DataCollector.__init__(self, name, runctrl)
        self.producer_queues={}
        self.nsev=0
        self.ndev=0

    def DoInitialise(self):
        conf=self.GetInitConfiguration().as_dict()
        self.dataproducers=[p.strip() for p in conf['dataproducers'].split(',')]
        self.nsev=0
        self.ndev=0

    def DoConfigure(self):
        self.nsev=0
        self.ndev=0

    def DoStartRun(self):
        self.nsev=0
        self.ndev=0
        self.producer_queues={}
        
    def DoStopRun(self):
        # Wait for all events to be written...
        # FIXME: what if senders still have stuff to send??
        #self.BuildAndWrite()
        #print('STOP:',self.iev,'remaining events in queues: ',[len(q) for q in self.producer_queues.values()])
        pass
        
    def DoReset(self):
        pass

    def DoStatus(self):
        self.SetStatusTag('StatusEventN','%d'%self.nsev);
        self.SetStatusTag('DataEventN'  ,'%d'%self.ndev);

    def DoConnect(self, con):
#        t = con.GetType()
#        n = con.GetName()
#        r = con.GetRemote()
#        print ("new producer connection: ", t, n, "from ", r)
        if con.GetName() in self.dataproducers:
            self.producer_queues[con]=deque()
        
    def DoDisconnect(self, con):
        # TODO: raise sth!!
#        t = con.GetType()
#        n = con.GetName()
#        r = con.GetRemote()
#        print ("delete producer connection: ", t, n, "from ", r)
        if con in self.producer_queues: del self.producer_queues[con]
        
    def DoReceive(self,con,subev):
        #print(subev)
        if 'status' in subev.GetDescription():
            print(subev)
            self.WriteEvent(subev)
            self.nsev+=1
        else:
            self.producer_queues[con].append(subev)
            self.BuildAndWrite()

    def BuildAndWrite(self):
        #print('BUILD:',self.iev,'remaining events in queues: ',[len(q) for q in self.producer_queues.values()])
        if all(len(q)!=0 for q in self.producer_queues.values()):
            globalev=pyeudaq.Event('RawEvent','ITS3global')
            for queue in self.producer_queues.values():
                # TODO: cross-checks! (timing + eventids)
                subev=queue.popleft()
                globalev.AddSubEvent(subev)
            self.WriteEvent(globalev)
            self.ndev+=1

        
if __name__=='__main__':
    dc=ITS3DataCollector('dc','tcp://localhost:44000')
    dc.Connect()
    sleep(2) # TODO: less sleep
    while dc.IsConnected():
        sleep(1)

