#!/usr/bin/env python3
import pyeudaq
from time import sleep
from collections import deque
import argparse
from datetime import datetime
from utils import exception_handler

class ITS3DataCollector(pyeudaq.DataCollector):
    def __init__(self, name, runctrl):
        pyeudaq.DataCollector.__init__(self, name, runctrl)
        self.producer_queues={}
        self.nsev=0
        self.ndev=0
        self.async_check = datetime.now()

    @exception_handler
    def DoInitialise(self):
        conf=self.GetInitConfiguration().as_dict()
        self.dataproducers=[p.strip() for p in conf['dataproducers'].split(',')]
        self.nsev=0
        self.ndev=0

    @exception_handler
    def DoConfigure(self):
        self.nsev=0
        self.ndev=0

    @exception_handler
    def DoStartRun(self):
        self.nsev=0
        self.ndev=0
        self.producer_queues={}
        
    @exception_handler
    def DoStopRun(self):
        # Wait for all events to be written...
        # FIXME: what if senders still have stuff to send??
        #self.BuildAndWrite()
        #print('STOP:',self.iev,'remaining events in queues: ',[len(q) for q in self.producer_queues.values()])
        pass

    @exception_handler
    def DoReset(self):
        pass

    @exception_handler
    def DoStatus(self):
        self.SetStatusTag('StatusEventN','%d'%self.nsev)
        self.SetStatusTag('DataEventN'  ,'%d'%self.ndev)
        qd=sum(len(q) for q in self.producer_queues.values())
        if self.async_check:
            if qd==0:
                self.async_check = False
                self.SetStatusMsg('Running')
            elif (datetime.now()-self.async_check).total_seconds()>=10:
                self.SetStatusMsg('Warning! Out of sync! ' + ' '.join(sorted( \
                    f"{k.GetName()[:2]}{k.GetName()[-1]}:{len(v)}" \
                        for k,v in self.producer_queues.items() if len(v) )))
        elif qd:
            self.async_check = datetime.now()
    
    @exception_handler
    def DoConnect(self, con):
#        t = con.GetType()
#        n = con.GetName()
#        r = con.GetRemote()
#        print ("new producer connection: ", t, n, "from ", r)
        if con.GetName() in self.dataproducers:
            self.producer_queues[con]=deque()

    @exception_handler
    def DoDisconnect(self, con):
        # TODO: raise sth!!
#        t = con.GetType()
#        n = con.GetName()
#        r = con.GetRemote()
#        print ("delete producer connection: ", t, n, "from ", r)
        if con in self.producer_queues: del self.producer_queues[con]

    @exception_handler
    def DoReceive(self,con,subev):
        #print(subev)
        if 'status' in subev.GetDescription():
            print(subev)
            self.WriteEvent(subev)
            self.nsev+=1
        else:
            self.producer_queues[con].append(subev)
            self.BuildAndWrite()

    @exception_handler
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
    parser=argparse.ArgumentParser(description='ITS3 Data Collector',formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('--name' ,'-n',default='dc')
    parser.add_argument('--run-control' ,'--rc',default='tcp://localhost:44000')
    args=parser.parse_args()

    dc=ITS3DataCollector(args.name,args.run_control)
    dc.Connect()
    sleep(2) # TODO: less sleep
    while dc.IsConnected():
        sleep(1)

